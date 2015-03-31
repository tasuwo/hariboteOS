/**
 * graphic.c
 * グラフィック関係
 */

#include "bootpack.h"

/**
 * カラーパレットを初期化する．
 * 15色指定できる
 */
void init_palette(void){
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,   /* 0:黒 */
        0xff, 0x00, 0x00,   /* 1:明るい赤 */
        0x00, 0xff, 0x00,   /*  2:明るい緑 */
        0xff, 0xff, 0x00,   /*  3:明るい黄色 */
        0x00, 0x00, 0xff,   /*  4:明るい青 */
        0xff, 0x00, 0xff,   /*  5:明るい紫 */
        0x00, 0xff, 0xff,   /*  6:明るい水色 */
        0xff, 0xff, 0xff,   /*  7:白 */
        0xc6, 0xc6, 0xc6,   /*  8:明るい灰色 */
        0x84, 0x00, 0x00,   /*  9:暗い赤 */
        0x00, 0x84, 0x00,   /* 10:暗い緑 */
        0x84, 0x84, 0x00,   /* 11:暗い黄色 */
        0x00, 0x00, 0x84,   /* 12:暗い青 */
        0x84, 0x00, 0x84,   /* 13:暗い紫 */
        0x00, 0x84, 0x84,   /* 14:暗い水色 */
        0x84, 0x84, 0x84    /* 15:暗い灰色 */
    };
    set_palette(0, 15, table_rgb);
    return;
}


void set_palette(int start, int end, unsigned char *rgb){
    int i, eflags;
    eflags = io_load_eflags();          // 割り込みフラグの記録
    io_cli();                           // 割り込み禁止にする
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);            // 割り込みフラグを戻す
    return;
}


/**
 * 四角形を描画する
 * @param vram  vramの先頭ポインタ
 * @param xsize 画面のx軸幅
 * @param c     色指定
 * @param x_org x座標原点
 * @param y_org y座標原点
 * @param x_end x座標終点
 * @param y_end y座標終点
 */
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x_org, int y_org, int x_end, int y_end){
    int x_drw,y_drw;
    for (y_drw = y_org; y_drw <= y_end; y_drw++){
        for (x_drw = x_org; x_drw <= x_end; x_drw++){
            vram[y_drw * xsize + x_drw] = c;
        }
    }
    return;
}


/**
 * 描画用バッファの初期化
 * @param vram vramの戦闘ポインタ
 * @param x    x幅
 * @param y    y幅
 */
void init_screen8(char *vram, int xsize, int ysize){
    boxfill8(vram, xsize, COL8_008484,  0,         0,          xsize -  1, ysize - 29);
    boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 28, xsize -  1, ysize - 28);
    boxfill8(vram, xsize, COL8_FFFFFF,  0,         ysize - 27, xsize -  1, ysize - 27);
    boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 26, xsize -  1, ysize -  1);

    boxfill8(vram, xsize, COL8_FFFFFF,  3,         ysize - 24, 59,         ysize - 24);
    boxfill8(vram, xsize, COL8_FFFFFF,  2,         ysize - 24,  2,         ysize -  4);
    boxfill8(vram, xsize, COL8_848484,  3,         ysize -  4, 59,         ysize -  4);
    boxfill8(vram, xsize, COL8_848484, 59,         ysize - 23, 59,         ysize -  5);
    boxfill8(vram, xsize, COL8_000000,  2,         ysize -  3, 59,         ysize -  3);
    boxfill8(vram, xsize, COL8_000000, 60,         ysize - 24, 60,         ysize -  3);

    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24);
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3);
    return;
}


/**
 * 1文字出力する
 * @param vram  vramの先頭ポインタ
 * @param xsize スクリーンのx幅
 * @param x     原点のx座標
 * @param y     原点のy座標
 * @param c     カラー
 * @param font  フォント情報
 */
void putfont8(char *vram, int xsize, int x, int y, char c, char *font){
    /* 文字の描画範囲 */
    int cols = 16;
    int rows = 8;                   // fontがcharであるため，これは8bit幅の文字出力専用

    int col,row;                    // カウンタ
    int org = x + y*xsize;          // 原点の座標
    char *p = (char *) &vram[org];  // 原点のポインタ

    for (col=0; col < cols; col++){
        for (row=0; row < rows; row++){
            if (font[col] & (0x80 >> row)){
                *(p + row + col*xsize) = c;
            }
        }
    }
}


/**
 * 文字列を出力する
 * @param vram  vramの戦闘ポインタ
 * @param xsize スクリーンのx幅
 * @param x     原点のx座標
 * @param y     原点のy座標
 * @param c     カラー
 * @param str   文字列
 */
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *str){
    // 外部からフォント情報を取得する
    extern char hankaku[4096];

    // C言語では，文字列は0x00で終わる
    for (; *str != 0x00; str++){
        putfont8(vram, xsize, x, y, c, hankaku + *str * 16);  // 1文字のデータ量は 8*16 = 16byte
        x += 8;                                             // 1文字の幅が8bitであるため，その分ずらす
    }
    return;
}


/**
 * マウスカーソル描画用のバッファ初期化
 * @param mouse マウスの先頭ポインタ
 * @param bc    背景カラー
 */
void init_mouse_cursor8(char *mouse, char bc){
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"
    };
    int x,y,dot;

    for (x = 0; x<16; x++){
        for(y = 0; y<16; y++){
            dot = x + y*16;
            switch (cursor[x][y]) {
                case '*':
                    mouse[dot] = COL8_000000;
                    break;
                case 'O':
                    mouse[dot] = COL8_FFFFFF;
                    break;
                case '.':
                    mouse[dot] = bc;
                    break;
            }
        }
    }
    return;
}


/**
 * 描画する
 * @param vram   vramの先頭ポインタ
 * @param vxsize スクリーンのx幅
 * @param pxsize 描画対象のx幅
 * @param pysize 描画対象のy幅
 * @param px0    描画対象のx原点
 * @param py0    描画対象のy原点
 * @param buf    描画対象を先頭ポインタ
 * @param bxsize 描画対象のx幅(描画する1ラインの画素数)
 */
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize){
    int x,y;
    int color;
    int org = px0 + py0*vxsize;

    for(x=0; x < pxsize; x++){
        for(y=0; y < pysize; y++){
            color = buf[x + y * bxsize];
            vram[org + x + y * vxsize] = color;
        }
    }
}
