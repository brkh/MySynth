#pragma once
// common.h

#pragma warning(disable:4996)

// デバッグウィンドウの表示有無設定
// 1=有り 0=無し
// 注意：
// ・プラグインを公開する場合は、必ずデバッグウィンドウは無しにして下さい。
// ・ホストがASIOを使っている場合、スレッド間でデッドロックが発生し、無応答になる可能性があります。
// 　その場合、ホストのAudio設定を"MME"に設定するか、ウィンドウをオフにして下さい。
#define _MYDEBUG_ON		1

// SSE利用のためのメモリ上のアラインメント設定(16バイト境界)
#define	CACHE_ALIGN16	__declspec(align(16))

// 信号値の型の定義、32bit浮動小数とする
typedef float SIGREAL;

// ステレオの信号値
struct SIG_LR {
	SIGREAL l;	// 左
	SIGREAL r;	// 右
};

// 作業用信号値バッファの最大サンプル
// 288は、単にUA-25 EXのデフォルトのサイズに合わせただけ。
#define	MAX_SIGBUF	(288)

// 汎用マクロ
#define MIN(a,b)	((a)<(b)?(a):(b))
#define MAX(a,b)	((a)>(b)?(a):(b))
#define PI		3.141592653589793238462643383279f

