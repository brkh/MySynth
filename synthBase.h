#pragma once
// synthBase.h
// �V���Z�{�̂ɕK�v�ƂȂ�AMIDI���߁A�{�C�X�R���g���[������ȂǁA�ʓ|�Ȏ���
// ����Ă����N���X�B


/////////////////////////////////////////////////////////////////////////
// �M�������̃x�[�X�N���X
// �T���v�����O���g���̕ێ���A�M���o��I/F���`
class CSignalProcessor {
public:
	CSignalProcessor() { mfs = 44100.f; };

	// �T���v�����O���g���֘A
	void setFs(float fs) { mfs = fs; changedSamplingFreq(); };	// �T���v�����O���g���ݒ�
	virtual void changedSamplingFreq(void) {};				// ���̕ύX�ʒm

	// �M�������i���m�����j
	virtual void processSignal(int size, SIGREAL* pSig, bool* pProcessed) {};

	// �M�������i�X�e���I�j
	virtual void processSignalLR(int size, SIG_LR* pSig, bool* pProcessed) {
		// �f�t�H���g����(���m�����������āA�X�e���I�f�[�^�Ɋg������)�̒�`
		// �܂����m�������������s
		processSignal(size, (SIGREAL*)pSig, pProcessed);
		if (*pProcessed) {
			// ���m�����f�[�^���X�e���I�f�[�^�֊g��
			// d,s��SIGREAL*�łȂ��Aint*���g���Ă���̂�FPU����݂����Ȃ����߁B
			// ������f�m�[�}������FPU�փ��[�h�����ƃg���u���ɂȂ�B�܂��AFPU�o�R�ł͎኱�x���Ȃ�\��������B
			int* s = (int*)pSig;
			int* d = s + (size - 1) * 2;
			for (int i = size - 1; i >= 0; i--, d -= 2) *d = *(d + 1) = s[i];
		}
	}

protected:
	float mfs;		// �T���v�����O���g��
};


/////////////////////////////////////////////////////////////////////
// MIDI���͉��߃N���X
class CMIDIInDev {
public:

	CMIDIInDev(void) {
		mprogramBankNo = mNRPNNo = mRPNNo = mparamData = 0;
	};

	virtual ~CMIDIInDev() {};

	// MIDI��M�ʒm
	// �`�����l���{�C�X���b�Z�[�W
	virtual void receiveNoteOff(int nn, int v) {};
	virtual void receiveNoteOn(int nn, int v) {};
	virtual void receivePolyKeyPressure(int nn, int v) {};
	// �`�����l���{�C�X���b�Z�[�W(�R���g���[���`�F���W)
	virtual bool processMIDICCOriginal(BYTE st, BYTE dt1, BYTE dt2) { return false; };
	virtual void receiveModulationWheel(int v) {};
	virtual void receiveVolume(int v) {};
	virtual void receiveDumperPedal(bool b) {};
	virtual void receiveNRPNData(int paramNo, int data, bool bHIByte) {};
	virtual void receiveRPNData(int paramNo, int data, bool bHIByte) {};
	// �`�����l�����[�h���b�Z�[�W
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

