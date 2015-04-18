/* タイマ関連 */

#include "bootpack.h"

#define PIT_CTRL    0x0043
#define PIT_CNT0    0x0040

struct TIMERCTL timerctl;

/**
 * PIT の初期化．割り込み周期を設定する．
 * 1秒間に100回割り込みが入る
 */
void init_pit(void){
    io_out8(PIT_CTRL, 0x34);    // 割り込み周期設定
    io_out8(PIT_CNT0, 0x9c);    // 割り込み周期の下位 8bit
    io_out8(PIT_CNT0, 0x2e);    // 割り込み周期の上位 8bit
    timerctl.count = 0;         // 経過時間の初期化
    timerctl.timeout = 0;       // タイムアウトまでの時間初期化
    return;
}

/**
 * 割込みハンドラ
 * 時間を測る
 */
void inthandler20(int *esp){
    io_out8(PIC0_OCW2, 0x60);   // PICに，IRQ-00の受付完了を通知
    timerctl.count++;           // 時間を進める
    if (timerctl.timeout > 0){  // タイムアウトが設定されている場合
        timerctl.timeout--;     // タイムアウトまでの時間を進める
        if (timerctl.timeout == 0){
            // タイムアウトになった場合，FIFOバッファに通知する
            fifo8_queue(timerctl.fifo, timerctl.data);
        }
    }
    return;
}

void settimer(unsigned int timeout, struct FIFO8 *fifo, unsigned char data){
    int eflags;
    eflags = io_load_eflags();     // EFLAGSレジスタのロード()一時退避)
    io_cli();                        // 割り込みを禁止する(設定が終わる前にIRQ0の割り込みが来るのを防ぐ)
    timerctl.timeout = timeout;    // タイムアウト時間を設定
    timerctl.fifo = fifo;          // FIFOバッファ設定
    timerctl.data = data;          // バッファへ送信するデータ設定
    io_store_eflags(eflags);       // EFLAGSレジスタの格納(元に戻す)
    return;
}
