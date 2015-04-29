// MySynthMain.cpp
// シンセ本体および、そのコンポーネントのクラス。

#include <windows.h>
#include <math.h>
#include <list>
#include "common.h"
#include "debug.h"
#include "synthBase.h"
#include "MySynthMain.h"

// 波形テーブル
#define	WT_SIZE		1024				// 波形テーブルサイズ(単位はサンプル)
static SIGREAL g_sinTable[WT_SIZE];		// サイン波テーブル
static SIGREAL g_triTable[WT_SIZE * 68];	// 三角波テーブル（周波数帯域毎に分割）
static SIGREAL g_sawTable[WT_SIZE * 68];	// ノコギリ
static SIGREAL g_squTable[WT_SIZE * 68];	// 矩形

// 波形テーブル情報
typedef struct tagTableInfo {
	float freq;		// 基音周波数
	float noteNo;	// 基音周波数に対応するMIDIノートNo
	int harmoNum;	// 倍音数
	int offset;		// 対応する波形テーブルの先頭へのオフセット
} TABLEINFO;
static TABLEINFO g_tblInfo[68 + 1];	// テーブル情報表
static int g_tblInfoNum;			// テーブル情報表の行数


/////////////////////////////////////////////////////////////////////
// 波形テーブルの初期化関連
#define _SIN(a)	(g_sinTable[(a)%WT_SIZE])	// sin()関数の代わりにテーブルを使い、テーブル生成を高速化

// 三角波テーブル生成
static void makeTriTable(SIGREAL* pOutBuf, int harmoNum) {
	for (int i = 0; i<WT_SIZE; i++) {
		pOutBuf[i] = 0;
		for (int j = 1; j <= harmoNum; j += 2) {		// harmoNum倍音まで生成（奇数倍音のみなので2ずつ増加）
			if (j % 4 == 1) pOutBuf[i] += 1.f / (j*j) * _SIN(j*i);	// j=1,5,9,13,...
			else		 pOutBuf[i] -= 1.f / (j*j) * _SIN(j*i);	// j=3,7,11,15,...
		}
		pOutBuf[i] *= 8.f / (PI*PI);
	}
}

// ノコギリ波テーブル生成
static void makeSawTable(SIGREAL* pOutBuf, int harmoNum) {
	for (int i = 0; i<WT_SIZE; i++) {
		pOutBuf[i] = 0;
		for (int j = 1; j <= harmoNum; j++) {
			pOutBuf[i] += 1.f / j*_SIN(j*i);
		}
		pOutBuf[i] *= 2.f / PI;
	}
}

// 矩形波テーブル生成
static void makeSquTable(SIGREAL* pOutBuf, int harmoNum) {
	for (int i = 0; i<WT_SIZE; i++) {
		pOutBuf[i] = 0;
		for (int j = 1; j <= harmoNum; j++) {
			pOutBuf[i] += 1.f / j*_SIN(j*i);
			pOutBuf[i] -= 1.f / j*_SIN(j*(i + WT_SIZE / 2));
		}
		pOutBuf[i] *= 1.f / PI;
	}
}

