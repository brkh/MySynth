// MySynthMain.cpp
// �V���Z�{�̂���сA���̃R���|�[�l���g�̃N���X�B

#include <windows.h>
#include <math.h>
#include <list>
#include "common.h"
#include "debug.h"
#include "synthBase.h"
#include "MySynthMain.h"

// �g�`�e�[�u��
#define	WT_SIZE		1024				// �g�`�e�[�u���T�C�Y(�P�ʂ̓T���v��)
static SIGREAL g_sinTable[WT_SIZE];		// �T�C���g�e�[�u��
static SIGREAL g_triTable[WT_SIZE * 68];	// �O�p�g�e�[�u���i���g���ш斈�ɕ����j
static SIGREAL g_sawTable[WT_SIZE * 68];	// �m�R�M��
static SIGREAL g_squTable[WT_SIZE * 68];	// ��`

// �g�`�e�[�u�����
typedef struct tagTableInfo {
	float freq;		// ����g��
	float noteNo;	// ����g���ɑΉ�����MIDI�m�[�gNo
	int harmoNum;	// �{����
	int offset;		// �Ή�����g�`�e�[�u���̐擪�ւ̃I�t�Z�b�g
} TABLEINFO;
static TABLEINFO g_tblInfo[68 + 1];	// �e�[�u�����\
static int g_tblInfoNum;			// �e�[�u�����\�̍s��


/////////////////////////////////////////////////////////////////////
// �g�`�e�[�u���̏������֘A
#define _SIN(a)	(g_sinTable[(a)%WT_SIZE])	// sin()�֐��̑���Ƀe�[�u�����g���A�e�[�u��������������

// �O�p�g�e�[�u������
static void makeTriTable(SIGREAL* pOutBuf, int harmoNum) {
	for (int i = 0; i<WT_SIZE; i++) {
		pOutBuf[i] = 0;
		for (int j = 1; j <= harmoNum; j += 2) {		// harmoNum�{���܂Ő����i��{���݂̂Ȃ̂�2�������j
			if (j % 4 == 1) pOutBuf[i] += 1.f / (j*j) * _SIN(j*i);	// j=1,5,9,13,...
			else		 pOutBuf[i] -= 1.f / (j*j) * _SIN(j*i);	// j=3,7,11,15,...
		}
		pOutBuf[i] *= 8.f / (PI*PI);
	}
}

// �m�R�M���g�e�[�u������
static void makeSawTable(SIGREAL* pOutBuf, int harmoNum) {
	for (int i = 0; i<WT_SIZE; i++) {
		pOutBuf[i] = 0;
		for (int j = 1; j <= harmoNum; j++) {
			pOutBuf[i] += 1.f / j*_SIN(j*i);
		}
		pOutBuf[i] *= 2.f / PI;
	}
}

// ��`�g�e�[�u������
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

