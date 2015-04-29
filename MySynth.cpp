// MySynth.cpp
// MySynth�N���X�̒�`�BVSTi�v���O�C�����C���N���X�Ƃł������ׂ��N���X�B
// �������A���̃N���X�̓��b�p�[�̂悤�Ȃ��̂Ȃ̂ŁA�ʏ�́A���Ɏ������ꕔ�̎��ʏ���������
// �C������K�v�͂Ȃ��B


// VSTSDK�AWindows�A���A�K�v�ȃw�b�_
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

// VSTSDK����(���ʂ̓R���p�C���P�ʂ𕪂��邪�A�ʓ|�Ȃ̂ł��̂悤�ɂ���)
#include <public.sdk\source\vst2.x\audioeffect.cpp>
#include <public.sdk\source\vst2.x\audioeffectx.cpp>
#include <public.sdk\source\vst2.x\vstplugmain.cpp>

// VST�I�u�W�F�N�g�̐����֐�
AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {
	// MySynth�̐���
	return new CMySynth(audioMaster);
}

// CMySynth�N���X�R���X�g���N�^
CMySynth::CMySynth(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, PROGRAM_NUM, SOUND_PARAM_NUM) {
	// �f�o�b�O�E�B���h�E�̃I�[�v��()
	startDbgWin((HINSTANCE)hInstance);	// hInstance�́Avstplugmain.cpp�ŃO���[�o���ϐ��Ƃ��Ē�`����Ă���B

	// �C�x���g�֘A�ϐ��̏�����
	mCurrentEventNo = mCurrentSample = 0;
	//mEvents=NULL;

	// ���̃v���O�C���̊e�푮����ݒ肷��
	programsAreChunks();	// ���F�f�[�^�͓Ǝ��`�����N�ŕۑ�����
	setNumInputs(0);		// ���͂Ȃ�
	setNumOutputs(2);		// �X�e���I�o��
	isSynth();				// �V���Z�ł���
	setUniqueID('VxS2');	// �����̃v���O�C���Ǝ��̃��j�[�NID�B�v���O�C�������J����ꍇ��Steinberg����擾���邱�ƁB

	// �V���Z�{�̂̐���
	mSynth = new CMySynthMain();

	// GUI�̐���
	//	setEditor(new hoge(this));

	// �J�����g�v���O�������O�Ԃɐݒ�
	curProgram = 0;
	setProgram(0);
	suspend();
}

// CMySynth�N���X�f�X�g���N�^
CMySynth::~CMySynth() {
	// �C�x���g�o�b�t�@�J��
	if (mSynth != NULL) delete mSynth;
	if (!mEvents.empty()) mEvents.clear();

	endDbgWin();
}

// �����̃v���O�C���̖��O��Ԃ�
bool CMySynth::getEffectName(char* name) {
	vst_strncpy(name, "MySynth", kVstMaxEffectNameLen);
	return true;
}

// ���x���_�[����Ԃ�
bool CMySynth::getVendorString(char* text) {
	vst_strncpy(text, "hoge", kVstMaxVendorStrLen);
	return true;
}

// �����̃v���O�C���Ɋւ���e�L�X�g��Ԃ�
bool CMySynth::getProductString(char* text) {
	vst_strncpy(text, "Vst Synth", kVstMaxProductStrLen);
	return true;
}

// ���x���_�[�ŗL�o�[�W������Ԃ�
VstInt32 CMySynth::getVendorVersion() {
	return 1000;
}

// ���̃v���O�C���ŃT�|�[�g����@�\��Ԃ�
VstInt32 CMySynth::canDo(char* text) {
	if (!strcmp(text, "receiveVstEvents"))	return 1;
	if (!strcmp(text, "receiveVstMidiEvent"))	return 1;
	return -1;
}

// MIDI�C�x���g��M
VstInt32 CMySynth::processEvents(VstEvents* ev) {
	mCurrentEventNo = 0;
	//meventNum=0;
	mCurrentSample = 0;
	for (int i = 0; i < ev->numEvents; i++) {
		VstMidiEvent* pE = (VstMidiEvent*)ev->events[i];
		if ((ev->events[i]->type == kVstMidiType) && ((pE->midiData[0] & 0xf0) != 0xf0)) {
			// �V�X�e�����b�Z�[�W�ȊO�Ȃ̂ŁA�o�b�t�@�֒ǉ�
			mEvents.push_back(*(ev->events[i]));
		}
	}
	/*
	if(ev->numEvents>0){
	if(meventMaxNum < ev->numEvents){
	// ���̃o�b�t�@�T�C�Y�ŏ���������̂ŁA�T�C�Y�g��
	if(mEvents) delete[] mEvents;
	meventMaxNum = ev->numEvents;
	mEvents = new VstEvent[meventMaxNum];
	}
	// "MIDI�V�X�e�����b�Z�[�W"�ȊO��MIDI�C�x���g�Ȃ�o�b�t�@�փR�s�[
	for(int i=0; i<ev->numEvents; i++){
	VstMidiEvent* pE = (VstMidiEvent*)ev->events[i];
	if((ev->events[i]->type==kVstMidiType) && ((pE->midiData[0] & 0xf0) != 0xf0)){
	// �V�X�e�����b�Z�[�W�ȊO�Ȃ̂ŁA�o�b�t�@�֒ǉ�
	mEvents[meventNum] = *(ev->events[i]);
	}
	}
	}
	*/
	return 1;
}


