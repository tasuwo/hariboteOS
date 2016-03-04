/**
 * keyboard.c
 * キーボード関連
 */

#include "bootpack.h"

struct FIFO8 keyfifo;

/**
 * KBCがデータ送信可能になるのを待つ
 */
void wait_KBC_sendready(void){
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

/**
 * KBCの初期化
 */
void init_keyboard(void){
    wait_KBC_sendready();                       // 待ち状態
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);    // モードを設定することを通知
    wait_KBC_sendready();                       // 待ち状態
    io_out8(PORT_KEYDAT, KBC_MODE);             // モード設定(マウス使用モード)
    return;
}

/**
 * キボード用の割込みハンドラ
 * @param esp ？？？
 */
void inthandler21(int *esp){
    unsigned char data;

    /* 受付終了をPICに通知する */
    io_out8(PIC0_OCW2, 0x61);
    /* キーコードを取得する */
    data = io_in8(PORT_KEYDAT);
    /* データをキューに格納する */
    fifo8_queue(&keyfifo, data);

    return;
}
