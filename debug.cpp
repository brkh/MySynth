// debug.cpp
// デバッグウィンドウ等のデバッグ用ルーチン

#include <windows.h>
#include <math.h>
#include "stdio.h"
#include "common.h"
#include "debug.h"

#if !_MYDEBUG_ON
// デバッグ用ウィンドウを使わない
#else
// デバッグ用ウィンドウを使う

#pragma warning(disable:4996)

#define	CLASS_DEBUG		"CLASS_DEBUG"
#define	TITLE			"Debug Monitor"

LRESULT CALLBACK WNDebugProc(HWND,UINT,WPARAM,LPARAM);
static HWND hWndDebug;
static HWND hDebugEdit;

static HINSTANCE ghInstance;
static void initDebugWindow(HINSTANCE hInstance);
static void destroyDebugWindow(void);


// 初期化等をスレッドセーフに行う為のクリティカルセクションクラス
class CMyCS{
public:
	CMyCS(){
		InitializeCriticalSection(&cs);
	};
	virtual ~CMyCS(){
		DeleteCriticalSection(&cs);
	};
	void enter(){
		EnterCriticalSection(&cs);
	};
	void leave(){
		LeaveCriticalSection(&cs);
	};
private:
	CRITICAL_SECTION cs;
};

static CMyCS g_cs;				// 使用カウント同期用クリティカルセクション
static int g_DbgStartCount=0;	// 使用カウント

// デバッグ開始
void startDbgWin(HINSTANCE hInst){
	g_cs.enter();
	if(g_DbgStartCount==0) initDebugWindow(hInst);
	g_DbgStartCount++;
	g_cs.leave();
}

// デバッグ終了
void endDbgWin(){
	g_cs.enter();
	g_DbgStartCount--;
	if(g_DbgStartCount==0) destroyDebugWindow();
	g_cs.leave();
}


// 信号値をデバッグウィンドウにプロット
int dbgDraw(float l,float r){
	static float x=0;

	int cy=200;
	HDC hDC=GetDC(hDebugEdit);
	SetPixel(hDC,(int)x,(int)(cy-l*200),RGB(255,0,0));
	SetPixel(hDC,(int)x,(int)(cy-r*200),RGB(0,0,255));
	SetPixel(hDC,(int)x,cy,RGB(0,0,0));

	x+=0.05f;
	if(x>1000){
		Rectangle(hDC,0,0,1020,400);
		x=0;
	}
	ReleaseDC(hDebugEdit,hDC);
	return 0;
}

// 文字出力
int edPrintf(const char *first,...)
{
	char szWork[4096];
	va_list argList;

	va_start(argList,first);
	vsprintf(szWork,first,argList);
	va_end(argList);

	hDebugEdit = GetWindow(hWndDebug,GW_CHILD);
	if(hDebugEdit){
		int max = SendMessage(hDebugEdit,EM_GETLIMITTEXT,0,0);
		int cur = SendMessage(hDebugEdit,WM_GETTEXTLENGTH,0,0);
		if(cur<max-1){
			SendMessage(hDebugEdit,EM_SETSEL,-1,-1);
			SendMessage(hDebugEdit,EM_REPLACESEL,0,(LPARAM)szWork);
		}
	}
	return 0;
}

// デバッグウィンドウオープン
static void initDebugWindow(HINSTANCE hInstance){
	ghInstance=hInstance;

	WNDCLASS	WndClass ;					// ウィンドウクラス構造体

	WndClass.style			= CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS ;
	WndClass.lpfnWndProc	= (WNDPROC)WNDebugProc;
	WndClass.cbClsExtra		= 0 ;
	WndClass.cbWndExtra		= sizeof(HHOOK) ;
	WndClass.hInstance		= hInstance ;
	WndClass.hIcon			= LoadIcon( NULL, IDI_APPLICATION);
	WndClass.hCursor		= LoadCursor( NULL, IDC_ARROW ) ;
	WndClass.hbrBackground	= (HBRUSH)GetStockObject( WHITE_BRUSH ) ;
	WndClass.lpszMenuName	= NULL ;
	WndClass.lpszClassName	= CLASS_DEBUG;
	if( !RegisterClass( &WndClass )) {
		goto EXIT;
	}

	// ウィンドウ作成
	if( !( hWndDebug = CreateWindow( CLASS_DEBUG, TITLE,
					WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					NULL, NULL, hInstance, NULL))){
		goto EXIT;
 	}

   ShowWindow( hWndDebug, SW_SHOWNA );
   UpdateWindow( hWndDebug );

EXIT:
	return;
}

