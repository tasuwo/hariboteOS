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
    timerctl.next = 0xffffffff; // 最初は作動中のタイマがないため
    for (i=0; i<MAX_TIMER; i++){
        // すべてのタイマを未使用にセット
        timerctl.timers0[i].flags = TIMER_UNUSED;
    }
    return;
}

/**
 * 未使用のタイマを探索し，返す
 */
struct TIMER *timer_alloc(void) {
    int i;
    for (i=0; i<MAX_TIMER; i++){
        if (timerctl.timers0[i].flags == 0){
            // 未使用のタイマを返す
            timerctl.timers0[i].flags = TIMER_FLAG_ALLOC;
            return &timerctl.timers0[i];
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
 * タイムアウトの早い順でタイマをセットしていかなくてはならない
 */
void timer_settime(struct TIMER *timer, unsigned int timeout){
    int e,i,j;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAG_USING;

    // 割り込み禁止
    e = io_load_eflags();
    io_cli;

    // 挿入位置を探索
    for (i=0; i<timerctl.using; i++){
        if (timerctl.timers[i]->timeout >= timer->timeout){
            break;
        }
    }

    // 挿入位置から後ろの要素をずらす
    for (j=timerctl.using; j>i; j--){
        timerctl.timers[j] = timerctl.timers[j - 1];
    }

    // 稼働中のタイマ数を増やす
    timerctl.using++;

    // タイマを挿入する
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;

    // 割り込みを戻す
    io_store_eflags(e);

    return;
}

/**
 * 割込みハンドラ
 * 時間を測る
 */
void inthandler20(int *esp){
    int to_timers,j;
    io_out8(PIC0_OCW2, 0x60);   // PICに，IRQ-00の受付完了を通知
    timerctl.count++;           // 時間を進める

    /*
     * 使用中のタイマの中で，一番短いものの時刻が next に格納されている．
     * なので，これに満たなければ割り込みを終了する．
     */
    if (timerctl.next > timerctl.count){
        return;
    }

    /*
     * next に格納されている割り込み時刻と同様のタイマを探索する
     */
    for(to_timers=0; to_timers<timerctl.using; to_timers++){
        if (timerctl.timers[to_timers]->timeout > timerctl.count){
            break;
        }
        // タイムアウトしていたら，FIFO に通知
        timerctl.timers[to_timers]->flags = TIMER_FLAG_ALLOC;
        fifo8_queue(timerctl.timers[to_timers]->fifo, timerctl.timers[to_timers]->data);
    }

    // i個のタイマがタイムアウトしたので，そのための処理を行う
    // 稼働中のタイマ数を減らす
    timerctl.using -= to_timers;

    // 配列の頭から，タイムアウトしたタイマ分要素をずらす
    // すなわち，タイムアウトしたタイマを配列から除く
    for (j=0; j<timerctl.using; j++){
        timerctl.timers[j] = timerctl.timers[to_timers + j];
    }

    if (timerctl.using > 0){
        // まだ稼働中のタイマがあれば，いちばん早いタイマを next に設定
        timerctl.next = timerctl.timers[0]->timeout;
    } else {
        // もうタイマがなければ，初期化
        timerctl.next = 0xffffffff;
    }
    return;
}
