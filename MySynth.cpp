// MySynth.cpp
// MySynthクラスの定義。VSTiプラグインメインクラスとでも言うべきクラス。
// ただし、このクラスはラッパーのようなものなので、通常は、★に示した一部の識別情報を除いて
// 修正する必要はない。


// VSTSDK、Windows、他、必要なヘッダ
#pragma warning(disable:4996)
#include <pluginterfaces/vst2.x/aeffect.h>
#include <public.sdk/source/vst2.x/audioeffectx.h>
#include <public.sdk/source/vst2.x/aeffeditor.h>
#include <windows.h>
#include <math.h>
#include <list>
#include <vector>
using namespace std;
#include "common.h"
#include "debug.h"
#include "synthBase.h"
#include "MySynthMain.h"
#include "MySynth.h"

// VSTSDK実装(普通はコンパイル単位を分けるが、面倒なのでこのようにする)
#include <public.sdk\source\vst2.x\audioeffect.cpp>
#include <public.sdk\source\vst2.x\audioeffectx.cpp>
#include <public.sdk\source\vst2.x\vstplugmain.cpp>

// VSTオブジェクトの生成関数
AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {
	// MySynthの生成
	return new CMySynth(audioMaster);
}

// CMySynthクラスコンストラクタ
CMySynth::CMySynth(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, PROGRAM_NUM, SOUND_PARAM_NUM) {
	// デバッグウィンドウのオープン()
	startDbgWin((HINSTANCE)hInstance);	// hInstanceは、vstplugmain.cppでグローバル変数として定義されている。

	// イベント関連変数の初期化
	mCurrentEventNo = mCurrentSample = 0;
	//mEvents=NULL;

	// このプラグインの各種属性を設定する
	programsAreChunks();	// 音色データは独自チャンクで保存する
	setNumInputs(0);		// 入力なし
	setNumOutputs(2);		// ステレオ出力
	isSynth();				// シンセである
	setUniqueID('VxS2');	// ★このプラグイン独自のユニークID。プラグインを公開する場合はSteinbergから取得すること。

	// シンセ本体の生成
	mSynth = new CMySynthMain();

	// GUIの生成
	//	setEditor(new hoge(this));

	// カレントプログラムを０番に設定
	curProgram = 0;
	setProgram(0);
	suspend();
}

// CMySynthクラスデストラクタ
CMySynth::~CMySynth() {
	// イベントバッファ開放
	if (mSynth != NULL) delete mSynth;
	if (!mEvents.empty()) mEvents.clear();

	endDbgWin();
}

// ★このプラグインの名前を返す
bool CMySynth::getEffectName(char* name) {
	vst_strncpy(name, "MySynth", kVstMaxEffectNameLen);
	return true;
}

// ★ベンダー名を返す
bool CMySynth::getVendorString(char* text) {
	vst_strncpy(text, "hoge", kVstMaxVendorStrLen);
	return true;
}

// ★このプラグインに関するテキストを返す
bool CMySynth::getProductString(char* text) {
	vst_strncpy(text, "Vst Synth", kVstMaxProductStrLen);
	return true;
}

// ★ベンダー固有バージョンを返す
VstInt32 CMySynth::getVendorVersion() {
	return 1000;
}

// このプラグインでサポートする機能を返す
VstInt32 CMySynth::canDo(char* text) {
	if (!strcmp(text, "receiveVstEvents"))	return 1;
	if (!strcmp(text, "receiveVstMidiEvent"))	return 1;
	return -1;
}

// MIDIイベント受信
VstInt32 CMySynth::processEvents(VstEvents* ev) {
	mCurrentEventNo = 0;
	//meventNum=0;
	mCurrentSample = 0;
	for (int i = 0; i < ev->numEvents; i++) {
		VstMidiEvent* pE = (VstMidiEvent*)ev->events[i];
		if ((ev->events[i]->type == kVstMidiType) && ((pE->midiData[0] & 0xf0) != 0xf0)) {
			// システムメッセージ以外なので、バッファへ追加
			mEvents.push_back(*(ev->events[i]));
		}
	}
	/*
	if(ev->numEvents>0){
	if(meventMaxNum < ev->numEvents){
	// 今のバッファサイズで小さすぎるので、サイズ拡張
	if(mEvents) delete[] mEvents;
	meventMaxNum = ev->numEvents;
	mEvents = new VstEvent[meventMaxNum];
	}
	// "MIDIシステムメッセージ"以外のMIDIイベントならバッファへコピー
	for(int i=0; i<ev->numEvents; i++){
	VstMidiEvent* pE = (VstMidiEvent*)ev->events[i];
	if((ev->events[i]->type==kVstMidiType) && ((pE->midiData[0] & 0xf0) != 0xf0)){
	// システムメッセージ以外なので、バッファへ追加
	mEvents[meventNum] = *(ev->events[i]);
	}
	}
	}
	*/
	return 1;
}


