// synthBase.cpp
// �V���Z�{�̂ɕK�v�ƂȂ�A�{�C�X�R���g���[������Ȃǂ̎���

#include <windows.h>
#include <math.h>
#include <list>
#include "common.h"
#include "debug.h"
#include "synthBase.h"


//////////////////////////////////////////
// CVoiceCtrl �{�C�X�R���g���[���N���X

CVoiceCtrl::CVoiceCtrl() {
	iKeyMode = KM_POLY;
	iCurVoiceNo = 0;
	kbdInfo = NULL;
	mUnisonNum = 1;
}

// �L�[�{�[�h���Ǘ��N���X�̐ݒ�
void CVoiceCtrl::setKeyboardInfo(CKeyboardInfo* p) {
	kbdInfo = p;
}

// �{�C�X���̐ݒ�
void CVoiceCtrl::setVoiceNum(int n) {
	mVoiceNum = n;
	mPolyNum = n;
}

// �e�{�C�X�̐ݒ�
void CVoiceCtrl::setVoice(int no, IVoice* pV) {
	mpVoices[no] = pV;
	pV->setNo(no);
}

// �L�[���[�h�iPOLY/MONO/MONO2�j�̐ݒ�
void CVoiceCtrl::setVoiceMode(int mode) {
	iKeyMode = mode % 3;	// 0,1,2
	// �s�v�{�C�X�́A�����[�X���A�J�����g�{�C�X��擪��
	for (int i = (iKeyMode == KM_POLY) ? mPolyNum : mUnisonNum; i<mVoiceNum; i++) {
		if (mpVoices[i]->isKeyOn()) mpVoices[i]->release(0);
		monVoices.remove(mpVoices[i]);
	}
	iCurVoiceNo = -1;

}

// ���j�]���{�C�X���̐ݒ�
void CVoiceCtrl::setUnisonVoiceNum(int n) {
	n = MIN(mVoiceNum, MAX(1, n));
	if (n == mUnisonNum) return;
	// ���j�]���{�C�X���ݒ�
	mUnisonNum = n;

	// ���j�]���{�C�X���ɉ����āA�{�C�X�Ԃ̃f�`���[���l�A�p���l���v�Z���A�ݒ肷��
	// �E�{�C�X�Ԃ̃f�`���[�����͈ꗥ�A10cent�Ƃ��Ă݂�B
	// �E�{�C�X�p���͍������ς�(=0)����E�����ς�(=1)�ɋϓ��ɎU�炷
	// �����ł̃f�`���[���l��p���l�̌��ߕ��͒P�Ȃ���B
	float detSpread = 10.f;
	for (int i = 0; i<mVoiceNum; i++) {
		int vn = i%mUnisonNum;
		float det = vn*detSpread - detSpread*(mUnisonNum - 1) / 2.f;
		float pan = (mUnisonNum == 1) ? 0.5f : vn*1.f / (mUnisonNum - 1);
		mpVoices[i]->setDetune(det);
		mpVoices[i]->setPan(pan);
	}

	// �s�v�{�C�X�́A�����[�X���A�J�����g�{�C�X��擪��
	for (int i = (iKeyMode == KM_POLY) ? mPolyNum : mUnisonNum; i<mVoiceNum; i++) {
		if (mpVoices[i]->isKeyOn()) mpVoices[i]->release(0);
		monVoices.remove(mpVoices[i]);
	}
	iCurVoiceNo = -1;
}

// �󂫃{�C�X�̃T�[�`
IVoice* CVoiceCtrl::getNextOffVoice() {
	int curVoiceNo = iCurVoiceNo;
	for (int i = 0; i<mPolyNum; i++) {
		curVoiceNo++;
		if (curVoiceNo >= mPolyNum) curVoiceNo = 0;
		if (!mpVoices[curVoiceNo]->isKeyOn()) return mpVoices[curVoiceNo];
	}
	// �ǂ���I���������B
	return NULL;
}

// ���Տ��Ɗe�{�C�X�̏�ԓ�����A�g���K�[/�����[�X�𐧌䂷��
void CVoiceCtrl::controlVoiceTrigger() {

	if (iKeyMode == KM_POLY)	controlVoicePoly();
	else						controlVoiceMono();
}

