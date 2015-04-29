// synthBase.cpp
// シンセ本体に必要となる、ボイスコントロール制御などの実装

#include <windows.h>
#include <math.h>
#include <list>
#include "common.h"
#include "debug.h"
#include "synthBase.h"


//////////////////////////////////////////
// CVoiceCtrl ボイスコントローラクラス

CVoiceCtrl::CVoiceCtrl() {
	iKeyMode = KM_POLY;
	iCurVoiceNo = 0;
	kbdInfo = NULL;
	mUnisonNum = 1;
}

// キーボード情報管理クラスの設定
void CVoiceCtrl::setKeyboardInfo(CKeyboardInfo* p) {
	kbdInfo = p;
}

// ボイス数の設定
void CVoiceCtrl::setVoiceNum(int n) {
	mVoiceNum = n;
	mPolyNum = n;
}

// 各ボイスの設定
void CVoiceCtrl::setVoice(int no, IVoice* pV) {
	mpVoices[no] = pV;
	pV->setNo(no);
}

// キーモード（POLY/MONO/MONO2）の設定
void CVoiceCtrl::setVoiceMode(int mode) {
	iKeyMode = mode % 3;	// 0,1,2
	// 不要ボイスは、リリースし、カレントボイスを先頭に
	for (int i = (iKeyMode == KM_POLY) ? mPolyNum : mUnisonNum; i<mVoiceNum; i++) {
		if (mpVoices[i]->isKeyOn()) mpVoices[i]->release(0);
		monVoices.remove(mpVoices[i]);
	}
	iCurVoiceNo = -1;

}

// ユニゾンボイス数の設定
void CVoiceCtrl::setUnisonVoiceNum(int n) {
	n = MIN(mVoiceNum, MAX(1, n));
	if (n == mUnisonNum) return;
	// ユニゾンボイス数設定
	mUnisonNum = n;

	// ユニゾンボイス数に応じて、ボイス間のデチューン値、パン値を計算し、設定する
	// ・ボイス間のデチューン幅は一律、10centとしてみる。
	// ・ボイスパンは左いっぱい(=0)から右いっぱい(=1)に均等に散らす
	// ここでのデチューン値やパン値の決め方は単なる一例。
	float detSpread = 10.f;
	for (int i = 0; i<mVoiceNum; i++) {
		int vn = i%mUnisonNum;
		float det = vn*detSpread - detSpread*(mUnisonNum - 1) / 2.f;
		float pan = (mUnisonNum == 1) ? 0.5f : vn*1.f / (mUnisonNum - 1);
		mpVoices[i]->setDetune(det);
		mpVoices[i]->setPan(pan);
	}

	// 不要ボイスは、リリースし、カレントボイスを先頭に
	for (int i = (iKeyMode == KM_POLY) ? mPolyNum : mUnisonNum; i<mVoiceNum; i++) {
		if (mpVoices[i]->isKeyOn()) mpVoices[i]->release(0);
		monVoices.remove(mpVoices[i]);
	}
	iCurVoiceNo = -1;
}

// 空きボイスのサーチ
IVoice* CVoiceCtrl::getNextOffVoice() {
	int curVoiceNo = iCurVoiceNo;
	for (int i = 0; i<mPolyNum; i++) {
		curVoiceNo++;
		if (curVoiceNo >= mPolyNum) curVoiceNo = 0;
		if (!mpVoices[curVoiceNo]->isKeyOn()) return mpVoices[curVoiceNo];
	}
	// どれもオンだった。
	return NULL;
}

// 鍵盤情報と各ボイスの状態等から、トリガー/リリースを制御する
void CVoiceCtrl::controlVoiceTrigger() {

	if (iKeyMode == KM_POLY)	controlVoicePoly();
	else						controlVoiceMono();
}

// ポリモード時のトリガー/リリースを制御
void CVoiceCtrl::controlVoicePoly() {
	//// まずキーリリース処理
	// オンボイスリストからみて、キーボードテーブル上でノートオフ(ベロシティ値=0)に
	// なっているかをチェック→ノートオフならリリースする
	for (std::list<IVoice*>::iterator v = monVoices.begin(); v != monVoices.end();) {
		if (kbdInfo->getVelocity((*v)->getNoteNo()) == 0) {
			(*v)->release(0);
			v = monVoices.erase(v);
		}
		else v++;
	}

	//// トリガー処理
	// 新規に押鍵されたキーを対象に処理を行う。
	int num = MIN(mPolyNum, kbdInfo->getOnKeyNum());	// 処理する最大ノート数はポリ数と押鍵キー数のどちらか少ない方
	for (int i = 0; i<num; i++) {
		int noteNo = kbdInfo->getNewOnKeyNN(i);		// 新規押鍵キーでi番目に新しいキーのノートNO（NEWキーフラグ付き）
		if (noteNo == -1) break;							// 新規押鍵キーはなかった→これ以上古い新規押鍵もないはずなので処理終了

		// 既に同じノートNoを発音しているボイスがあれば、リリースする。
		// 例えばアルペジエータでゲートタイム１００％とした時に、この対応がなければ、音がどんどん重なっていく。
		// これは、このアルゴリズムではnote on/offは鍵盤の状態を変更するだけで、もし同一時刻にoff→onの順で同時にきた場合、
		// 結果的にnoteoffは無視される事になるため。
		// このような事にならないために、同一ノートの複数ボイス発音は不可とする。
		for (std::list<IVoice*>::iterator v = monVoices.begin(); v != monVoices.end();) {
			if ((*v)->getNoteNo() == noteNo) {
				(*v)->release(0);
				v = monVoices.erase(v);
			}
			else v++;
		}

		// ユニゾンボイスの数だけ、トリガー処理
		IVoice* pUnisonMasterVoice;
		for (int u = 0; u < mUnisonNum; u++) {
			//// トリガーするボイスの決定
			// キーオフされているボイスを取得。もし全部ビジーだったら、一番古くからオンになっているボイスを取得
			// ※ただし、ベース音は除くなどの工夫の余地はある
			IVoice* v = getNextOffVoice();
			if (v == NULL) {
				v = monVoices.front();	// オン中で一番古いボイス
				if (v == NULL) return;
				monVoices.pop_front();
			}
			iCurVoiceNo = v->getNo();
			if (u == 0) pUnisonMasterVoice = v; // ユニソンマスターボイスの退避

			// ノートNO等の設定とトリガー
			v->setNoteInfo(noteNo, kbdInfo->getVelocity(noteNo));
			v->setUnisonInfo(pUnisonMasterVoice, mUnisonNum, u);
			v->trigger();

			// オンボイスリストへ追加
			monVoices.push_back(v);
		}
	}
}

