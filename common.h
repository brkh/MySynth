#pragma once
// common.h

#pragma warning(disable:4996)

// �f�o�b�O�E�B���h�E�̕\���L���ݒ�
// 1=�L�� 0=����
// ���ӁF
// �E�v���O�C�������J����ꍇ�́A�K���f�o�b�O�E�B���h�E�͖����ɂ��ĉ������B
// �E�z�X�g��ASIO���g���Ă���ꍇ�A�X���b�h�ԂŃf�b�h���b�N���������A�������ɂȂ�\��������܂��B
// �@���̏ꍇ�A�z�X�g��Audio�ݒ��"MME"�ɐݒ肷�邩�A�E�B���h�E���I�t�ɂ��ĉ������B
#define _MYDEBUG_ON		1

// SSE���p�̂��߂̃�������̃A���C�������g�ݒ�(16�o�C�g���E)
#define	CACHE_ALIGN16	__declspec(align(16))

// �M���l�̌^�̒�`�A32bit���������Ƃ���
typedef float SIGREAL;

// �X�e���I�̐M���l
struct SIG_LR {
	SIGREAL l;	// ��
	SIGREAL r;	// �E
};

// ��Ɨp�M���l�o�b�t�@�̍ő�T���v��
// 288�́A�P��UA-25 EX�̃f�t�H���g�̃T�C�Y�ɍ��킹�������B
#define	MAX_SIGBUF	(288)

// �ėp�}�N��
#define MIN(a,b)	((a)<(b)?(a):(b))
#define MAX(a,b)	((a)>(b)?(a):(b))
#define PI		3.141592653589793238462643383279f

