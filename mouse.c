/**
 * mouse.c
 * マウス関連
 */

#include "bootpack.h"

struct FIFO8 mousefifo;

/**
 * マウス用の割込みハンドラ
 * @param esp ？？？
 */
void inthandler2c(int *esp){
    unsigned char data;

    /* スレーブ(PIC1)側に受付終了を通知する */
    io_out8(PIC1_OCW2, 0x64);
    /* マスタ(PIC0)側に受付終了を通知する */
    io_out8(PIC0_OCW2, 0x62);
    /* データをを取得する */
    data = io_in8(PORT_KEYDAT);
    /* データをキューに格納する */
    fifo8_queue(&mousefifo, data);

    return;
}

/**
 * マウスに有効化命令を送信する
 */
void enable_mouse(struct MOUSEINFO *mf){
    wait_KBC_sendready();                       // 待ち状態
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);  // マウスにデータを送信する
    wait_KBC_sendready();                       // 待ち状態
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);      // 有効化命令を送信
    mf->phase = 0;
    return; /* マウスからACK(0xfa)が送信されてくる */
}

int mouse_decode(struct MOUSEINFO *mf, unsigned char data){
    switch (mf->phase){
        case 0:
            if(data == 0xfa){
                mf->phase++;
            }
            return 0;
            break;
        case 1:
            /**
             * 1byte目：移動に反応する．0〜3桁
             * 2bute目：クリックに反応する．8〜F桁
             */
            if((data & 0xc8) == 0x08){
                mf->buf[0] = data;
                mf->phase++;
            }
            return 0;
            break;
        case 2:
            mf->buf[1] = data;
            mf->phase++;
            return 0;
            break;
        case 3:
            mf->buf[2] = data;
            mf->phase = 1;
            /* データの解釈 */
            mf->btn = mf->buf[0] & 0x07;
            mf->x = mf->buf[1];
            mf->y = mf->buf[2];
            if ((mf->buf[0] & 0x10) != 0) {
                mf->x |= 0xffffff00;
            }
            if ((mf->buf[0] & 0x20) != 0) {
                mf->y |= 0xffffff00;
            }
            mf->y = - mf->y; /* マウスではy方向の符号が画面と反対 */
            return 1;
            break;
    }
    return -1;
}
