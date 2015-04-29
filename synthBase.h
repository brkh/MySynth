#pragma once
// synthBase.h
// シンセ本体に必要となる、MIDI解釈、ボイスコントロール制御など、面倒な事を
// やってくれるクラス達


/////////////////////////////////////////////////////////////////////////
// 信号処理のベースクラス
// サンプリング周波数の保持や、信号出力I/Fを定義
class CSignalProcessor {
public:
	CSignalProcessor() { mfs = 44100.f; };

	// サンプリング周波数関連
	void setFs(float fs) { mfs = fs; changedSamplingFreq(); };	// サンプリング周波数設定
	virtual void changedSamplingFreq(void) {};				// その変更通知

	// 信号処理（モノラル）
	virtual void processSignal(int size, SIGREAL* pSig, bool* pProcessed) {};

	// 信号処理（ステレオ）
	virtual void processSignalLR(int size, SIG_LR* pSig, bool* pProcessed) {
		// デフォルト実装(モノラル処理して、ステレオデータに拡張する)の定義
		// まずモノラル処理を実行
		processSignal(size, (SIGREAL*)pSig, pProcessed);
		if (*pProcessed) {
			// モノラルデータ→ステレオデータへ拡張
			// d,sにSIGREAL*でなく、int*を使っているのはFPUを介在させないため。
			// 万が一デノーマル数がFPUへロードされるとトラブルになる。また、FPU経由では若干遅くなる可能性がある。
			int* s = (int*)pSig;
			int* d = s + (size - 1) * 2;
			for (int i = size - 1; i >= 0; i--, d -= 2) *d = *(d + 1) = s[i];
		}
	}

protected:
	float mfs;		// サンプリング周波数
};


/////////////////////////////////////////////////////////////////////
// MIDI入力解釈クラス
class CMIDIInDev {
public:

	CMIDIInDev(void) {
		mprogramBankNo = mNRPNNo = mRPNNo = mparamData = 0;
	};

	virtual ~CMIDIInDev() {};

	// MIDI受信通知
	// チャンネルボイスメッセージ
	virtual void receiveNoteOff(int nn, int v) {};
	virtual void receiveNoteOn(int nn, int v) {};
	virtual void receivePolyKeyPressure(int nn, int v) {};
	// チャンネルボイスメッセージ(コントロールチェンジ)
	virtual bool processMIDICCOriginal(BYTE st, BYTE dt1, BYTE dt2) { return false; };
	virtual void receiveModulationWheel(int v) {};
	virtual void receiveVolume(int v) {};
	virtual void receiveDumperPedal(bool b) {};
	virtual void receiveNRPNData(int paramNo, int data, bool bHIByte) {};
	virtual void receiveRPNData(int paramNo, int data, bool bHIByte) {};
	// チャンネルモードメッセージ
	virtual void receiveAllSoundOff(void) { receiveAllNoteOff(); };
	virtual void receiveResetAllController(void) {
		receiveModulationWheel(0);
		receivePitchBend(0);
	};
	virtual void receiveAllNoteOff(void) {
		receiveDumperPedal(false);
		for (int i = 0; i<128; i++)receiveNoteOff(i, 0);
	};
	virtual void receiveProgramChange(int progNo, int bankNo) {};
	virtual void receiveChannelPressure(int v) {};
	virtual void receivePitchBend(int v) {};

