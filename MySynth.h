#pragma once

// MySynth.h
// MySynth�N���X�BVSTi�v���O�C���̃��C���N���X�Ƃł������ׂ��N���X�B
// �������A�V���Z�{�̂�CMySynthMain�N���X�ɕ������Ă���̂ŁA�ʏ�͕ύX�̕K�v�Ȃ��B

class CMySynth : public AudioEffectX {
public:
	CMySynth(audioMasterCallback audioMaster);
	virtual ~CMySynth(void);

	//// AudioEffectX����I�[�o�[���C�h
	// �C�x���g����
	VstInt32 canDo(char* text);
	VstInt32 processEvents(VstEvents* events);

	// �M������
	void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);
	void setSampleRate(float sampleRate);

	// �v���p�e�B�֘A
	bool getEffectName(char* name);
	bool getVendorString(char* text);
	bool getProductString(char* text);
	VstInt32 getVendorVersion(void);

	// �p�����[�^�֘A
	void setParameter(VstInt32 index, float value);
	float getParameter(VstInt32 index);
	void getParameterName(VstInt32 index, char* text);
	void getParameterDisplay(VstInt32 index, char* text);

	// �v���O�����֘A
	bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text);
	void getProgramName(char* name);
	void setProgramName(char* name);
	void setProgram(VstInt32 program);
	VstInt32 getChunk(void** data, bool isPreset);
	VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset);

private:
	CMySynthMain*	mSynth;			// �V���Z�{�̃N���X
	//VstEvent	*mEvents;				// �C�x���g�o�b�t�@
	vector<VstEvent>	mEvents;
	//int			meventMaxNum;			// �o�b�t�@�Ɋi�[�\�ȍő�C�x���g��
	//int			meventNum;				// �o�b�t�@�Ɋi�[����Ă���C�x���g��
	int			mCurrentEventNo;		// ���ݏ������̃C�x���gNo
	int			mCurrentSample;		// ���ݏ������̃T���v��No
};

