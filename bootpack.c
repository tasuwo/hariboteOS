/**
 * bootpack.c
 * main
 */

#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

struct MOUSEINFO{
    unsigned char phase,buf[3];
    int x, y, btn;
};

/* main */
void HariMain(void)
{
    /* asmhead.nasでメモした値を取得 */
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSEINFO minfo;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, key;

    /* 初期化処理 */
    init_gdtidt();                                          // GDT，IDT初期化
    init_pic();                                             // PIC初期化
    io_sti();                                               // 割り込み禁止を解除
    fifo8_init(&keyfifo, 32, keybuf);                       // キーボード用バッファ初期化
    fifo8_init(&mousefifo, 32, mousebuf);                   // マウス用バッファ初期化
    io_out8(PIC0_IMR, 0xf9);                                // 割り込み許可：キーボードとPIC1(11111001)
    io_out8(PIC1_IMR, 0xef);                                // 割り込み許可：マウス(11101111)
    init_keyboard();                                        // KBCの初期化(マウス使用モードに設定)
    init_palette();                                         // パレット初期化
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);  // OS初期画面描画
    enable_mouse(&minfo);                                   // マウス有効化

    /* マウスカーソルの描画 */
    mx = (binfo->scrnx - 16) / 2;                                           // 座標計算(画面中央)
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);                               // mcursorにマウスを初期化
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);    // mcursorを描画
    sprintf(s, "(%d, %d)", mx, my);                                         // 座標をメモリに書き込む
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);         // 座標の描画

    /* CPU休止 */
    while(1){
        /* 割り込み禁止にする */
        io_cli();

        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
            // 割り込みがなければ
            /* 割り込みフラグをセットし，hltする */
            io_stihlt();
        } else {
            /* キーボード，マウスの順に調べる */
            if (fifo8_status(&keyfifo) != 0) {
                /* キーを読み込む */
                key = fifo8_dequeue(&keyfifo);
                /* 割り込みフラグをセットする */
                io_sti();
                /* キーの表示 */
                sprintf(s, "%02X", key);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                /* データの取得 */
                key = fifo8_dequeue(&mousefifo);
                /* 割り込みフラグをセットする */
                io_sti();

                // マウスの座標描画
                if (mouse_decode(&minfo, key) != 0){
                    /* データが3バイト揃ったので表示 */
                    sprintf(s, "[lcr %4d %4d]", minfo.x, minfo.y);
                    if ((minfo.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((minfo.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((minfo.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                }

                /* マウス移動 */
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* マウス消す */
                    mx += minfo.x;
                    my += minfo.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16) {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16) {
                        my = binfo->scrny - 16;
                    }
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); /* 座標消す */
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); /* 座標書く */
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); /* マウス描く */
            }
        }
    }
}

#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064  // KBC
#define KEYSTA_SEND_NOTREADY    0x02    // KBCの制御命令受付可能判定用
#define KEYCMD_WRITE_MODE       0x60    // モード設定のためのモード
#define KBC_MODE                0x47    // マウスを利用するためのモード

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

#define KEYCMD_SENDTO_MOUSE     0xd4    // マウスへの命令送信
#define MOUSECMD_ENABLE         0xf4    // マウスへの有効化命令

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
             * 1byte目：移動に反応する．0～3桁
             * 2bute目：クリックに反応する．8～F桁
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