// モノモード時のトリガー/リリースを制御
void CVoiceCtrl::controlVoiceMono() {
	// なにもキーがおさえられていなければ、現在のオンボイスをリリースして終わり
	if (kbdInfo->getOnKeyNum() == 0) {
		for (int i = 0; i < mUnisonNum; i++) {
			monVoices.remove(mpVoices[i]);
			if (mpVoices[i]->isKeyOn()) mpVoices[i]->release(0);
		}
		mmonoCurrentVelocity = 0;
		return;
	}

	// なにかキーがおさえられているので一番新しい鍵盤をとってきて、発音が必要か判定
	// 1.新規の押鍵なら無条件に発音
	// 2.新規ではない場合、ボイスがオン中でないか、オン中でもノートNOが違えば発音
	bool bProcess = false;
	int noteNo = kbdInfo->getNewOnKeyNN(0);
	if (noteNo != -1) {
		bProcess = true;
		mmonoCurrentVelocity = kbdInfo->getVelocity(noteNo);
	}
	else {
		// 新規ではない→何かキーが離された結果、発音される事になったノート
		// このベロシティは離されたキーと同じとする。よって、mmonoCurrentVelocityは更新しない。
		noteNo = kbdInfo->getOnKeyNN(0);
		if (!mpVoices[0]->isKeyOn() || mpVoices[0]->getNoteNo() != noteNo) bProcess = true;
	}

	if (!bProcess) return;	// 発音不要

	// 発音処理
	IVoice* pUnisonMasterVoice = mpVoices[0];	// ユニゾンマスターは常にボイス番号ゼロ
	if (iKeyMode == KM_MONO) {
		// モノモード時
		// 必ずトリガー
		for (int i = 0; i < mUnisonNum; i++) {
			IVoice* v = mpVoices[i];
			// ノートNO等の設定とトリガー
			v->setNoteInfo(noteNo, mmonoCurrentVelocity);
			v->setUnisonInfo(pUnisonMasterVoice, mUnisonNum, 0);
			v->trigger();

			// オンボイスリストへ追加 (すでに登録されている可能性があるので、一度削除してから追加)
			monVoices.remove(v);
			monVoices.push_back(v);
		}
	}
	else if (iKeyMode == KM_MONO2) {
		// レガートモノモード時
		// 現在キーオフだった場合のみトリガー
		for (int i = 0; i < mUnisonNum; i++) {
			IVoice* v = mpVoices[i];
			// ノートNO等の設定とトリガー
			v->setNoteInfo(noteNo, mmonoCurrentVelocity);
			if (!v->isKeyOn()) {
				// 現在キーオフ→トリガーし、オンボイスリストへ追加
				v->setUnisonInfo(pUnisonMasterVoice, mUnisonNum, 0);
				v->trigger();
				monVoices.push_back(v);
			}
			else {
				// 現在オン→トリガーしない
			}
		}
	}
}

// 信号処理（各ボイスの信号処理を行い、ステレオMIXする）
void CVoiceCtrl::processSignalLR(int size, SIG_LR* pSig, bool* pProcessed) {
	// ボイスの信号をモノラルバッファに出力し、ステレオMIXしていく。
	CACHE_ALIGN16 SIGREAL monoSig[MAX_SIGBUF];
	int processedVoiceNum = 0;
	for (int i = 0; i<mVoiceNum; i++) {
		// ボイス毎に信号処理を実行
		if (!mpVoices[i]->isBusy()) continue;	// 発音中でなければ、処理依頼しない
		bool voiceProcessed = false;
		mpVoices[i]->processSignal(size, monoSig, &voiceProcessed);	// 信号処理
		if (!voiceProcessed) continue;			// 信号処理されていないので、何もしない

		// mono出力をステレオMIX
		// 初めてのMIX処理なら、まず出力バッファを無音にする
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