	// MIDIショートメッセージ受信処理
	void processMIDIShortData(BYTE st, BYTE dt1, BYTE dt2) {
		int const nKind = st & 0xF0;
		int ch = st & 0x0F;
		switch (nKind) {
			case 0x80:		// ノートオフ
			_NOTEOFF :
				if (dt1 < 0 || dt1 > 127) break;
					 receiveNoteOff(dt1, dt2);
					 break;

			case 0x90:		// ノートオン
				if (dt2 == 0) goto _NOTEOFF;
				if (dt1 < 0 || dt1 > 127) break;
				receiveNoteOn(dt1, dt2);
				break;

			case 0xA0:		// ポリフォニックキープレッシャー
				receivePolyKeyPressure(dt1, dt2);
				break;

			case 0xB0:
				if (dt1<120) {	// コントロールチェンジ
					//edPrintf("コントロールチェンジdt1,dt2=%02d,%02d\n",dt1,dt2);
					// 独自のCCアサイン処理
					bool rtn = processMIDICCOriginal(st, dt1, dt2);
					if (rtn == true) break;

					// デフォルトのCC処理
					switch (dt1) {
						case 0x00:		// プログラムバンク選択（上位）
							mprogramBankNo = MAKEWORD(LOBYTE(mprogramBankNo), dt2);
							break;

						case 0x01:		// モジュレーションホイール
							receiveModulationWheel(dt2);
							break;

						case 0x06:		// データエントリー（上位）
							setDataEntryHI(dt2);
							break;

						case 0x07:		// ボリューム
							receiveVolume(dt2);
							break;

						case 0x20:		// プログラムバンク選択（下位）
							mprogramBankNo = MAKEWORD(dt2, HIBYTE(mprogramBankNo));
							break;

						case 0x26:		// データエントリー（下位）
							setDataEntryLO(dt2);
							break;

						case 0x40:		// ダンパーペダル
							receiveDumperPedal(dt2 >= 64);
							break;

						case 0x62:		// ＮＲＰＮパラメタ選択(下位)
							mNRPNNo = MAKEWORD(LOBYTE(mNRPNNo), dt2);
							mRPNNo = -1;
							break;

						case 0x63:		// ＮＲＰＮパラメタ選択(上位)
							mNRPNNo = MAKEWORD(LOBYTE(mNRPNNo), dt2);
							break;

						case 0x64:		// ＲＰＮパラメタ選択(下位)
							mRPNNo = MAKEWORD(dt2, HIBYTE(mRPNNo));
							mNRPNNo = -1;
							break;

						case 0x65:		// ＲＰＮパラメタ選択(上位)
							mRPNNo = MAKEWORD(dt2, HIBYTE(mRPNNo));
							break;
					}

				}
				else {		// チャネルモードメッセージ
					switch (dt1) {
						case 0x78:		// オールサウンドオフ
							receiveAllSoundOff();
							break;

						case 0x79:		// リセットオールコントローラ
							receiveResetAllController();
							break;

						case 0x7B:		// オールノートオフ
							receiveAllNoteOff();
							break;
					}
				}
				break;

			case 0xC0:		// プログラムチェンジ
				receiveProgramChange(dt1, mprogramBankNo);
				break;

			case 0xD0:		// チャンネルプレッシャー
				receiveChannelPressure(dt1);
				break;

			case 0xE0:		// ピッチベンド
				//edPrintf("dt1,dt2=%02d,%02d\n",dt1,dt2);
				receivePitchBend(dt2 * 128 + dt1 - 8192);
				break;

			case 0xF0:		// システムメッセージ
				//edPrintf("システムメッセージ st=%02x,dt1,dt2=%02x,%02x\n",st,dt1,dt2);
				break;
		}

	};

private:
	int mprogramBankNo;
	int mNRPNNo;
	int mRPNNo;
	int mparamData;

	void setDataEntryHI(BYTE dt2) {
		mparamData = MAKEWORD(LOBYTE(mparamData), dt2);
		if (mNRPNNo != -1)	receiveNRPNData(mNRPNNo, mparamData, true);
		else				receiveRPNData(mRPNNo, mparamData, true);
	};

	void setDataEntryLO(BYTE dt2) {
		mparamData = MAKEWORD(dt2, HIBYTE(mparamData));
		if (mNRPNNo != -1)	receiveNRPNData(mNRPNNo, mparamData, false);
		else				receiveRPNData(mRPNNo, mparamData, false);
	};

};


///////////////////////
// 鍵盤情報管理クラス
// ボイスのトリガー/リリースをコントロールするために、押された鍵盤の順序等を記憶、管理するクラス
class CKeyboardInfo {
public:
	CKeyboardInfo() {
		mOnKeyNum = 0;
		bStatusChanged = bDumper = false;
		for (int i = 0; i<128; i++) mKeyTable[i] = mDumperTable[i] = 0;
	};
	virtual ~CKeyboardInfo() {};
	// キー打鍵
	void onKey(int nn, int v) {
		if (mKeyTable[nn] != 0) offKeyImpl(nn);
		mKeyTable[nn] = v;
		mDumperTable[nn] = false;
		mOnKeyNNList[mOnKeyNum] = nn + 256;	// +256は新規押鍵フラグ
		mOnKeyNum++;
		bStatusChanged = true;
	};
	// キー離した
	void offKey(int nn) {
		if (bDumper) mDumperTable[nn] = true;
		else offKeyImpl(nn);
	};
	// ダンパーペダルの踏み/離し
	void setDumper(bool v) {
		bDumper = v;
		if (!bDumper) {
			for (int i = 0; i<128; i++) {
				if (mDumperTable[i]) {
					mDumperTable[i] = false;
					offKeyImpl(i);
				}
			}
		}
	};
	// n番目に新しい押鍵キーのノートNOを取得。なければ、-1を返す。
	int getOnKeyNN(int n) {
		if (n >= mOnKeyNum) return -1;
		return mOnKeyNNList[mOnKeyNum - 1 - n];
	};
	// n番目に新しい新規押鍵キーのノートNOを取得。なければ、-1を返す。
	int getNewOnKeyNN(int n) {
		if (n >= mOnKeyNum) return -1;
		int noteNo = mOnKeyNNList[mOnKeyNum - 1 - n];
		if (noteNo < 256) return -1;		// 新規ではない
		return noteNo - 256;
	};
	// 鍵盤状態変更フラグリセット
	void resetStatusChange(void) {
		bStatusChanged = false;
		// 新規押鍵フラグもすべて落とす
		for (int i = 0; i<mOnKeyNum; i++) if (mOnKeyNNList[i] >= 256) mOnKeyNNList[i] -= 256;
	};
	// キーオンされているキーの数を返す
	int getOnKeyNum(void) { return mOnKeyNum; };
	// 最後にコールしたresetStatusChange()から鍵盤状態が変更されているか？を返す
	bool isStatusChanged(void) { return bStatusChanged; };
	// 押下時のベロシティ値を返す
	int getVelocity(int nn) { return mKeyTable[nn]; };

private:
	int mOnKeyNum;				// 押下中のキー数
	int mKeyTable[128];		// キーとベロシティとの対応表
	int mOnKeyNNList[128];		// 押下されたキーの順番を保持するリスト
	bool bDumper;				// ダンパーペダルフラグ
	bool mDumperTable[128];	// ダンパーオフで、リリースすべき鍵盤情報
	bool bStatusChanged;		// キーの押下状態が変更されたかのフラグ