// デバッグウィンドウクローズ
static void destroyDebugWindow(){
	DestroyWindow(hWndDebug);
	UnregisterClass(CLASS_DEBUG,ghInstance);
}


// デバッグウィンドウ関数
LRESULT CALLBACK WNDebugProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message){
		case WM_CREATE:
			// エディットボックス作成
			hDebugEdit = CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE,0,0,0,0,hWnd,0,ghInstance,NULL);
			break;

		case WM_SIZE:
			
			// エディットボックス変更
			MoveWindow(hDebugEdit,0,50,LOWORD(lParam),HIWORD(lParam),true);
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
   }
   return 0;
}


// クロック計測結果を簡易集計し、表示する。
void dispClock2(int nSample,long long clock){
	//// オーバーヘッドクロックの計測
	static int overHeadClock;
	if(overHeadClock == 0) {
		long long _clock_;
		_clock_ = _get_clock();
		overHeadClock = (int)(_get_clock2()-_clock_);
		_clock_ = _get_clock();
		overHeadClock = (int)(_get_clock2()-_clock_);
		edPrintf("overHeadClock=%d\n",overHeadClock);
	}
	clock -= overHeadClock;

	//// 今回のクロック数を表示
	HDC hDC;
	static float curCl=0;
	char szWork[255];
	float newCl = (float)clock/nSample;
	curCl = newCl;
	sprintf(szWork,"【瞬時】:%6.1f [clk/サンプル]   ",newCl);
	for(int i=9;i<10+4;i++)if(szWork[i]==' ') szWork[i]='0';

	hDC = GetDC(hDebugEdit);
	TextOut(hDC,0,4,szWork,lstrlen(szWork));
	ReleaseDC(hDebugEdit, hDC);

	//// 1秒あたりの平均クロックの計算。
	static const int BUF_SIZE=1024;
	static int sampleSum=0,maxSample,nn,printNo;
	static long long clockSum=0;
	static float maxClock;
	static float clocks[BUF_SIZE];
	static int samples[BUF_SIZE];
	char szWork2[256];
	sampleSum += nSample;
	clockSum += clock;
	samples[nn] = nSample;
	clocks[nn] = (float)clock/nSample;
	if(maxClock < clocks[nn]){
		maxClock = clocks[nn];
		maxSample = nSample;
	}
	nn++;

	//// 1秒経過したか、バッファがいっぱいなら、平均クロックの表示
	if(sampleSum>44100 || nn >= BUF_SIZE){
		// 平均
		float aveClock = (float)clockSum/sampleSum;
		float aveSample = (float)sampleSum/nn;

#if 0
		// 値をCSV形式で出力（ＥＸＣＥＬでグラフ化用）
		printNo++;
		if(printNo % 30 == 0){
		if( GetKeyState( VK_SHIFT) & 0x80 ){
			for(int i=0;i<nn;i++)  edPrintf("%f , %d\n",clocks[i],samples[i]);
		}
#endif

		// 分散値も出す(あまり意味はない)
		float v=0;
		for(int i=0;i<nn;i++)  {
			float a=(clocks[i]-aveClock)/aveClock;
			a = fabs(a);
			v += a;
		}
		v /= (float)nn;
		v *= 100;

		// 平均とあまりに乖離している(平均の2倍)ものは除いて新たな平均をつくる
		float newAve=0;
		int excludeNum=0;
		for(int i=0;i<nn;i++)  {
			float a=(clocks[i]-aveClock)/aveClock;
			a = fabs(a);
			if(a>1.0)excludeNum++;
			else newAve += clocks[i];
		}
		newAve /= (nn-excludeNum);

		// 出力
		sprintf(szWork2,"【平均】:%.1f [clk/サンプル]  極端値除く:%.1f  ワースト:%.1f  分散:%3.1f BlockDataサイズ:%.1f[サンプル]      ",aveClock,newAve,maxClock,v,aveSample);
		hDC = GetDC(hDebugEdit);
		TextOut(hDC,250,4,szWork2,lstrlen(szWork2));
		ReleaseDC(hDebugEdit, hDC);

		sampleSum=maxSample=nn=0;
		clockSum=0;
		maxClock = 0;
	}

}


#endif