// �|�����[�h���̃g���K�[/�����[�X�𐧌�
void CVoiceCtrl::controlVoicePoly() {
	//// �܂��L�[�����[�X����
	// �I���{�C�X���X�g����݂āA�L�[�{�[�h�e�[�u����Ńm�[�g�I�t(�x���V�e�B�l=0)��
	// �Ȃ��Ă��邩���`�F�b�N���m�[�g�I�t�Ȃ烊���[�X����
	for (std::list<IVoice*>::iterator v = monVoices.begin(); v != monVoices.end();) {
		if (kbdInfo->getVelocity((*v)->getNoteNo()) == 0) {
			(*v)->release(0);
			v = monVoices.erase(v);
		}
		else v++;
	}

	//// �g���K�[����
	// �V�K�ɉ������ꂽ�L�[��Ώۂɏ������s���B
	int num = MIN(mPolyNum, kbdInfo->getOnKeyNum());	// ��������ő�m�[�g���̓|�����Ɖ����L�[���̂ǂ��炩���Ȃ���
	for (int i = 0; i<num; i++) {
		int noteNo = kbdInfo->getNewOnKeyNN(i);		// �V�K�����L�[��i�ԖڂɐV�����L�[�̃m�[�gNO�iNEW�L�[�t���O�t���j
		if (noteNo == -1) break;							// �V�K�����L�[�͂Ȃ�����������ȏ�Â��V�K�������Ȃ��͂��Ȃ̂ŏ����I��

		// ���ɓ����m�[�gNo�𔭉����Ă���{�C�X������΁A�����[�X����B
		// �Ⴆ�΃A���y�W�G�[�^�ŃQ�[�g�^�C���P�O�O���Ƃ������ɁA���̑Ή����Ȃ���΁A�����ǂ�ǂ�d�Ȃ��Ă����B
		// ����́A���̃A���S���Y���ł�note on/off�͌��Ղ̏�Ԃ�ύX���邾���ŁA�������ꎞ����off��on�̏��œ����ɂ����ꍇ�A
		// ���ʓI��noteoff�͖�������鎖�ɂȂ邽�߁B
		// ���̂悤�Ȏ��ɂȂ�Ȃ����߂ɁA����m�[�g�̕����{�C�X�����͕s�Ƃ���B
		for (std::list<IVoice*>::iterator v = monVoices.begin(); v != monVoices.end();) {
			if ((*v)->getNoteNo() == noteNo) {
				(*v)->release(0);
				v = monVoices.erase(v);
			}
			else v++;
		}

		// ���j�]���{�C�X�̐������A�g���K�[����
		IVoice* pUnisonMasterVoice;
		for (int u = 0; u < mUnisonNum; u++) {
			//// �g���K�[����{�C�X�̌���
			// �L�[�I�t����Ă���{�C�X���擾�B�����S���r�W�[��������A��ԌÂ�����I���ɂȂ��Ă���{�C�X���擾
			// ���������A�x�[�X���͏����Ȃǂ̍H�v�̗]�n�͂���
			IVoice* v = getNextOffVoice();
			if (v == NULL) {
				v = monVoices.front();	// �I�����ň�ԌÂ��{�C�X
				if (v == NULL) return;
				monVoices.pop_front();
			}
			iCurVoiceNo = v->getNo();
			if (u == 0) pUnisonMasterVoice = v; // ���j�\���}�X�^�[�{�C�X�̑ޔ�

			// �m�[�gNO���̐ݒ�ƃg���K�[
			v->setNoteInfo(noteNo, kbdInfo->getVelocity(noteNo));
			v->setUnisonInfo(pUnisonMasterVoice, mUnisonNum, u);
			v->trigger();

			// �I���{�C�X���X�g�֒ǉ�
			monVoices.push_back(v);
		}
	}
}

