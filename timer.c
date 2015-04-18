/* タイマ関連 */

#include "bootpack.h"

#define PIT_CTRL    0x0043
#define PIT_CNT0    0x0040

/**
 * PIT の初期化．割り込み周期を設定する．
 * 1秒間に100回割り込みが入る
 */
void init_pit(void){
    io_out8(PIT_CTRL, 0x34);    // 割り込み周期設定
    io_out8(PIT_CNT0, 0x9c);    // 割り込み周期の下位 8bit
    io_out8(PIT_CNT0, 0x2e);    // 割り込み周期の上位 8bit
    return;
}

/**
 * 割込みハンドラ
 */
void inthandler20(int *esp){
    io_out8(PIC0_OCW2, 0x60);   // PICに，IRQ-00の受付完了を通知
    return;
}