	// キーオフ時に使用するサブルーチン
	void offKeyImpl(int nn) {
		mKeyTable[nn] = 0;
		bStatusChanged = true;
		// オンキーリストから、キーオフした鍵盤情報を取り除く
		for (int i = 0; i<mOnKeyNum; i++) {
			int onNN = mOnKeyNNList[i];
			if (nn == ((onNN >= 256) ? onNN - 256 : onNN)) {
				for (; i<mOnKeyNum - 1; i++) mOnKeyNNList[i] = mOnKeyNNList[i + 1];
				mOnKeyNum--;
				return;
			}
		}
	};
};


///////////////////////
// ボイス(Interface)
class IVoice : public CSignalProcessor {
public:
	IVoice() { mno = 0; mpan = 0.5f; };
	void setNo(int no) { mno = no; };				// ボイスを識別するためのボイス番号(連番)を設定
	int getNo(void) { return mno; };				// ボイス番号を返す
	virtual void setDetune(float det) {};		// デチューン値設定、単位はセント
	virtual void setPan(float pan) { mpan = pan; };	// パン値設定
	virtual float getPan(void) { return mpan; };	// パン値を返す
	virtual void setNoteInfo(int nn, int velo) {};// 発振ノートNo等の設定。キーオン毎にコールされる。
	// ユニゾンボイスの為の情報設定。ボイス間で位相を制御する時の為用。キーオン毎にコールされる。
	virtual void setUnisonInfo(IVoice* pMasterVoice, int unisonNum, int unisonNo) {};
	virtual void trigger(void) {};				// キートリガー通知
	virtual void release(int velo) {};			// キーリリース通知
	virtual int  getNoteNo(void) { return 0; };	// 発振ノートNoを返す
	virtual bool isKeyOn(void) { return false; };	// キーオン中かを返す
	// ボイスがビジー(発音中)かどうかを返す
	// isKeyOn()が純粋なキーのON/OFFフラグなのに対し、isBusy()は現在発音しているかどうかを返す。
	// キーオフ状態でisKeyOn()はfalseを返すような状況でも、リリース音が発音されている状態なら、isBusy()はtrueを返す。
	virtual bool isBusy(void) { return false; };

protected:
	int mno;		// ボイス番号
	float mpan;	// ボイスパン値(0=left ... 0.5=center ... 1=right)
};



///////////////////////////////////////////////////
// ボイスコントローラー
// ボイスを管理下において、キーモードに応じて、トリガー/リリース等を実行する。
#define KM_POLY			0	// ポリ
#define KM_MONO			1	// モノ
#define KM_MONO2		2	// レガート

class CVoiceCtrl : public CSignalProcessor {
public:
	CVoiceCtrl();

	void setKeyboardInfo(CKeyboardInfo* p);	// キーボード情報管理クラスの設定
	void setVoiceNum(int n);				// 管理するボイス数の設定
	void setVoice(int no, IVoice* pV);		// 管理する各ボイスの設定
	void setVoiceMode(int mode);			// キーモードの設定
	void setUnisonVoiceNum(int n);			// ユニゾンボイス数の設定
	void controlVoiceTrigger(void);			// トリガー/リリースを制御する
	// 信号処理（ボイスの信号処理を行い、ステレオMIXする）
	void processSignalLR(int size, SIG_LR* pSig, bool* pProcessed);

private:
	int iKeyMode;				// キーモード
	CKeyboardInfo* kbdInfo;	// キーボード管理クラス
	int mVoiceNum;				// 登録されたボイスの数
	int mPolyNum;				// ポリモード時の最大ボイス数
	int mUnisonNum;			// ユニゾンボイス数

	IVoice* mpVoices[128];			// 管理するボイスたち
	int iCurVoiceNo;				// カレントボイス番号
	std::list<IVoice*> monVoices;	// キーオン中のボイスリスト
	int mmonoCurrentVelocity;		// モノモード時に使うワーク用ベロシテシティ値
	IVoice* getNextOffVoice(void);	// 空きボイスのサーチ
	void controlVoicePoly(void);	// ポリモードでのトリガー/リリース制御
	void controlVoiceMono(void);	// モノモードでのトリガー/リリース制御
};