	// MIDI�V���[�g���b�Z�[�W��M����
	void processMIDIShortData(BYTE st, BYTE dt1, BYTE dt2) {
		int const nKind = st & 0xF0;
		int ch = st & 0x0F;
		switch (nKind) {
			case 0x80:		// �m�[�g�I�t
			_NOTEOFF :
				if (dt1 < 0 || dt1 > 127) break;
					 receiveNoteOff(dt1, dt2);
					 break;

			case 0x90:		// �m�[�g�I��
				if (dt2 == 0) goto _NOTEOFF;
				if (dt1 < 0 || dt1 > 127) break;
				receiveNoteOn(dt1, dt2);
				break;

			case 0xA0:		// �|���t�H�j�b�N�L�[�v���b�V���[
				receivePolyKeyPressure(dt1, dt2);
				break;

			case 0xB0:
				if (dt1<120) {	// �R���g���[���`�F���W
					//edPrintf("�R���g���[���`�F���Wdt1,dt2=%02d,%02d\n",dt1,dt2);
					// �Ǝ���CC�A�T�C������
					bool rtn = processMIDICCOriginal(st, dt1, dt2);
					if (rtn == true) break;

					// �f�t�H���g��CC����
					switch (dt1) {
						case 0x00:		// �v���O�����o���N�I���i��ʁj
							mprogramBankNo = MAKEWORD(LOBYTE(mprogramBankNo), dt2);
							break;

						case 0x01:		// ���W�����[�V�����z�C�[��
							receiveModulationWheel(dt2);
							break;

						case 0x06:		// �f�[�^�G���g���[�i��ʁj
							setDataEntryHI(dt2);
							break;

						case 0x07:		// �{�����[��
							receiveVolume(dt2);
							break;

						case 0x20:		// �v���O�����o���N�I���i���ʁj
							mprogramBankNo = MAKEWORD(dt2, HIBYTE(mprogramBankNo));
							break;

						case 0x26:		// �f�[�^�G���g���[�i���ʁj
							setDataEntryLO(dt2);
							break;

						case 0x40:		// �_���p�[�y�_��
							receiveDumperPedal(dt2 >= 64);
							break;

						case 0x62:		// �m�q�o�m�p�����^�I��(����)
							mNRPNNo = MAKEWORD(LOBYTE(mNRPNNo), dt2);
							mRPNNo = -1;
							break;

						case 0x63:		// �m�q�o�m�p�����^�I��(���)
							mNRPNNo = MAKEWORD(LOBYTE(mNRPNNo), dt2);
							break;

						case 0x64:		// �q�o�m�p�����^�I��(����)
							mRPNNo = MAKEWORD(dt2, HIBYTE(mRPNNo));
							mNRPNNo = -1;
							break;

						case 0x65:		// �q�o�m�p�����^�I��(���)
							mRPNNo = MAKEWORD(dt2, HIBYTE(mRPNNo));
							break;
					}

				}
				else {		// �`���l�����[�h���b�Z�[�W
					switch (dt1) {
						case 0x78:		// �I�[���T�E���h�I�t
							receiveAllSoundOff();
							break;

						case 0x79:		// ���Z�b�g�I�[���R���g���[��
							receiveResetAllController();
							break;

						case 0x7B:		// �I�[���m�[�g�I�t
							receiveAllNoteOff();
							break;
					}
				}
				break;

			case 0xC0:		// �v���O�����`�F���W
				receiveProgramChange(dt1, mprogramBankNo);
				break;

			case 0xD0:		// �`�����l���v���b�V���[
				receiveChannelPressure(dt1);
				break;

			case 0xE0:		// �s�b�`�x���h
				//edPrintf("dt1,dt2=%02d,%02d\n",dt1,dt2);
				receivePitchBend(dt2 * 128 + dt1 - 8192);
				break;

			case 0xF0:		// �V�X�e�����b�Z�[�W
				//edPrintf("�V�X�e�����b�Z�[�W st=%02x,dt1,dt2=%02x,%02x\n",st,dt1,dt2);
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
// ���Տ��Ǘ��N���X
// �{�C�X�̃g���K�[/�����[�X���R���g���[�����邽�߂ɁA�����ꂽ���Ղ̏��������L���A�Ǘ�����N���X
class CKeyboardInfo {
public:
	CKeyboardInfo() {
		mOnKeyNum = 0;
		bStatusChanged = bDumper = false;
		for (int i = 0; i<128; i++) mKeyTable[i] = mDumperTable[i] = 0;
	};
	virtual ~CKeyboardInfo() {};
	// �L�[�Ō�
	void onKey(int nn, int v) {
		if (mKeyTable[nn] != 0) offKeyImpl(nn);
		mKeyTable[nn] = v;
		mDumperTable[nn] = false;
		mOnKeyNNList[mOnKeyNum] = nn + 256;	// +256�͐V�K�����t���O
		mOnKeyNum++;
		bStatusChanged = true;
	};
	// �L�[������
	void offKey(int nn) {
		if (bDumper) mDumperTable[nn] = true;
		else offKeyImpl(nn);
	};
	// �_���p�[�y�_���̓���/����
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
	// n�ԖڂɐV���������L�[�̃m�[�gNO���擾�B�Ȃ���΁A-1��Ԃ��B
	int getOnKeyNN(int n) {
		if (n >= mOnKeyNum) return -1;
		return mOnKeyNNList[mOnKeyNum - 1 - n];
	};
	// n�ԖڂɐV�����V�K�����L�[�̃m�[�gNO���擾�B�Ȃ���΁A-1��Ԃ��B
	int getNewOnKeyNN(int n) {
		if (n >= mOnKeyNum) return -1;
		int noteNo = mOnKeyNNList[mOnKeyNum - 1 - n];
		if (noteNo < 256) return -1;		// �V�K�ł͂Ȃ�
		return noteNo - 256;
	};
	// ���Տ�ԕύX�t���O���Z�b�g
	void resetStatusChange(void) {
		bStatusChanged = false;
		// �V�K�����t���O�����ׂė��Ƃ�
		for (int i = 0; i<mOnKeyNum; i++) if (mOnKeyNNList[i] >= 256) mOnKeyNNList[i] -= 256;
	};
	// �L�[�I������Ă���L�[�̐���Ԃ�
	int getOnKeyNum(void) { return mOnKeyNum; };
	// �Ō�ɃR�[������resetStatusChange()���献�Տ�Ԃ��ύX����Ă��邩�H��Ԃ�
	bool isStatusChanged(void) { return bStatusChanged; };
	// �������̃x���V�e�B�l��Ԃ�
	int getVelocity(int nn) { return mKeyTable[nn]; };

private:
	int mOnKeyNum;				// �������̃L�[��
	int mKeyTable[128];		// �L�[�ƃx���V�e�B�Ƃ̑Ή��\
	int mOnKeyNNList[128];		// �������ꂽ�L�[�̏��Ԃ�ێ����郊�X�g
	bool bDumper;				// �_���p�[�y�_���t���O
	bool mDumperTable[128];	// �_���p�[�I�t�ŁA�����[�X���ׂ����Տ��
	bool bStatusChanged;		// �L�[�̉�����Ԃ��ύX���ꂽ���̃t���O

	// �L�[�I�t���Ɏg�p����T�u���[�`��
	void offKeyImpl(int nn) {
		mKeyTable[nn] = 0;
		bStatusChanged = true;
		// �I���L�[���X�g����A�L�[�I�t�������Տ�����菜��
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
// �{�C�X(Interface)
class IVoice : public CSignalProcessor {
public:
	IVoice() { mno = 0; mpan = 0.5f; };
	void setNo(int no) { mno = no; };				// �{�C�X�����ʂ��邽�߂̃{�C�X�ԍ�(�A��)��ݒ�
	int getNo(void) { return mno; };				// �{�C�X�ԍ���Ԃ�
	virtual void setDetune(float det) {};		// �f�`���[���l�ݒ�A�P�ʂ̓Z���g
	virtual void setPan(float pan) { mpan = pan; };	// �p���l�ݒ�
	virtual float getPan(void) { return mpan; };	// �p���l��Ԃ�
	virtual void setNoteInfo(int nn, int velo) {};// ���U�m�[�gNo���̐ݒ�B�L�[�I�����ɃR�[�������B
	// ���j�]���{�C�X�ׂ̈̏��ݒ�B�{�C�X�Ԃňʑ��𐧌䂷�鎞�̈חp�B�L�[�I�����ɃR�[�������B
	virtual void setUnisonInfo(IVoice* pMasterVoice, int unisonNum, int unisonNo) {};
	virtual void trigger(void) {};				// �L�[�g���K�[�ʒm
	virtual void release(int velo) {};			// �L�[�����[�X�ʒm
	virtual int  getNoteNo(void) { return 0; };	// ���U�m�[�gNo��Ԃ�
	virtual bool isKeyOn(void) { return false; };	// �L�[�I��������Ԃ�
	// �{�C�X���r�W�[(������)���ǂ�����Ԃ�
	// isKeyOn()�������ȃL�[��ON/OFF�t���O�Ȃ̂ɑ΂��AisBusy()�͌��ݔ������Ă��邩�ǂ�����Ԃ��B
	// �L�[�I�t��Ԃ�isKeyOn()��false��Ԃ��悤�ȏ󋵂ł��A�����[�X������������Ă����ԂȂ�AisBusy()��true��Ԃ��B
	virtual bool isBusy(void) { return false; };

protected:
	int mno;		// �{�C�X�ԍ�
	float mpan;	// �{�C�X�p���l(0=left ... 0.5=center ... 1=right)
};



///////////////////////////////////////////////////
// �{�C�X�R���g���[���[
// �{�C�X���Ǘ����ɂ����āA�L�[���[�h�ɉ����āA�g���K�[/�����[�X�������s����B
#define KM_POLY			0	// �|��
#define KM_MONO			1	// ���m
#define KM_MONO2		2	// ���K�[�g

class CVoiceCtrl : public CSignalProcessor {
public:
	CVoiceCtrl();

	void setKeyboardInfo(CKeyboardInfo* p);	// �L�[�{�[�h���Ǘ��N���X�̐ݒ�
	void setVoiceNum(int n);				// �Ǘ�����{�C�X���̐ݒ�
	void setVoice(int no, IVoice* pV);		// �Ǘ�����e�{�C�X�̐ݒ�
	void setVoiceMode(int mode);			// �L�[���[�h�̐ݒ�
	void setUnisonVoiceNum(int n);			// ���j�]���{�C�X���̐ݒ�
	void controlVoiceTrigger(void);			// �g���K�[/�����[�X�𐧌䂷��
	// �M�������i�{�C�X�̐M���������s���A�X�e���IMIX����j
	void processSignalLR(int size, SIG_LR* pSig, bool* pProcessed);

private:
	int iKeyMode;				// �L�[���[�h
	CKeyboardInfo* kbdInfo;	// �L�[�{�[�h�Ǘ��N���X
	int mVoiceNum;				// �o�^���ꂽ�{�C�X�̐�
	int mPolyNum;				// �|�����[�h���̍ő�{�C�X��
	int mUnisonNum;			// ���j�]���{�C�X��

	IVoice* mpVoices[128];			// �Ǘ�����{�C�X����
	int iCurVoiceNo;				// �J�����g�{�C�X�ԍ�
	std::list<IVoice*> monVoices;	// �L�[�I�����̃{�C�X���X�g
	int mmonoCurrentVelocity;		// ���m���[�h���Ɏg�����[�N�p�x���V�e�V�e�B�l
	IVoice* getNextOffVoice(void);	// �󂫃{�C�X�̃T�[�`
	void controlVoicePoly(void);	// �|�����[�h�ł̃g���K�[/�����[�X����
	void controlVoiceMono(void);	// ���m���[�h�ł̃g���K�[/�����[�X����
};