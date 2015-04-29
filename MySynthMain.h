#pragma once
// MySynthMain.h
// �V���Z�{�̃N���X�y�сA���̃R���|�[�l���g�ƂȂ�N���X

////////////////////////////////////////////////////////////
// VCO(�I�V���[�^�[)�N���X
#define		WF_SINE		0	// �T�C���g
#define		WF_TRI		1	// �O�p�g
#define		WF_SAW		2	// ���g
#define		WF_SQUARE	3	// ��`�g

class CVCO : public CSignalProcessor {
public:
	CVCO();
	virtual ~CVCO();

	// �f�`���[���l�ݒ�
	void setDetune(float det);

	// �p�����^�l�ݒ�
	void setWF(int waveFormType);				// �g�`���
	void setPortaTime(float fVal);	// �|���^�����g�^�C��

	// ���U�m�[�g�ԍ��ݒ�
	void setNoteNo(int nn, bool isKeyOn);

	// ���m�����M������(CSignalProcessor����p��)
	void processSignal(int size, SIGREAL* pSig, bool* pProcessed);

private:
	// �g�`�����ɒ��ڂ������ϐ�
	float notesNum;				// ���U�w�肳�ꂽ�m�[�gNo
	float curNotesNum;			// ���ݔ��U���̃m�[�gNo�i�m�[�gNo�������ɂ��鎖�Ń|���^�����g���̃m�[�gNo��\������j
	float portaStartNotesNum;	// �|���^�����g�J�n����noteNo
	float curPortaTime;	// �|���^�����g�o�ߎ��ԁi1�Ő��K���A0�`1�Ń|���^�����g���j
	float portaTimeDelta;	// �|���^�����g���x
	float detuneCent;		// �{�C�X�ԃf�`���[���l�i�P�ʂ̓Z���g�j
	unsigned int phase;		// �ʑ�(�Œ菬���_)
	float phase2;				// �ʑ�(���������_)�A��r�p
	int waveFormType;				// �g�`���
};


////////////////////////////////////////////////////////////
// �{�C�X�N���X
class CMyVoice : public IVoice {
public:
	CMyVoice();
	~CMyVoice();

	// �{�C�X�ԃf�`���[��(IVoice����p��)
	void setDetune(float det);

	// �g���K�[�֘A����(IVoice����p��)
	void setNoteInfo(int nn, int velo);
	void trigger(void);
	void release(int velo);
	int  getNoteNo(void);
	bool isKeyOn(void);
	bool isBusy(void);

	// �M������(CSignalProcessor����p��)
	void processSignal(int size, SIGREAL* pSig, bool* pProcessed);
	void changedSamplingFreq(void);

	// �O�����J�I�u�W�F�N�g
	CVCO vco;		// VCO

private:
	int notesNum;		// ���݂̃m�[�gNO
	int velo;		// �g���K�[���̃x���V�e�B�l
	bool bKeyOn;	// �L�[��������Ă��邩�ǂ���
};


////////////////////////////////////////////////////////////
// �V���Z�{�̃N���X�y�сA�p�����^�\�����̒�`

// �V���Z�̊�{�萔
#define	VOICE_NUM			16		// 16�{�C�X�Ƃ���
#define	SOUND_PARAM_NUM		5		// �T�E���h�p�����^�̎�ސ�
#define	PROGRAM_NUM			128		// 128�v���O����

// �T�E���h�p�����^ID�ꗗ
#define	SPID_MASTER_VOL			0		// �}�X�^�[�{�����[��
#define	SPID_VCO_WF				1		// VCO�E�F�[�u�t�H�[��
#define	SPID_VCO_PORTA_TIME		2		// VCO�|���^�����g�^�C��
#define	SPID_VOICE_MODE			3		// �{�C�X���[�h
#define	SPID_UNISON_NUM			4		// ���j�]���{�C�X��

// �P�̃p�b�`�����i�[����\��
typedef struct tagPatchInfo {
	char programName[25];					// �v���O������(24����+�I�[)
	float soundParam[SOUND_PARAM_NUM];	// �T�E���h�p�����^�ێ��̈�
} PATCHINFO;

// �V���Z�{�̃N���X
class CMySynthMain : public CSignalProcessor, public CMIDIInDev {
public:
	CMySynthMain::CMySynthMain();
	virtual ~CMySynthMain(void);

	// MIDI�C�x���g���̃n���h��(CMIDIInDev����p��)
	void receiveNoteOff(int nn, int v);
	void receiveNoteOn(int nn, int v);
	void receiveDumperPedal(bool b);
	void setBaseTempo(float t);

	// �p�����^�֘A
	void setSoundParam(int id, float fVal);		// �T�E���h�p�����^�l�̎�M
	void getSoundParamName(int id, char* rtn);	// �T�E���h�p�����^���̕ԐM
	void getSoundParamValText(int id, char* rtn);// �T�E���h�p�����^�l�ɑΉ�����e�L�X�g�ԐM

	// �v���O�����֘A
	void setProgramNo(int no);	// �v���O����(�p�b�`)�̕ύX�v��

	// �M������(CSignalProcessor����p��)
	void processSignalLR(int size, SIG_LR* pSig, bool* pProcessed);
	void changedSamplingFreq(void);

	// �p�b�`���i���J�I�u�W�F�N�g�j
	PATCHINFO patchInfo[PROGRAM_NUM];

private:
	CKeyboardInfo kbdInfo;		// ���Տ��Ǘ��N���X
	CVoiceCtrl vc;					// �{�C�X�R���g���[���[
	CMyVoice mVoice[VOICE_NUM];	// �{�C�X
	float masterVol;				// �}�X�^�[�{�����[��
	float* mCurSoundParam;			// �J�����g�T�E���h�p�����^�i�ւ̃|�C���^�j
};

