/* タイマ関連 */

#include "bootpack.h"

struct TIMERCTL timerctl;

/**
 * PIT の初期化．割り込み周期を設定する．
 * 1秒間に100回割り込みが入る
 */
void init_pit(void){
    int i;
    io_out8(PIT_CTRL, 0x34);    // 割り込み周期設定
    io_out8(PIT_CNT0, 0x9c);    // 割り込み周期の下位 8bit
    io_out8(PIT_CNT0, 0x2e);    // 割り込み周期の上位 8bit
    timerctl.count = 0;         // 経過時間の初期化
    for (i=0; i<MAX_TIMER; i++){
        // すべてのタイマを未使用にセット
        timerctl.timer[i].flags = TIMER_UNUSED;
    }
    return;
}

/**
 * 未使用のタイマを探索し，返す
 */
struct TIMER *timer_alloc(void) {
    int i;
    for (i=0; i<MAX_TIMER; i++){
        if (timerctl.timer[i].flags == 0){
            // 未使用のタイマを返す
            timerctl.timer[i].flags = TIMER_FLAG_ALLOC;
            return &timerctl.timer[i];
        }
    }
    return 0;
}

/**
 * タイマを未使用にする
 */
void timer_free(struct TIMER *timer){
    timer->flags = TIMER_UNUSED;
    return;
}

/**
 * タイマの使用準備を行う
 */
void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}

/**
 * タイマにタイムアウトを設定する
 */
void timer_settime(struct TIMER *timer, unsigned int timeout){
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAG_USING;
    return;
}

/**
 * 割込みハンドラ
 * 時間を測る
 */
void inthandler20(int *esp){
    int i;
    io_out8(PIC0_OCW2, 0x60);   // PICに，IRQ-00の受付完了を通知
    timerctl.count++;           // 時間を進める
    for(i=0; i<MAX_TIMER; i++){
        if (timerctl.timer[i].flags == TIMER_FLAG_USING){
            if (timerctl.timer[i].timeout <= timerctl.count){
                // タイムアウトしたら，FIFO に通知
                timerctl.timer[i].flags = TIMER_FLAG_ALLOC;
                fifo8_queue(timerctl.timer[i].fifo, timerctl.timer[i].data);
            }
        }
    }
    return;
}
