/**
 * bootpack.c
 * main
 */

#include "bootpack.h"
#include <stdio.h>

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
    struct LYRCTL *lyrctl;                                            // レイヤ制御用の構造体
    struct LAYER *lyr_back, *lyr_mouse, *lyr_win;                     // 各レイヤ
    unsigned char *buf_back, buf_mouse[256], *buf_win;                // 各レイヤのバッファ
    unsigned int count = 0;

    /* 初期化処理 */
    init_gdtidt();                                          // GDT，IDT初期化
    init_pic();                                             // PIC初期化
    io_sti();                                               // 割り込み禁止を解除
    fifo8_init(&keyfifo, 32, keybuf);                       // キーボード用バッファ初期化
    fifo8_init(&mousefifo, 128, mousebuf);                  // マウス用バッファ初期化
    init_pit();                                             // PIT(タイマ割り込み)の周期初期化
    io_out8(PIC0_IMR, 0xf8);                                // 割り込み許可：キーボード，PIC1(11111000)，PIT
    io_out8(PIC1_IMR, 0xef);                                // 割り込み許可：マウス(11101111)

    init_keyboard();                                        // KBCの初期化(マウス使用モードに設定)
    enable_mouse(&minfo);                                   // マウス有効化

    memtotal = memtest(0x00400000, 0xbfffffff);             // 使用可能なメモリをチェックする
    memman_init(memman);                                    // メモリ管理用の構造体初期化
    memman_free(memman, 0x00001000, 0x0009e000);            // ？？？
    memman_free(memman, 0x00400000, memtotal - 0x00400000); // 0x00400000以降のメモリを解放

    /* 描画のための初期化処理 */
    init_palette();                                                         // パレット初期化
    lyrctl = lyrctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  // レイヤ制御用の構造体初期化

    /* メモリ確保 */
    // 各レイヤ
    lyr_back  = layer_alloc(lyrctl);
    lyr_mouse = layer_alloc(lyrctl);
    lyr_win   = layer_alloc(lyrctl);
    // 各レイヤのバッファ(マウスは256固定)
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    buf_win   = (unsigned char *) memman_alloc_4k(memman,160 * 52);

    /* レイヤ設定 */
    // レイヤ制御構造体とのバインディング
    layer_setbuf(lyr_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);
    layer_setbuf(lyr_mouse, buf_mouse, 16,  16, 99);
    layer_setbuf(lyr_win,   buf_win,   160, 52, -1);
    // バッファを初期化
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);  // OS初期画面
    init_mouse_cursor8(buf_mouse, 99);                   // マウス
    make_window8(buf_win, 160, 52, "counter");            // ウインドウ
    // 優先度を設定
    layer_updown(lyr_back,  0);
    layer_updown(lyr_mouse, 2);
    layer_updown(lyr_win,   1);

    /******************************** 描画 ***********************************/
    /*** ウインドウレイヤ ***/
    /** 現状，文字列とウインドウは特に紐付いていない **/
    //putfonts8_asc(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
    //putfonts8_asc(buf_win, 160, 24, 44, COL8_000000, "  Haribote-OS!");
    layer_slide(lyr_win, 80, 72);

    /*** マウスレイヤ ***/
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    layer_slide(lyr_mouse, mx, my);                                    // マウス描画位置

    /*** 背景レイヤ ***/
    layer_slide(lyr_back, 0, 0);                                       // 背景描画位置
    // マウスカーソルの座標
    sprintf(s, "(%3d, %3d)", mx, my);                                  // メモリに書き込み
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);       // バッファに格納
    // メモリ容量，メモリ空き状態
    sprintf(s, "memory %dMB  free : %dKB",
            memtotal / (1024 * 1024), memman_total(memman) / 1024);    // メモリに書き込み
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);      // バッファに格納

    /*** 各レイヤの描画 ***/
    layer_refresh(lyr_back, 0, 0, binfo->scrnx, 48);
    /*************************************************************************/

    /* CPU休止 */
    while(1){
        /* カウンタ */
        count++;
        sprintf(s, "%010d", timerctl.count);
        boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
        putfonts8_asc(buf_win, 160, 40, 28, COL8_000000, s);
        layer_refresh(lyr_win, 40, 28, 120, 44);

        /* 割り込み禁止にする */
        io_cli();

        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
            // 割り込みがなければ
            /* 割り込みフラグをセットし，hltする */
            //io_stihlt();
            io_sti();
        } else {
            /* キーボード，マウスの順に調べる */
            if (fifo8_status(&keyfifo) != 0) {
                /* キーを読み込む */
                key = fifo8_dequeue(&keyfifo);
                /* 割り込みフラグをセットする */
                io_sti();

                /* 押下されたキーの描画 */
                sprintf(s, "%02X", key);                                         // メモリに文字列を書き出す
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);    // バッファのクリア(透明色で塗りつぶし)
                putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);    // 文字をバッファに格納
                layer_refresh(lyr_back, 0, 16, 16, 32);                          // 文字列の表示範囲の描画を更新

            } else if (fifo8_status(&mousefifo) != 0) {
                /* データの取得 */
                key = fifo8_dequeue(&mousefifo);
                /* 割り込みフラグをセットする */
                io_sti();

                // マウスの座標描画
                if (mouse_decode(&minfo, key) != 0){
                    /* データが3バイト揃ったので表示 */

                    /* 描画：マウスからの入力 */
                    sprintf(s, "[lcr %4d %4d]", minfo.x, minfo.y);                                 // メモリに文字列を書き出す
                    if ((minfo.btn & 0x01) != 0) { s[1] = 'L'; }
                    if ((minfo.btn & 0x02) != 0) { s[3] = 'R'; }
                    if ((minfo.btn & 0x04) != 0) { s[2] = 'C'; }
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);    // バッファのクリア(透明色で塗りつぶし)
                    putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);                 // 文字をバッファに格納
                    layer_refresh(lyr_back, 32, 16, 32 + 15 * 8, 32);                              // 文字列の表示範囲の描画を更新

                    /* マウスカーソルの移動した座標 */
                    mx += minfo.x;
                    my += minfo.y;
                    if (mx < 0) { mx = 0; }
                    if (my < 0) { my = 0; }
                    if (mx > binfo->scrnx - 1) { mx = binfo->scrnx - 1; }
                    if (my > binfo->scrny - 1) { my = binfo->scrny - 1; }

                    /* 描画：マウスカーソルの座標 */
                    sprintf(s, "(%3d, %3d)", mx, my);                            // メモリに文字列を書き出す
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // 描画範囲のクリア(透明色で塗りつぶし)
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); // 文字をバッファに格納
                    layer_refresh(lyr_back, 0, 0, 80, 16);                       // 描画範囲の表示更新

                    /* マウスカーソルを動かす */
                    layer_slide(lyr_mouse, mx, my);
                }
            }
        }
    }
}