// 波形テーブルの初期化（生成）
static void initWaveTable() {
	static bool g_bFirst = true;

	if (!g_bFirst) return;
	g_bFirst = false;

	// サイン波テーブル生成
	for (int i = 0; i<WT_SIZE; i++) {
		g_sinTable[i] = sin(2.f*PI*i / WT_SIZE);
	}

	// 波形テーブル情報表（30Hz～22.05KHzを複数の帯域に分割しそれぞれの帯域の情報を記した表）を計算する。
	// ※この計算方法では、入力パラメタとして90分割を与えた場合、68分割に最適化される
	g_tblInfoNum = 0;
	int oldHarmoNum = -1;
	int j = 0;
	float minFreq = 30.f;	// 最低周波数を30Hzとする。これを下回る発振周波数では、倍音不足になる。
	// ただし、テーブルサイズが小さくても倍音不足になるので、要注意。
	float fs = 44100.f;	// fsは44.1Kとする。
	// ※fsが48Kや96Kの場合にここで求めた表と波形テーブルをそのまま使うと、
	// 倍音不足になる。本来は、fsが変わったら、本表および、波形テーブルは再生成が望ましい。
	// が、おそらく音は殆ど変わらないと思われる。
	for (int i = 0; i<90; i++) {
		// テーブル情報を計算
		g_tblInfo[j].freq = minFreq * pow(fs / 2 / minFreq, (float)i / 90);
		g_tblInfo[j].harmoNum = (int)(fs / 2 / g_tblInfo[j].freq);
		g_tblInfo[j].offset = j*WT_SIZE;
		// 前回と同じ倍音数かどうかチェック
		if (g_tblInfo[j].harmoNum == oldHarmoNum) continue;	// 同じなので処理スキップ
		oldHarmoNum = g_tblInfo[j].harmoNum;
		// 対応周波数を求めなおし、noteNoを求める
		g_tblInfo[j].freq = fs / 2 / g_tblInfo[j].harmoNum;
		g_tblInfo[j].noteNo = 69.f + 12.f*log(g_tblInfo[j].freq / 440.f) / log(2.f);
		edPrintf("freq=%f noteNo=%f,baion=%d\n", g_tblInfo[j].freq, g_tblInfo[j].noteNo, g_tblInfo[j].harmoNum);

		j++;
	}
	g_tblInfoNum = j;
	edPrintf("g_tblInfoNum=%d\n", g_tblInfoNum);

	// 周波数帯域毎に、三角波、ノコギリ波、矩形波テーブルを生成
	for (int i = 0; i<g_tblInfoNum; i++) {
		SIGREAL* pTable;

		pTable = &g_triTable[0] + g_tblInfo[i].offset;
		makeTriTable(pTable, g_tblInfo[i].harmoNum);

		pTable = &g_sawTable[0] + g_tblInfo[i].offset;
		makeSawTable(pTable, g_tblInfo[i].harmoNum);

		pTable = &g_squTable[0] + g_tblInfo[i].offset;
		makeSquTable(pTable, g_tblInfo[i].harmoNum);
	}

}

/////////////////////////////////////////////////////////////////////
// 波形生成に関する基本的なルーチン集

// 波形テーブルへのポインタと、位相（16:16固定小数）を与えると、線形補間して波形値を読み出す関数
__forceinline SIGREAL _GET_WAVE(SIGREAL* pTbl, int phase) {
	// 16:16の固定小数からインデックス(=整数部)を取り出すマクロ、x:固定小数点による位相、y:インデックスオフセット
#define _IDX(x,y)	(((y)+((x)>>16))&(WT_SIZE-1))

	int idx = _IDX(phase, 0);		// 整数部の取り出し
	int idx2 = _IDX(phase, 1);		// 補間用に、idxの次のidxを求める。単純に+1すると範囲オーバーの可能性有り。
	float deci = (float)(phase & ((1 << 16) - 1)) / (1 << 16);	// 少数部の取り出し(上位16ビットをクリアし、1/65536倍する)
	return pTbl[idx] + (pTbl[idx2] - pTbl[idx]) * deci;	// 線形補完で出力
}

// NoteNo,デチューン(セント単位)から角速度を求める
static unsigned int calcWFromNoteNo(float nn, float det, float fs) {
	// NoteNo、デチューン値から発振周波数fを求める noteNo=69(A4)で440Hz
	// ※デチューン値：100セントで1NoteNo分のピッチに相当する
	float f = 440.f * pow(2.f, (nn + det / 100.f - 69.f) / 12.f);
	// 角速度(１周期＝WT_SIZEの16:16固定小数点表現)へ変換
	return (unsigned int)(WT_SIZE*(1 << 16)*f / fs);
}

// NoteNoから最適な（＝倍音がナイキストを超えない）波形テーブルを求める
static SIGREAL* getWTFromNoteNo(int waveFormType, float nn) {
	if (waveFormType == WF_SINE) {
		return &g_sinTable[0];
	}
	else {
		// テーブル情報表を頭からサーチ。
		// もしパフォーマンスが問題になる場合は、nnを適当に整数化したものとoffsetのマップテーブルを使う。
		SIGREAL* pTbl = (waveFormType == WF_TRI) ? &g_triTable[0] : ((waveFormType == WF_SAW) ? &g_sawTable[0] : &g_squTable[0]);
		for (int i = 0; i<g_tblInfoNum; i++) {
			if (nn <= g_tblInfo[i].noteNo) {
				return pTbl + g_tblInfo[i].offset;
			}
		}
	}
	return &g_sinTable[0];
}


/////////////////////////////////////////////////////////////////////
// VCOクラス
CVCO::CVCO() {
	initWaveTable();
	curNotesNum = notesNum = portaStartNotesNum = 0.f;
	phase = 0;
	phase2 = 0.f;
	waveFormType = WF_SINE;
	curPortaTime = 1.f;
	portaTimeDelta = 0.f;
	detuneCent = 0.f;	// ボイス間デチューン値（単位はセント）
}

