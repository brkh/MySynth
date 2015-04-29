#pragma once

// MySynth.h
// MySynthクラス。VSTiプラグインのメインクラスとでも言うべきクラス。
// ただし、シンセ本体はCMySynthMainクラスに分離しているので、通常は変更の必要なし。

class CMySynth : public AudioEffectX {
public:
	CMySynth(audioMasterCallback audioMaster);
	virtual ~CMySynth(void);

	//// AudioEffectXからオーバーライド
	// イベント処理
	VstInt32 canDo(char* text);
	VstInt32 processEvents(VstEvents* events);

	// 信号処理
	void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);
	void setSampleRate(float sampleRate);

	// プロパティ関連
	bool getEffectName(char* name);
	bool getVendorString(char* text);
	bool getProductString(char* text);
	VstInt32 getVendorVersion(void);

	// パラメータ関連
	void setParameter(VstInt32 index, float value);
	float getParameter(VstInt32 index);
	void getParameterName(VstInt32 index, char* text);
	void getParameterDisplay(VstInt32 index, char* text);

	// プログラム関連
	bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text);
	void getProgramName(char* name);
	void setProgramName(char* name);
	void setProgram(VstInt32 program);
	VstInt32 getChunk(void** data, bool isPreset);
	VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset);

private:
	CMySynthMain*	mSynth;			// シンセ本体クラス
	//VstEvent	*mEvents;				// イベントバッファ
	vector<VstEvent>	mEvents;
	//int			meventMaxNum;			// バッファに格納可能な最大イベント数
	//int			meventNum;				// バッファに格納されているイベント数
	int			mCurrentEventNo;		// 現在処理中のイベントNo
	int			mCurrentSample;		// 現在処理中のサンプルNo
};