// �M�������֐�
void CMySynth::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) {
	int nRestSample = sampleFrames;
	VstMidiEvent *pE;
	float* out1 = outputs[0];
	float* out2 = outputs[1];
	int nProcessSample;

	// �e���|�ݒ�
	VstTimeInfo *vti = getTimeInfo(kVstTempoValid);
	if (vti) mSynth->setBaseTempo((float)vti->tempo);

	while (nRestSample>0) {
		// ���̃C�x���g�܂ł̏����ł���T���v���������߂�
		if ((unsigned int)mCurrentEventNo<mEvents.size()) {	// �C�x���g����
			pE = (VstMidiEvent*)&mEvents[mCurrentEventNo];
			nProcessSample = MAX(0, MIN(nRestSample, pE->deltaFrames - mCurrentSample));
		}
		else {								// �C�x���g�Ȃ�
			pE = NULL;
			nProcessSample = nRestSample;
		}

		// �M������
		// SSE�g�p��O���16�o�C�g���E�ɑ����ăo�b�t�@�m��
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

		// �C�x���g����
		if (pE) {
			mSynth->processMIDIShortData(pE->midiData[0], pE->midiData[1], pE->midiData[2]);
			mCurrentEventNo++;
		}
	}

}

// �T���v�����O���[�g�ݒ�
void CMySynth::setSampleRate(float sampleRate) {
	AudioEffectX::setSampleRate(sampleRate);

	mSynth->setFs(sampleRate);
}

// �z�X�g����̃V���Z�p�����^�ݒ�
void CMySynth::setParameter(VstInt32 index, float value) {
	mSynth->setSoundParam(index, value);
}

// �z�X�g����̃V���Z�p�����^�v��
float CMySynth::getParameter(VstInt32 index) {
	if (index >= 0 && index < SOUND_PARAM_NUM) return mSynth->patchInfo[curProgram].soundParam[index];
	return 0.f;
}

// �p�����^����Ԃ�
void CMySynth::getParameterName(VstInt32 index, char* text) {
	char work[256];
	mSynth->getSoundParamName(index, work);
	work[kVstMaxParamStrLen] = '\0';
	strcpy(text, work);
}

// �p�����^�l�ɑΉ�����e�L�X�g��Ԃ�
void CMySynth::getParameterDisplay(VstInt32 index, char* text) {
	char work[256];
	mSynth->getSoundParamValText(index, work);
	work[kVstMaxParamStrLen] = '\0';
	strcpy(text, work);
}

// �v���O��������Ԃ�
bool CMySynth::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text) {
	if (index < 0 || index >= PROGRAM_NUM) return false;

	vst_strncpy(text, &mSynth->patchInfo[index].programName[0], kVstMaxProgNameLen);
	return true;
}

// �J�����g�̃v���O��������Ԃ�
void CMySynth::getProgramName(char* name) {
	vst_strncpy(name, &mSynth->patchInfo[curProgram].programName[0], kVstMaxProgNameLen);
	return;
}

// �J�����g�̃v���O�������̕ύX�v��
void CMySynth::setProgramName(char* name) {
	vst_strncpy(&mSynth->patchInfo[curProgram].programName[0], name, kVstMaxProgNameLen);
	return;
}

// �v���O����(�p�b�`)�̕ύX�v��
void CMySynth::setProgram(VstInt32 program) {
	if (program < 0 || program >= PROGRAM_NUM) return;

	curProgram = program;
	mSynth->setProgramNo(program);
}

// �p�b�`����Ԃ�
VstInt32 CMySynth::getChunk(void** data, bool isPreset) {
	if (isPreset)	*data = (void*)&mSynth->patchInfo[curProgram];
	else			*data = (void*)&mSynth->patchInfo[0];
	if (isPreset)	return sizeof(PATCHINFO);
	else			return sizeof(PATCHINFO)*PROGRAM_NUM;
}

// �p�b�`�����󂯎���ē����ɐݒ肷��
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