CVCO::~CVCO() {
}

// ボイス間デチューン値(単位はセント)の設定
void CVCO::setDetune(float det) {
	detuneCent = det;
}

// 波形種類設定
void CVCO::setWF(int waveFormType) {
	waveFormType = waveFormType % 4;	// WF_SINE(=0)～WF_SQUARE(=3)
}

// ポルタメント時間設定
void CVCO::setPortaTime(float fVal) {
	// 0秒～3秒。0秒でポルタメントオフ。つまみ位置(fVal)に対して指数関数的に変化するように。
	float t = exp((fVal - 1) * 3)*fVal * 3;
	// ポルタメント時間に応じた、portaTimeDeltaを計算
	if (fVal == 0.f)	portaTimeDelta = 0.f;	// ポルタメントなし
	else			portaTimeDelta = 1.f / (mfs*t);
}

// 発振NoteNoの設定
void CVCO::setNoteNo(int nn, bool isKeyOn) {
	// ポルタメント関連の変数処理
	if (curPortaTime<1.f)		portaStartNotesNum = curNotesNum;	// ポルタメント中→開始NoteNoはポルタメント中のNoteNo
	else						portaStartNotesNum = notesNum;		// 定常状態→開始NoteNoは今までのノートNo
	// ポルタメントが必要かどうかを判定
	// NordLead2やSynth1の"Auto"なポルタメント動作とする。
	// これを止め、常にポルタメントするようにするには、開始条件からisKeyOnを削除する。
	if (portaStartNotesNum != nn && portaTimeDelta != 0.f
		&& isKeyOn)	curPortaTime = 0.f;	// ポルタメント開始
	else							curPortaTime = 1.f;	// ポルタメント不要
	// 新しいノートNoを保持
	notesNum = (float)nn;
}

// 信号処理
void CVCO::processSignal(int size, SIGREAL* pSig, bool* pProcessed) {
#if 1
	// 波形テーブルと固定小数点位相変数と線形補間で波形生成する処理→実測約19clk/sample（定常状態）

	// Synth1と同じ、指数関数風のポルタメントカーブ(x=0～1,y=0～1)
#define		PORTAMENT_CURVE(x)		(1.f-exp(-x*4.f)*(1.f-x))

	int p = phase;	// パフォーマンス向上のため、クリティカルなループ内では位相等のメンバ変数を直接更新/参照しない
	while (size > 0) {
		int processSize;
		int w;
		SIGREAL* pTbl;
		if (curPortaTime >= 1.f) {
			// 定常状態（ポルタメント中ではない）
			// 発振ノートNoに応じた、角速度wと波形テーブルを求める。
			w = calcWFromNoteNo(notesNum, detuneCent, mfs);
			pTbl = getWTFromNoteNo(waveFormType, notesNum);
			processSize = size;
		}
		else {
			// ポルタメント中。発振周波数を連続的に変化させる。
			// 発振ノートNoを求める
			curNotesNum = portaStartNotesNum + (notesNum - portaStartNotesNum) * PORTAMENT_CURVE(curPortaTime);
			// 発振ノートNoに応じた、角速度wと波形テーブルを求める。
			w = calcWFromNoteNo(curNotesNum, detuneCent, mfs);
			pTbl = getWTFromNoteNo(waveFormType, curNotesNum);
			// 1サンプル毎にwを再計算するのが、音質的には理想だが、パフォーマンスが悪い。
			// ここでは、複数サンプル毎にしか計算しないようにする。
			// サンプル数は、ポルタメントカーブがy=xな直線と仮定し(実際は指数関数)、
			// ピッチの解像度が5セント(適当)になるように決定する。
			// 本来はポルタメントカーブの逆関数から求めた方がいいが、面倒なので省略。
			// ただし、サンプル数が小さすぎると負荷になるため、最低24サンプルとする。
			// 音質について：自分の耳では1サンプル毎にw再計算の音と判別不能。
			float totalCent = fabs(notesNum - portaStartNotesNum)*100.f;
			int totalPortaSample = (int)(1.f / portaTimeDelta);
			processSize = MAX(24, (int)(totalPortaSample / totalCent / 5.f));
			processSize = MIN(size, processSize);
			curPortaTime += portaTimeDelta*processSize;	// ポルタメント経過時間を加算
		}

		// 波形読み出し。
		for (int i = 0; i<processSize; i++) {
			*pSig++ = _GET_WAVE(pTbl, p);
			p += w;		// 位相を進める
		}
		size -= processSize;
	}
	phase = p;		// ローカル位相変数をメンバ変数へ格納

#else
	// sin()と浮動小数点位相変数で波形生成するベーシックな処理→実測約77clk/sample
	// 上の波形テーブル方式とのパフォーマンス比較用ルーチン。ポルタメント実装なし。
	// コードはシンプルだが、波形テーブル方式に比べて4倍パフォーマンスが悪い。
	float p = mpp;
	float f = 440.f * pow(2.f, (notesNum + detuneCent / 100.f - 69) / 12.f);
	float w = 2 * PI*f / mfs;		// 角速度(１周期＝２πの浮動小数点)
	for (int i = 0; i<size; i++) {
		pSig[i] = sin(p);
		p += w;					// 位相を進める
		if (p > 2.f*PI) p -= 2.f*PI;	// 位相範囲チェック
	}
	mpp = p;
#endif

	*pProcessed = true;
}

