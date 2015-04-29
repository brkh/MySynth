#pragma once
// debug.h
// �f�o�b�O���[�`��
// _MYDEBUG_ON�}�N����1�ɒ�`����Ă���΁A�f�o�b�O�E�B���h�E�����L���������
// _MYDEBUG_ON�}�N����0�ɒ�`����Ă���΁A�f�o�b�O�֘A�̃\�[�X�R�[�h�͂��ꂸ�A�E�B���h�E���J���Ȃ��B

#if !_MYDEBUG_ON
// �f�o�b�O����������
#define startDbgWin(a)
#define endDbgWin()
#define edPrintf(...)
#define dispClock2(a,b)
#define dbgDraw(a,b)
#define _START_CLOCK()
#define _DISP_CLOCK(size)

#else
// �f�o�b�O�����L����
#ifdef __cplusplus
extern "C"{
#endif 

	void startDbgWin(HINSTANCE hInst);				// �f�o�b�O�����J�n
	void endDbgWin(void);							// �f�o�b�O�����I��
	int edPrintf(const char *first, ...);			// ������o��
	void dispClock2(int nSample, long long clock);	// 1�T���v���ӂ�̃N���b�N�����f�B�X�v���C�㕔�ɋ����\��
	int dbgDraw(float l, float r);					// �M���l���f�o�b�O�E�B���h�E��Ƀv���b�g�i�w�{���j


#ifdef __cplusplus
}
#endif 

// �N���b�N�v���E�\���}�N��
// ����
// �E���̃}�N���y�A�́A���s�����R�[�h�̒��ň�g�����ł��鎖�B
#define	_START_CLOCK()		long long _start_=_get_clock();
#define	_DISP_CLOCK(size)	long long _end_=_get_clock2();dispClock2(size,_end_-_start_);

inline long long _get_clock() {
	// RDTSC�Ōv�������N���b�N�͎��ۂ̃R�A�N���b�N�ł͂Ȃ��AFSB���x�[�X�ɂ������̂̂悤���B
	// SpeedStep�Ŏ��ۂ̃R�A�N���b�N���������Ă��ARDTSC�ł͈��̒l�������Ȃ��B
	// �܂�ASpeedStep���ŃR�A�N���b�N���ω�����ƁA�R�[�h���s�̃p�t�H�[�}���X���オ������A
	// ���������肷��悤�Ɍ����邪�A����͌����|���Ȃ̂Œ��ӂ��K�v�B
	LARGE_INTEGER clock;
	SetThreadAffinityMask(GetCurrentThread(), 1);
	__asm {
		cpuid
			rdtsc
			mov clock.LowPart, eax
			mov clock.HighPart, edx
	}
	return clock.QuadPart;
}

inline long long _get_clock2() {
	LARGE_INTEGER clock;
	__asm {
		rdtsc
			mov clock.LowPart, eax
			mov clock.HighPart, edx
	}
	return clock.QuadPart;
}

#endif