// ���m���[�h���̃g���K�[/�����[�X�𐧌�
void CVoiceCtrl::controlVoiceMono() {
	// �Ȃɂ��L�[�����������Ă��Ȃ���΁A���݂̃I���{�C�X�������[�X���ďI���
	if (kbdInfo->getOnKeyNum() == 0) {
		for (int i = 0; i < mUnisonNum; i++) {
			monVoices.remove(mpVoices[i]);
			if (mpVoices[i]->isKeyOn()) mpVoices[i]->release(0);
		}
		mmonoCurrentVelocity = 0;
		return;
	}

	// �Ȃɂ��L�[�����������Ă���̂ň�ԐV�������Ղ��Ƃ��Ă��āA�������K�v������
	// 1.�V�K�̉����Ȃ疳�����ɔ���
	// 2.�V�K�ł͂Ȃ��ꍇ�A�{�C�X���I�����łȂ����A�I�����ł��m�[�gNO���Ⴆ�Δ���
	bool bProcess = false;
	int noteNo = kbdInfo->getNewOnKeyNN(0);
	if (noteNo != -1) {
		bProcess = true;
		mmonoCurrentVelocity = kbdInfo->getVelocity(noteNo);
	}
	else {
		// �V�K�ł͂Ȃ��������L�[�������ꂽ���ʁA��������鎖�ɂȂ����m�[�g
		// ���̃x���V�e�B�͗����ꂽ�L�[�Ɠ����Ƃ���B����āAmmonoCurrentVelocity�͍X�V���Ȃ��B
		noteNo = kbdInfo->getOnKeyNN(0);
		if (!mpVoices[0]->isKeyOn() || mpVoices[0]->getNoteNo() != noteNo) bProcess = true;
	}

	if (!bProcess) return;	// �����s�v

	// ��������
	IVoice* pUnisonMasterVoice = mpVoices[0];	// ���j�]���}�X�^�[�͏�Ƀ{�C�X�ԍ��[��
	if (iKeyMode == KM_MONO) {
		// ���m���[�h��
		// �K���g���K�[
		for (int i = 0; i < mUnisonNum; i++) {
			IVoice* v = mpVoices[i];
			// �m�[�gNO���̐ݒ�ƃg���K�[
			v->setNoteInfo(noteNo, mmonoCurrentVelocity);
			v->setUnisonInfo(pUnisonMasterVoice, mUnisonNum, 0);
			v->trigger();

			// �I���{�C�X���X�g�֒ǉ� (���łɓo�^����Ă���\��������̂ŁA��x�폜���Ă���ǉ�)
			monVoices.remove(v);
			monVoices.push_back(v);
		}
	}
	else if (iKeyMode == KM_MONO2) {
		// ���K�[�g���m���[�h��
		// ���݃L�[�I�t�������ꍇ�̂݃g���K�[
		for (int i = 0; i < mUnisonNum; i++) {
			IVoice* v = mpVoices[i];
			// �m�[�gNO���̐ݒ�ƃg���K�[
			v->setNoteInfo(noteNo, mmonoCurrentVelocity);
			if (!v->isKeyOn()) {
				// ���݃L�[�I�t���g���K�[���A�I���{�C�X���X�g�֒ǉ�
				v->setUnisonInfo(pUnisonMasterVoice, mUnisonNum, 0);
				v->trigger();
				monVoices.push_back(v);
			}
			else {
				// ���݃I�����g���K�[���Ȃ�
			}
		}
	}
}

// �M�������i�e�{�C�X�̐M���������s���A�X�e���IMIX����j
void CVoiceCtrl::processSignalLR(int size, SIG_LR* pSig, bool* pProcessed) {
	// �{�C�X�̐M�������m�����o�b�t�@�ɏo�͂��A�X�e���IMIX���Ă����B
	CACHE_ALIGN16 SIGREAL monoSig[MAX_SIGBUF];
	int processedVoiceNum = 0;
	for (int i = 0; i<mVoiceNum; i++) {
		// �{�C�X���ɐM�����������s
		if (!mpVoices[i]->isBusy()) continue;	// �������łȂ���΁A�����˗����Ȃ�
		bool voiceProcessed = false;
		mpVoices[i]->processSignal(size, monoSig, &voiceProcessed);	// �M������
		if (!voiceProcessed) continue;			// �M����������Ă��Ȃ��̂ŁA�������Ȃ�

		// mono�o�͂��X�e���IMIX
		// ���߂Ă�MIX�����Ȃ�A�܂��o�̓o�b�t�@�𖳉��ɂ���
		if (processedVoiceNum == 0) for (int j = 0; j<size; j++) pSig[j].l = pSig[j].r = 0;
		float pan = mpVoices[i]->getPan();
		float lGain = (pan<0.5f) ? 1.f : (1.f - pan)*2.f;
		float rGain = (pan>0.5f) ? 1.f : pan*2.f;
		for (int j = 0; j<size; j++) {
			pSig[j].l += monoSig[j] * lGain;
			pSig[j].r += monoSig[j] * rGain;
		}
		processedVoiceNum++;
		*pProcessed = true;
	}
}