/////////////////////////////////////////////////////////////////////
// ボイスクラス
CMyVoice::CMyVoice() {
	notesNum = velo = 0;
	bKeyOn = false;
}

CMyVoice::~CMyVoice() {
}

// キーオン中かどうかを返す
bool CMyVoice::isKeyOn(void) {
	return bKeyOn;
}

// 発音中かどうかを返す。
bool CMyVoice::isBusy(void) {
	return bKeyOn;	// エンベロープを実装していないので、発音状態はキーのオンオフ状態と同じ
}

// デチューン値設定
void CMyVoice::setDetune(float det) {
	// デチューン値の影響を受けるのは、VCOなので、VCOにデチューン値設定
	vco.setDetune(det);
}

// 発振ノートNo等の設定。キーオン毎にコールされる。
void CMyVoice::setNoteInfo(int nn, int velo) {
	notesNum = nn;
	velo = velo;
	vco.setNoteNo(nn, isKeyOn());
}

// トリガー通知（レガートモード(KM_MONO2)でレガート奏法を行った時には、コールされない）
void CMyVoice::trigger(void) {
	bKeyOn = true;
	edPrintf("VoiceNo[%d] trigger\n", mno);
}

// キーオフ通知
void CMyVoice::release(int velo) {
	bKeyOn = false;
	edPrintf("VoiceNo[%d] release\n", mno);
}

// 現在のノートNoを返す
int CMyVoice::getNoteNo(void) {
	return notesNum;
}

// 信号処理(モノラル)
void CMyVoice::processSignal(int size, SIGREAL* pSig, bool* pProcessed) {
	// VCOに信号生成を行わせる
	vco.processSignal(size, pSig, pProcessed);
}

// サンプリング周波数が通知される
void CMyVoice::changedSamplingFreq(void) {
	vco.setFs(mfs);
}

/////////////////////////////////////////////////////////////////////
// シンセ本体クラス
CMySynthMain::CMySynthMain() {
	vc.setKeyboardInfo(&kbdInfo);	// キーボード情報管理クラスをボイスコントローラへ登録
	vc.setVoiceNum(VOICE_NUM);		// ボイス数の設定
	for (int i = 0; i<VOICE_NUM; i++) vc.setVoice(i, &mVoice[i]);	// ボイスコントローラーへボイスを登録

	// 128プログラム初期化
	for (int i = 0; i<PROGRAM_NUM; i++) {
		// 値に整数をとるパラメタは、固定的に128で割って、0～1以内に収める
		patchInfo[i].soundParam[SPID_MASTER_VOL] = 0.1f;			// マスターボリュームノブ位置
		patchInfo[i].soundParam[SPID_VCO_WF] = WF_TRI / 128.f;		// ウェーブフォーム
		patchInfo[i].soundParam[SPID_VCO_PORTA_TIME] = 0.2f;		// ポルタメントタイムノブ位置
		patchInfo[i].soundParam[SPID_VOICE_MODE] = KM_MONO / 128.f;// ボイスモード
		patchInfo[i].soundParam[SPID_UNISON_NUM] = 4 / 128.f;		// ユニゾンボイス数
		strcpy(&patchInfo[i].programName[0], "init");
	}
	setProgramNo(0);
}

CMySynthMain::~CMySynthMain() {
}

// テンポ情報設定(テンポ同期する機能を実装する時に使う)
void CMySynthMain::setBaseTempo(float t) {
}

// MIDIノートオフ受信
void CMySynthMain::receiveNoteOff(int nn, int v) {
	kbdInfo.offKey(nn);
}

