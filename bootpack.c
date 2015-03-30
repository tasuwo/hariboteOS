/**
 * bootpack.c
 * main
 */

#include <stdio.h>
#include "bootpack.h"

/* main */
void HariMain(void)
{
    /* asmhead.nasでメモした値を取得 */
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], keybuf[32], mousebuf[128];
    int mx, my, key;
    unsigned int memtotal;
    struct MOUSEINFO minfo;
    struct MEMMANAGE *memman = (struct MEMMANAGE *) MEMMANAGE_ADDR;
    struct LYRCTL *lyrctl;
    struct LAYER *lyr_back, *lyr_mouse;
    unsigned char *buf_back, buf_mouse[256];

    /* 初期化処理 */
    init_gdtidt();                                          // GDT，IDT初期化
    init_pic();                                             // PIC初期化
    io_sti();                                               // 割り込み禁止を解除
    fifo8_init(&keyfifo, 32, keybuf);                       // キーボード用バッファ初期化
    fifo8_init(&mousefifo, 128, mousebuf);                   // マウス用バッファ初期化
    io_out8(PIC0_IMR, 0xf9);                                // 割り込み許可：キーボードとPIC1(11111001)
    io_out8(PIC1_IMR, 0xef);                                // 割り込み許可：マウス(11101111)

    init_keyboard();                                        // KBCの初期化(マウス使用モードに設定)
    enable_mouse(&minfo);                                   // マウス有効化
    memtotal = memtest(0x00400000, 0xbfffffff);             // 使用可能なメモリをチェックする
    memman_init(memman);                                    // メモリ管理用の構造体初期化
    /**
     * メモリの解放を行う
     * 0x00001000 〜 0x0009e000 は？？？
     * 0x00000000 〜 0x00400000 は，起動中やフロッピーディスクの内容記録，IDTやGDT等に使用されている
     * そのため，0x00400000 以降を解放する
     */
    memman_free(memman, 0x00001000, 0x0009e000);            // ？？？
    memman_free(memman, 0x00400000, memtotal - 0x00400000); // 0x00400000以降のメモリを解放

    /* 画面描画のための初期化処理 */
    init_palette();                                         // パレット初期化
    lyrctl = lyrctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    /* メモリ確保  */
    lyr_back = layer_alloc(lyrctl);
    lyr_mouse = layer_alloc(lyrctl);
    buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    layer_setbuf(lyr_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    layer_setbuf(lyr_mouse, buf_mouse, 16, 16, 99);
    /**/
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);  // OS初期画面描画
    init_mouse_cursor8(buf_mouse, 99);               // mcursorにマウスを初期化
    /**/
    layer_slide(lyrctl, lyr_back, 0, 0);
    mx = (binfo->scrnx - 16) / 2; 
    my = (binfo->scrny - 28 - 16) / 2;
    /**/
    layer_slide(lyrctl, lyr_mouse, mx, my);
    /**/
    layer_updown(lyrctl, lyr_back, 0);
    layer_updown(lyrctl, lyr_mouse, 1);
    /* 座標描画 */
    sprintf(s, "(%3d, %3d)", mx, my);                                         // 座標をメモリに書き込む
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);       // 座標の描画
    /* 容量描画 */
    sprintf(s, "memory %dMB  free : %dKB",
            memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    /* 描画 */
    layer_refresh(lyrctl);

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
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
                layer_refresh(lyrctl);
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
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    /* マウスカーソルの移動 */
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
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); /* 座標消す */
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); /* 座標書く */
                    layer_slide(lyrctl, lyr_mouse, mx, my);
                }
            }
        }
    }
}
