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
    struct MOUSEINFO minfo;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, key;
    unsigned int memtotal;
    struct MEMMANAGE *memman = (struct MEMMANAGE *) MEMMANAGE_ADDR;

    /* 初期化処理 */
    init_gdtidt();                                          // GDT，IDT初期化
    init_pic();                                             // PIC初期化
    io_sti();                                               // 割り込み禁止を解除
    fifo8_init(&keyfifo, 32, keybuf);                       // キーボード用バッファ初期化
    fifo8_init(&mousefifo, 32, mousebuf);                   // マウス用バッファ初期化
    io_out8(PIC0_IMR, 0xf9);                                // 割り込み許可：キーボードとPIC1(11111001)
    io_out8(PIC1_IMR, 0xef);                                // 割り込み許可：マウス(11101111)

    init_keyboard();                                        // KBCの初期化(マウス使用モードに設定)
    enable_mouse(&minfo);                                   // マウス有効化

    init_palette();                                         // パレット初期化
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);  // OS初期画面描画
    memman_init(memman);                                    // メモリ管理用の構造体初期化

    /* マウスカーソルの描画 */
    mx = (binfo->scrnx - 16) / 2;                                           // 座標計算(画面中央)
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);                               // mcursorにマウスを初期化
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);    // mcursorを描画
    sprintf(s, "(%d, %d)", mx, my);                                         // 座標をメモリに書き込む
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);         // 座標の描画

    /**
     * メモリの解放を行う
     * 0x00001000 ～ 0x0009e000 は？？？
     * 0x00000000 ～ 0x00400000 は，起動中やフロッピーディスクの内容記録，IDTやGDT等に使用されている
     * そのため，0x00400000 以降を解放する
     */
    memtotal = memtest(0x00400000, 0xbfffffff);             // 使用可能なメモリをチェックする
    memman_free(memman, 0x00001000, 0x0009e000);            // ？？？
    memman_free(memman, 0x00400000, memtotal - 0x00400000); // 0x00400000以降のメモリを解放

    /* 文字列描画  */
    sprintf(s, "memory %dMB  free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

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