// MIDIノートオン受信
void CMySynthMain::receiveNoteOn(int nn, int v) {
	kbdInfo.onKey(nn, v);
}

// MIDIダンパーコントロール受信
void CMySynthMain::receiveDumperPedal(bool b) {
	kbdInfo.setDumper(b);
}

// サウンドパラメタ(パッチパラメタ)値の受信
void CMySynthMain::setSoundParam(int id, float fVal) {
	// 0.f～1.fのVST準拠のパラメタ値を0～127の整数に変換
	int iVal = MAX(0, MIN(127, (int)(fVal*128.f)));
	edPrintf("setSoundParam id=%d,val=%f\n", id, fVal);
	// 受信したパラメタに応じて、各部品に設定を行う。
	switch (id) {
		case SPID_MASTER_VOL:		masterVol = fVal;													break;
		case SPID_VCO_WF:			for (int i = 0; i<VOICE_NUM; i++) mVoice[i].vco.setWF(iVal);			break;
		case SPID_VCO_PORTA_TIME:	for (int i = 0; i<VOICE_NUM; i++) mVoice[i].vco.setPortaTime(fVal);	break;
		case SPID_VOICE_MODE:		vc.setVoiceMode(iVal);											break;
		case SPID_UNISON_NUM:		vc.setUnisonVoiceNum(iVal);										break;
	}
	// パラメタ値の保持
	if (id >= 0 && id < SOUND_PARAM_NUM) mCurSoundParam[id] = fVal;
}

// サウンドパラメタ名を返す
void CMySynthMain::getSoundParamName(int id, char* rtn) {
	static char* names[] = { "Master Volume", "Wave Form", "Portament Time", "Voice Mode", "Unison Voice Num" };
	if (id >= 0 && id < SOUND_PARAM_NUM)
		strcpy(rtn, names[id]);
	else
		rtn[0] = '\0';
}

// サウンドパラメタ値の表示テキストを返す
void CMySynthMain::getSoundParamValText(int id, char* rtn) {
	static char* strWf[] = { "SINE", "TRIANGLE", "SAW", "PULSE" };
	static char* strVoiceMode[] = { "POLY", "MONO1", "MONO2" };

	if (id >= 0 && id < SOUND_PARAM_NUM) {
		int iVal = MAX(0, MIN(127, (int)(mCurSoundParam[id] * 128.f)));
		switch (id) {
			case SPID_VCO_WF:		sprintf(rtn, "%s", strWf[iVal % 4]);				break;
			case SPID_VOICE_MODE:	sprintf(rtn, "%s", strVoiceMode[iVal % 3]);			break;
			case SPID_UNISON_NUM:	sprintf(rtn, "%d", MIN(VOICE_NUM, MAX(1, iVal)));	break;
			default:				sprintf(rtn, "%.2f", mCurSoundParam[id]);		break;
		}
	}
	else {
		rtn[0] = '\0';
	}
}

// カレントプログラムをチェンジする
void CMySynthMain::setProgramNo(int no) {
	edPrintf("setProgramNo no=%d\n", no);
	mCurSoundParam = &patchInfo[no].soundParam[0];
	// 新しいプログラムのパラメタを各部品に設定する
	for (int id = 0; id<SOUND_PARAM_NUM; id++) setSoundParam(id, mCurSoundParam[id]);
}

// シンセ音の出力(ステレオ)
void CMySynthMain::processSignalLR(int size, SIG_LR* pSig, bool* pProcessed) {
	_START_CLOCK();		// CPUクロック計測開始
	// 鍵盤状態が変更されていれば、ボイスのトリガー/リリース処理をボイスコントローラーに処理させる
	if (kbdInfo.isStatusChanged()) {
		vc.controlVoiceTrigger();		// トリガー/リリース処理
		kbdInfo.resetStatusChange();	// 鍵盤状態変更フラグを落とす
	}

	// 各ボイスの信号処理を行いステレオMIXする（ボイスコントローラーの仕事）。
	vc.processSignalLR(size, pSig, pProcessed);

	// マスターボリューム処理
	if (*pProcessed) {
		float vol = masterVol;
		for (int i = 0; i<size; i++) {
			pSig[i].l *= vol;
			pSig[i].r *= vol;
		}
	}
	_DISP_CLOCK(size);	// CPUクロック計測終了・ディスプレイ上部に表示
}

// サンプリング周波数が通知される
void CMySynthMain::changedSamplingFreq() {
	vc.setFs(mfs);
	for (int i = 0; i<VOICE_NUM; i++) mVoice[i].setFs(mfs);
}