// 信号処理関数
void CMySynth::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) {
	int nRestSample = sampleFrames;
	VstMidiEvent *pE;
	float* out1 = outputs[0];
	float* out2 = outputs[1];
	int nProcessSample;

	// テンポ設定
	VstTimeInfo *vti = getTimeInfo(kVstTempoValid);
	if (vti) mSynth->setBaseTempo((float)vti->tempo);

	while (nRestSample>0) {
		// 次のイベントまでの処理できるサンプル数を求める
		if ((unsigned int)mCurrentEventNo<mEvents.size()) {	// イベントある
			pE = (VstMidiEvent*)&mEvents[mCurrentEventNo];
			nProcessSample = MAX(0, MIN(nRestSample, pE->deltaFrames - mCurrentSample));
		}
		else {								// イベントない
			pE = NULL;
			nProcessSample = nRestSample;
		}

		// 信号処理
		// SSE使用を前提に16バイト境界に揃えてバッファ確保
		CACHE_ALIGN16 SIG_LR dataLR[MAX_SIGBUF];
		int nRestSample2 = nProcessSample;
		while (nRestSample2>0) {
			int size = MIN(sizeof(dataLR) / sizeof(SIG_LR), nRestSample2);
			bool processed = false;
			mSynth->processSignalLR(size, dataLR, &processed);
			if (processed)	for (int i = 0; i<size; i++) { *out1++ = dataLR[i].l;	*out2++ = dataLR[i].r; }
			else			for (int i = 0; i<size; i++) { *out1++ = 0;			*out2++ = 0; }

			nRestSample2 -= size;
		}
		nRestSample -= nProcessSample;
		mCurrentSample += nProcessSample;

		// イベント処理
		if (pE) {
			mSynth->processMIDIShortData(pE->midiData[0], pE->midiData[1], pE->midiData[2]);
			mCurrentEventNo++;
		}
	}

}

// サンプリングレート設定
void CMySynth::setSampleRate(float sampleRate) {
	AudioEffectX::setSampleRate(sampleRate);

	mSynth->setFs(sampleRate);
}

// ホストからのシンセパラメタ設定
void CMySynth::setParameter(VstInt32 index, float value) {
	mSynth->setSoundParam(index, value);
}

// ホストからのシンセパラメタ要求
float CMySynth::getParameter(VstInt32 index) {
	if (index >= 0 && index < SOUND_PARAM_NUM) return mSynth->patchInfo[curProgram].soundParam[index];
	return 0.f;
}

// パラメタ名を返す
void CMySynth::getParameterName(VstInt32 index, char* text) {
	char work[256];
	mSynth->getSoundParamName(index, work);
	work[kVstMaxParamStrLen] = '\0';
	strcpy(text, work);
}

// パラメタ値に対応するテキストを返す
void CMySynth::getParameterDisplay(VstInt32 index, char* text) {
	char work[256];
	mSynth->getSoundParamValText(index, work);
	work[kVstMaxParamStrLen] = '\0';
	strcpy(text, work);
}

// プログラム名を返す
bool CMySynth::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text) {
	if (index < 0 || index >= PROGRAM_NUM) return false;

	vst_strncpy(text, &mSynth->patchInfo[index].programName[0], kVstMaxProgNameLen);
	return true;
}

// カレントのプログラム名を返す
void CMySynth::getProgramName(char* name) {
	vst_strncpy(name, &mSynth->patchInfo[curProgram].programName[0], kVstMaxProgNameLen);
	return;
}

// カレントのプログラム名の変更要求
void CMySynth::setProgramName(char* name) {
	vst_strncpy(&mSynth->patchInfo[curProgram].programName[0], name, kVstMaxProgNameLen);
	return;
}

// プログラム(パッチ)の変更要求
void CMySynth::setProgram(VstInt32 program) {
	if (program < 0 || program >= PROGRAM_NUM) return;

	curProgram = program;
	mSynth->setProgramNo(program);
}

// パッチ情報を返す
VstInt32 CMySynth::getChunk(void** data, bool isPreset) {
	if (isPreset)	*data = (void*)&mSynth->patchInfo[curProgram];
	else			*data = (void*)&mSynth->patchInfo[0];
	if (isPreset)	return sizeof(PATCHINFO);
	else			return sizeof(PATCHINFO)*PROGRAM_NUM;
}

// パッチ情報を受け取って内部に設定する
VstInt32 CMySynth::setChunk(void* data, VstInt32 byteSize, bool isPreset) {
	//edPrintf("setChunk() byteSize=%d, isPreset=%d SIZEOF(PATCHINFO)=%d\n",byteSize,isPreset,sizeof(PATCHINFO));
	if (isPreset) {
		if (byteSize != sizeof(PATCHINFO)) return 0;
		mSynth->patchInfo[curProgram] = *(PATCHINFO*)data;
	}
	else {
		if (byteSize != sizeof(PATCHINFO)*PROGRAM_NUM) return 0;
		for (int i = 0; i<PROGRAM_NUM; i++)mSynth->patchInfo[i] = ((PATCHINFO*)data)[i];
	}
	return 1;
}