// �g�`�e�[�u���̏������i�����j
static void initWaveTable() {
	static bool g_bFirst = true;

	if (!g_bFirst) return;
	g_bFirst = false;

	// �T�C���g�e�[�u������
	for (int i = 0; i<WT_SIZE; i++) {
		g_sinTable[i] = sin(2.f*PI*i / WT_SIZE);
	}

	// �g�`�e�[�u�����\�i30Hz�`22.05KHz�𕡐��̑ш�ɕ��������ꂼ��̑ш�̏����L�����\�j���v�Z����B
	// �����̌v�Z���@�ł́A���̓p�����^�Ƃ���90������^�����ꍇ�A68�����ɍœK�������
	g_tblInfoNum = 0;
	int oldHarmoNum = -1;
	int j = 0;
	float minFreq = 30.f;	// �Œ���g����30Hz�Ƃ���B���������锭�U���g���ł́A�{���s���ɂȂ�B
	// �������A�e�[�u���T�C�Y���������Ă��{���s���ɂȂ�̂ŁA�v���ӁB
	float fs = 44100.f;	// fs��44.1K�Ƃ���B
	// ��fs��48K��96K�̏ꍇ�ɂ����ŋ��߂��\�Ɣg�`�e�[�u�������̂܂܎g���ƁA
	// �{���s���ɂȂ�B�{���́Afs���ς������A�{�\����сA�g�`�e�[�u���͍Đ������]�܂����B
	// ���A�����炭���͖w�Ǖς��Ȃ��Ǝv����B
	for (int i = 0; i<90; i++) {
		// �e�[�u�������v�Z
		g_tblInfo[j].freq = minFreq * pow(fs / 2 / minFreq, (float)i / 90);
		g_tblInfo[j].harmoNum = (int)(fs / 2 / g_tblInfo[j].freq);
		g_tblInfo[j].offset = j*WT_SIZE;
		// �O��Ɠ����{�������ǂ����`�F�b�N
		if (g_tblInfo[j].harmoNum == oldHarmoNum) continue;	// �����Ȃ̂ŏ����X�L�b�v
		oldHarmoNum = g_tblInfo[j].harmoNum;
		// �Ή����g�������߂Ȃ����AnoteNo�����߂�
		g_tblInfo[j].freq = fs / 2 / g_tblInfo[j].harmoNum;
		g_tblInfo[j].noteNo = 69.f + 12.f*log(g_tblInfo[j].freq / 440.f) / log(2.f);
		edPrintf("freq=%f noteNo=%f,baion=%d\n", g_tblInfo[j].freq, g_tblInfo[j].noteNo, g_tblInfo[j].harmoNum);

		j++;
	}
	g_tblInfoNum = j;
	edPrintf("g_tblInfoNum=%d\n", g_tblInfoNum);

	// ���g���ш斈�ɁA�O�p�g�A�m�R�M���g�A��`�g�e�[�u���𐶐�
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
// �g�`�����Ɋւ����{�I�ȃ��[�`���W

// �g�`�e�[�u���ւ̃|�C���^�ƁA�ʑ��i16:16�Œ菬���j��^����ƁA���`��Ԃ��Ĕg�`�l��ǂݏo���֐�
__forceinline SIGREAL _GET_WAVE(SIGREAL* pTbl, int phase) {
	// 16:16�̌Œ菬������C���f�b�N�X(=������)�����o���}�N���Ax:�Œ菬���_�ɂ��ʑ��Ay:�C���f�b�N�X�I�t�Z�b�g
#define _IDX(x,y)	(((y)+((x)>>16))&(WT_SIZE-1))

	int idx = _IDX(phase, 0);		// �������̎��o��
	int idx2 = _IDX(phase, 1);		// ��ԗp�ɁAidx�̎���idx�����߂�B�P����+1����Ɣ͈̓I�[�o�[�̉\���L��B
	float deci = (float)(phase & ((1 << 16) - 1)) / (1 << 16);	// �������̎��o��(���16�r�b�g���N���A���A1/65536�{����)
	return pTbl[idx] + (pTbl[idx2] - pTbl[idx]) * deci;	// ���`�⊮�ŏo��
}

// NoteNo,�f�`���[��(�Z���g�P��)����p���x�����߂�
static unsigned int calcWFromNoteNo(float nn, float det, float fs) {
	// NoteNo�A�f�`���[���l���甭�U���g��f�����߂� noteNo=69(A4)��440Hz
	// ���f�`���[���l�F100�Z���g��1NoteNo���̃s�b�`�ɑ�������
	float f = 440.f * pow(2.f, (nn + det / 100.f - 69.f) / 12.f);
	// �p���x(�P������WT_SIZE��16:16�Œ菬���_�\��)�֕ϊ�
	return (unsigned int)(WT_SIZE*(1 << 16)*f / fs);
}

// NoteNo����œK�ȁi���{�����i�C�L�X�g�𒴂��Ȃ��j�g�`�e�[�u�������߂�
static SIGREAL* getWTFromNoteNo(int waveFormType, float nn) {
	if (waveFormType == WF_SINE) {
		return &g_sinTable[0];
	}
	else {
		// �e�[�u�����\�𓪂���T�[�`�B
		// �����p�t�H�[�}���X�����ɂȂ�ꍇ�́Ann��K���ɐ������������̂�offset�̃}�b�v�e�[�u�����g���B
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
// VCO�N���X
CVCO::CVCO() {
	initWaveTable();
	curNotesNum = notesNum = portaStartNotesNum = 0.f;
	phase = 0;
	phase2 = 0.f;
	waveFormType = WF_SINE;
	curPortaTime = 1.f;
	portaTimeDelta = 0.f;
	detuneCent = 0.f;	// �{�C�X�ԃf�`���[���l�i�P�ʂ̓Z���g�j
}

CVCO::~CVCO() {
}

// �{�C�X�ԃf�`���[���l(�P�ʂ̓Z���g)�̐ݒ�
void CVCO::setDetune(float det) {
	detuneCent = det;
}

// �g�`��ސݒ�
void CVCO::setWF(int waveFormType) {
	waveFormType = waveFormType % 4;	// WF_SINE(=0)�`WF_SQUARE(=3)
}

// �|���^�����g���Ԑݒ�
void CVCO::setPortaTime(float fVal) {
	// 0�b�`3�b�B0�b�Ń|���^�����g�I�t�B�܂݈ʒu(fVal)�ɑ΂��Ďw���֐��I�ɕω�����悤�ɁB
	float t = exp((fVal - 1) * 3)*fVal * 3;
	// �|���^�����g���Ԃɉ������AportaTimeDelta���v�Z
	if (fVal == 0.f)	portaTimeDelta = 0.f;	// �|���^�����g�Ȃ�
	else			portaTimeDelta = 1.f / (mfs*t);
}

// ���UNoteNo�̐ݒ�
void CVCO::setNoteNo(int nn, bool isKeyOn) {
	// �|���^�����g�֘A�̕ϐ�����
	if (curPortaTime<1.f)		portaStartNotesNum = curNotesNum;	// �|���^�����g�����J�nNoteNo�̓|���^�����g����NoteNo
	else						portaStartNotesNum = notesNum;		// ����ԁ��J�nNoteNo�͍��܂ł̃m�[�gNo
	// �|���^�����g���K�v���ǂ����𔻒�
	// NordLead2��Synth1��"Auto"�ȃ|���^�����g����Ƃ���B
	// ������~�߁A��Ƀ|���^�����g����悤�ɂ���ɂ́A�J�n��������isKeyOn���폜����B
	if (portaStartNotesNum != nn && portaTimeDelta != 0.f
		&& isKeyOn)	curPortaTime = 0.f;	// �|���^�����g�J�n
	else							curPortaTime = 1.f;	// �|���^�����g�s�v
	// �V�����m�[�gNo��ێ�
	notesNum = (float)nn;
}

// �M������
void CVCO::processSignal(int size, SIGREAL* pSig, bool* pProcessed) {
#if 1
	// �g�`�e�[�u���ƌŒ菬���_�ʑ��ϐ��Ɛ��`��ԂŔg�`�������鏈����������19clk/sample�i����ԁj

	// Synth1�Ɠ����A�w���֐����̃|���^�����g�J�[�u(x=0�`1,y=0�`1)
#define		PORTAMENT_CURVE(x)		(1.f-exp(-x*4.f)*(1.f-x))

	int p = phase;	// �p�t�H�[�}���X����̂��߁A�N���e�B�J���ȃ��[�v���ł͈ʑ����̃����o�ϐ��𒼐ڍX�V/�Q�Ƃ��Ȃ�
	while (size > 0) {
		int processSize;
		int w;
		SIGREAL* pTbl;
		if (curPortaTime >= 1.f) {
			// ����ԁi�|���^�����g���ł͂Ȃ��j
			// ���U�m�[�gNo�ɉ������A�p���xw�Ɣg�`�e�[�u�������߂�B
			w = calcWFromNoteNo(notesNum, detuneCent, mfs);
			pTbl = getWTFromNoteNo(waveFormType, notesNum);
			processSize = size;
		}
		else {
			// �|���^�����g���B���U���g����A���I�ɕω�������B
			// ���U�m�[�gNo�����߂�
			curNotesNum = portaStartNotesNum + (notesNum - portaStartNotesNum) * PORTAMENT_CURVE(curPortaTime);
			// ���U�m�[�gNo�ɉ������A�p���xw�Ɣg�`�e�[�u�������߂�B
			w = calcWFromNoteNo(curNotesNum, detuneCent, mfs);
			pTbl = getWTFromNoteNo(waveFormType, curNotesNum);
			// 1�T���v������w���Čv�Z����̂��A�����I�ɂ͗��z�����A�p�t�H�[�}���X�������B
			// �����ł́A�����T���v�����ɂ����v�Z���Ȃ��悤�ɂ���B
			// �T���v�����́A�|���^�����g�J�[�u��y=x�Ȓ����Ɖ��肵(���ۂ͎w���֐�)�A
			// �s�b�`�̉𑜓x��5�Z���g(�K��)�ɂȂ�悤�Ɍ��肷��B
			// �{���̓|���^�����g�J�[�u�̋t�֐����狁�߂������������A�ʓ|�Ȃ̂ŏȗ��B
			// �������A�T���v����������������ƕ��ׂɂȂ邽�߁A�Œ�24�T���v���Ƃ���B
			// �����ɂ��āF�����̎��ł�1�T���v������w�Čv�Z�̉��Ɣ��ʕs�\�B
			float totalCent = fabs(notesNum - portaStartNotesNum)*100.f;
			int totalPortaSample = (int)(1.f / portaTimeDelta);
			processSize = MAX(24, (int)(totalPortaSample / totalCent / 5.f));
			processSize = MIN(size, processSize);
			curPortaTime += portaTimeDelta*processSize;	// �|���^�����g�o�ߎ��Ԃ����Z
		}

		// �g�`�ǂݏo���B
		for (int i = 0; i<processSize; i++) {
			*pSig++ = _GET_WAVE(pTbl, p);
			p += w;		// �ʑ���i�߂�
		}
		size -= processSize;
	}
	phase = p;		// ���[�J���ʑ��ϐ��������o�ϐ��֊i�[

#else
	// sin()�ƕ��������_�ʑ��ϐ��Ŕg�`��������x�[�V�b�N�ȏ�����������77clk/sample
	// ��̔g�`�e�[�u�������Ƃ̃p�t�H�[�}���X��r�p���[�`���B�|���^�����g�����Ȃ��B
	// �R�[�h�̓V���v�������A�g�`�e�[�u�������ɔ�ׂ�4�{�p�t�H�[�}���X�������B
	float p = mpp;
	float f = 440.f * pow(2.f, (notesNum + detuneCent / 100.f - 69) / 12.f);
	float w = 2 * PI*f / mfs;		// �p���x(�P�������Q�΂̕��������_)
	for (int i = 0; i<size; i++) {
		pSig[i] = sin(p);
		p += w;					// �ʑ���i�߂�
		if (p > 2.f*PI) p -= 2.f*PI;	// �ʑ��͈̓`�F�b�N
	}
	mpp = p;
#endif

	*pProcessed = true;
}

/////////////////////////////////////////////////////////////////////
// �{�C�X�N���X
CMyVoice::CMyVoice() {
	notesNum = velo = 0;
	bKeyOn = false;
}

CMyVoice::~CMyVoice() {
}

// �L�[�I�������ǂ�����Ԃ�
bool CMyVoice::isKeyOn(void) {
	return bKeyOn;
}

// ���������ǂ�����Ԃ��B
bool CMyVoice::isBusy(void) {
	return bKeyOn;	// �G���x���[�v���������Ă��Ȃ��̂ŁA������Ԃ̓L�[�̃I���I�t��ԂƓ���
}

// �f�`���[���l�ݒ�
void CMyVoice::setDetune(float det) {
	// �f�`���[���l�̉e�����󂯂�̂́AVCO�Ȃ̂ŁAVCO�Ƀf�`���[���l�ݒ�
	vco.setDetune(det);
}

// ���U�m�[�gNo���̐ݒ�B�L�[�I�����ɃR�[�������B
void CMyVoice::setNoteInfo(int nn, int velo) {
	notesNum = nn;
	velo = velo;
	vco.setNoteNo(nn, isKeyOn());
}

// �g���K�[�ʒm�i���K�[�g���[�h(KM_MONO2)�Ń��K�[�g�t�@���s�������ɂ́A�R�[������Ȃ��j
void CMyVoice::trigger(void) {
	bKeyOn = true;
	edPrintf("VoiceNo[%d] trigger\n", mno);
}

// �L�[�I�t�ʒm
void CMyVoice::release(int velo) {
	bKeyOn = false;
	edPrintf("VoiceNo[%d] release\n", mno);
}

// ���݂̃m�[�gNo��Ԃ�
int CMyVoice::getNoteNo(void) {
	return notesNum;
}

// �M������(���m����)
void CMyVoice::processSignal(int size, SIGREAL* pSig, bool* pProcessed) {
	// VCO�ɐM���������s�킹��
	vco.processSignal(size, pSig, pProcessed);
}

// �T���v�����O���g�����ʒm�����
void CMyVoice::changedSamplingFreq(void) {
	vco.setFs(mfs);
}

/////////////////////////////////////////////////////////////////////
// �V���Z�{�̃N���X
CMySynthMain::CMySynthMain() {
	vc.setKeyboardInfo(&kbdInfo);	// �L�[�{�[�h���Ǘ��N���X���{�C�X�R���g���[���֓o�^
	vc.setVoiceNum(VOICE_NUM);		// �{�C�X���̐ݒ�
	for (int i = 0; i<VOICE_NUM; i++) vc.setVoice(i, &mVoice[i]);	// �{�C�X�R���g���[���[�փ{�C�X��o�^

	// 128�v���O����������
	for (int i = 0; i<PROGRAM_NUM; i++) {
		// �l�ɐ������Ƃ�p�����^�́A�Œ�I��128�Ŋ����āA0�`1�ȓ��Ɏ��߂�
		patchInfo[i].soundParam[SPID_MASTER_VOL] = 0.1f;			// �}�X�^�[�{�����[���m�u�ʒu
		patchInfo[i].soundParam[SPID_VCO_WF] = WF_TRI / 128.f;		// �E�F�[�u�t�H�[��
		patchInfo[i].soundParam[SPID_VCO_PORTA_TIME] = 0.2f;		// �|���^�����g�^�C���m�u�ʒu
		patchInfo[i].soundParam[SPID_VOICE_MODE] = KM_MONO / 128.f;// �{�C�X���[�h
		patchInfo[i].soundParam[SPID_UNISON_NUM] = 4 / 128.f;		// ���j�]���{�C�X��
		strcpy(&patchInfo[i].programName[0], "init");
	}
	setProgramNo(0);
}

CMySynthMain::~CMySynthMain() {
}

// �e���|���ݒ�(�e���|��������@�\���������鎞�Ɏg��)
void CMySynthMain::setBaseTempo(float t) {
}

// MIDI�m�[�g�I�t��M
void CMySynthMain::receiveNoteOff(int nn, int v) {
	kbdInfo.offKey(nn);
}

// MIDI�m�[�g�I����M
void CMySynthMain::receiveNoteOn(int nn, int v) {
	kbdInfo.onKey(nn, v);
}

// MIDI�_���p�[�R���g���[����M
void CMySynthMain::receiveDumperPedal(bool b) {
	kbdInfo.setDumper(b);
}

// �T�E���h�p�����^(�p�b�`�p�����^)�l�̎�M
void CMySynthMain::setSoundParam(int id, float fVal) {
	// 0.f�`1.f��VST�����̃p�����^�l��0�`127�̐����ɕϊ�
	int iVal = MAX(0, MIN(127, (int)(fVal*128.f)));
	edPrintf("setSoundParam id=%d,val=%f\n", id, fVal);
	// ��M�����p�����^�ɉ����āA�e���i�ɐݒ���s���B
	switch (id) {
		case SPID_MASTER_VOL:		masterVol = fVal;													break;
		case SPID_VCO_WF:			for (int i = 0; i<VOICE_NUM; i++) mVoice[i].vco.setWF(iVal);			break;
		case SPID_VCO_PORTA_TIME:	for (int i = 0; i<VOICE_NUM; i++) mVoice[i].vco.setPortaTime(fVal);	break;
		case SPID_VOICE_MODE:		vc.setVoiceMode(iVal);											break;
		case SPID_UNISON_NUM:		vc.setUnisonVoiceNum(iVal);										break;
	}
	// �p�����^�l�̕ێ�
	if (id >= 0 && id < SOUND_PARAM_NUM) mCurSoundParam[id] = fVal;
}

// �T�E���h�p�����^����Ԃ�
void CMySynthMain::getSoundParamName(int id, char* rtn) {
	static char* names[] = { "Master Volume", "Wave Form", "Portament Time", "Voice Mode", "Unison Voice Num" };
	if (id >= 0 && id < SOUND_PARAM_NUM)
		strcpy(rtn, names[id]);
	else
		rtn[0] = '\0';
}

// �T�E���h�p�����^�l�̕\���e�L�X�g��Ԃ�
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

// �J�����g�v���O�������`�F���W����
void CMySynthMain::setProgramNo(int no) {
	edPrintf("setProgramNo no=%d\n", no);
	mCurSoundParam = &patchInfo[no].soundParam[0];
	// �V�����v���O�����̃p�����^���e���i�ɐݒ肷��
	for (int id = 0; id<SOUND_PARAM_NUM; id++) setSoundParam(id, mCurSoundParam[id]);
}

// �V���Z���̏o��(�X�e���I)
void CMySynthMain::processSignalLR(int size, SIG_LR* pSig, bool* pProcessed) {
	_START_CLOCK();		// CPU�N���b�N�v���J�n
	// ���Տ�Ԃ��ύX����Ă���΁A�{�C�X�̃g���K�[/�����[�X�������{�C�X�R���g���[���[�ɏ���������
	if (kbdInfo.isStatusChanged()) {
		vc.controlVoiceTrigger();		// �g���K�[/�����[�X����
		kbdInfo.resetStatusChange();	// ���Տ�ԕύX�t���O�𗎂Ƃ�
	}

	// �e�{�C�X�̐M���������s���X�e���IMIX����i�{�C�X�R���g���[���[�̎d���j�B
	vc.processSignalLR(size, pSig, pProcessed);

	// �}�X�^�[�{�����[������
	if (*pProcessed) {
		float vol = masterVol;
		for (int i = 0; i<size; i++) {
			pSig[i].l *= vol;
			pSig[i].r *= vol;
		}
	}
	_DISP_CLOCK(size);	// CPU�N���b�N�v���I���E�f�B�X�v���C�㕔�ɕ\��
}

// �T���v�����O���g�����ʒm�����
void CMySynthMain::changedSamplingFreq() {
	vc.setFs(mfs);
	for (int i = 0; i<VOICE_NUM; i++) mVoice[i].setFs(mfs);
}

