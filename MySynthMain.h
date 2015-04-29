#pragma once
// MySynthMain.h
// シンセ本体クラス及び、そのコンポーネントとなるクラス

////////////////////////////////////////////////////////////
// VCO(オシレーター)クラス
#define		WF_SINE		0	// サイン波
#define		WF_TRI		1	// 三角波
#define		WF_SAW		2	// 鋸波
#define		WF_SQUARE	3	// 矩形波

class CVCO : public CSignalProcessor {
public:
	CVCO();
	virtual ~CVCO();

	// デチューン値設定
	void setDetune(float det);

	// パラメタ値設定
	void setWF(int waveFormType);				// 波形種類
	void setPortaTime(float fVal);	// ポルタメントタイム

	// 発振ノート番号設定
	void setNoteNo(int nn, bool isKeyOn);

	// モノラル信号処理(CSignalProcessorから継承)
	void processSignal(int size, SIGREAL* pSig, bool* pProcessed);

private:
	// 波形生成に直接かかわる変数
	float notesNum;				// 発振指定されたノートNo
	float curNotesNum;			// 現在発振中のノートNo（ノートNoを小数にする事でポルタメント中のノートNoを表現する）
	float portaStartNotesNum;	// ポルタメント開始時のnoteNo
	float curPortaTime;	// ポルタメント経過時間（1で正規化、0～1でポルタメント中）
	float portaTimeDelta;	// ポルタメント速度
	float detuneCent;		// ボイス間デチューン値（単位はセント）
	unsigned int phase;		// 位相(固定小数点)
	float phase2;				// 位相(浮動小数点)、比較用
	int waveFormType;				// 波形種類
};


////////////////////////////////////////////////////////////
// ボイスクラス
class CMyVoice : public IVoice {
public:
	CMyVoice();
	~CMyVoice();

	// ボイス間デチューン(IVoiceから継承)
	void setDetune(float det);

	// トリガー関連処理(IVoiceから継承)
	void setNoteInfo(int nn, int velo);
	void trigger(void);
	void release(int velo);
	int  getNoteNo(void);
	bool isKeyOn(void);
	bool isBusy(void);

	// 信号処理(CSignalProcessorから継承)
	void processSignal(int size, SIGREAL* pSig, bool* pProcessed);
	void changedSamplingFreq(void);

	// 外部公開オブジェクト
	CVCO vco;		// VCO

private:
	int notesNum;		// 現在のノートNO
	int velo;		// トリガー時のベロシティ値
	bool bKeyOn;	// キーが押されているかどうか
};


////////////////////////////////////////////////////////////
// シンセ本体クラス及び、パラメタ構造等の定義

// シンセの基本定数
#define	VOICE_NUM			16		// 16ボイスとする
#define	SOUND_PARAM_NUM		5		// サウンドパラメタの種類数
#define	PROGRAM_NUM			128		// 128プログラム

// サウンドパラメタID一覧
#define	SPID_MASTER_VOL			0		// マスターボリューム
#define	SPID_VCO_WF				1		// VCOウェーブフォーム
#define	SPID_VCO_PORTA_TIME		2		// VCOポルタメントタイム
#define	SPID_VOICE_MODE			3		// ボイスモード
#define	SPID_UNISON_NUM			4		// ユニゾンボイス数

// １つのパッチ情報を格納する構造
typedef struct tagPatchInfo {
	char programName[25];					// プログラム名(24文字+終端)
	float soundParam[SOUND_PARAM_NUM];	// サウンドパラメタ保持領域
} PATCHINFO;

// シンセ本体クラス
class CMySynthMain : public CSignalProcessor, public CMIDIInDev {
public:
	CMySynthMain::CMySynthMain();
	virtual ~CMySynthMain(void);

	// MIDIイベント等のハンドラ(CMIDIInDevから継承)
	void receiveNoteOff(int nn, int v);
	void receiveNoteOn(int nn, int v);
	void receiveDumperPedal(bool b);
	void setBaseTempo(float t);

	// パラメタ関連
	void setSoundParam(int id, float fVal);		// サウンドパラメタ値の受信
	void getSoundParamName(int id, char* rtn);	// サウンドパラメタ名の返信
	void getSoundParamValText(int id, char* rtn);// サウンドパラメタ値に対応するテキスト返信

	// プログラム関連
	void setProgramNo(int no);	// プログラム(パッチ)の変更要求

	// 信号処理(CSignalProcessorから継承)
	void processSignalLR(int size, SIG_LR* pSig, bool* pProcessed);
	void changedSamplingFreq(void);

	// パッチ情報（公開オブジェクト）
	PATCHINFO patchInfo[PROGRAM_NUM];

private:
	CKeyboardInfo kbdInfo;		// 鍵盤情報管理クラス
	CVoiceCtrl vc;					// ボイスコントローラー
	CMyVoice mVoice[VOICE_NUM];	// ボイス
	float masterVol;				// マスターボリューム
	float* mCurSoundParam;			// カレントサウンドパラメタ（へのポインタ）
};

