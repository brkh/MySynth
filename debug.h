#pragma once
// debug.h
// デバッグルーチン
// _MYDEBUG_ONマクロが1に定義されていれば、デバッグウィンドウ等が有効化される
// _MYDEBUG_ONマクロが0に定義されていれば、デバッグ関連のソースコードはされず、ウィンドウも開かない。

#if !_MYDEBUG_ON
// デバッグ処理無効化
#define startDbgWin(a)
#define endDbgWin()
#define edPrintf(...)
#define dispClock2(a,b)
#define dbgDraw(a,b)
#define _START_CLOCK()
#define _DISP_CLOCK(size)

#else
// デバッグ処理有効化
#ifdef __cplusplus
extern "C"{
#endif 

	void startDbgWin(HINSTANCE hInst);				// デバッグ処理開始
	void endDbgWin(void);							// デバッグ処理終了
	int edPrintf(const char *first, ...);			// 文字列出力
	void dispClock2(int nSample, long long clock);	// 1サンプル辺りのクロック等をディスプレイ上部に強制表示
	int dbgDraw(float l, float r);					// 信号値をデバッグウィンドウ上にプロット（ヘボい）


#ifdef __cplusplus
}
#endif 

// クロック計測・表示マクロ
// 注意
// ・このマクロペアは、実行されるコードの中で一組だけである事。
#define	_START_CLOCK()		long long _start_=_get_clock();
#define	_DISP_CLOCK(size)	long long _end_=_get_clock2();dispClock2(size,_end_-_start_);

inline long long _get_clock() {
	// RDTSCで計測されるクロックは実際のコアクロックではなく、FSBをベースにしたもののようだ。
	// SpeedStepで実際のコアクロックが下がっても、RDTSCでは一定の値しか取れない。
	// つまり、SpeedStep等でコアクロックが変化すると、コード実行のパフォーマンスが上がったり、
	// 下がったりするように見えるが、それは見せ掛けなので注意が必要。
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

