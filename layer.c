/**
 * レイヤー関連
 * layer.c
 */

#include "bootpack.h"


/**
 * レイヤ制御用の構造体初期化
 * @param   {struct MEMMANAGE} memman    メモリ管理用構造体
 * @param   {unsigned int}     vram      VRAM
 * @param   {unsigned int}     xsize     レイヤのX方向のサイズ
 * @param   {unsigned int}     ysize     レイヤのY方向のサイズ
 * @returns {struct LYRCTL}    ctl       レイヤ制御用構造体
 */
struct LYRCTL *lyrctl_init(struct MEMMANAGE *memman, unsigned char *vram, int xsize,int ysize){
    struct LYRCTL *ctl;
    int i;

    // レイヤ制御用構造体のためのメモリを確保
    ctl = (struct LYRCTL *) memman_alloc_4k(memman, sizeof(struct LYRCTL));
    if (ctl == 0){
        goto err;
    }

    // 構造体の初期化
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;
    for (i=0; i<MAX_LAYERS; i++){
        ctl->layers0[i].flags = LAYER_UNUSED;
    }
 err:
    return ctl;
}


/**
 * レイヤー情報のセット
 * @param {struct LAYER}  lyr     レイヤ情報保持のための構造体
 * @param {unsigned char} buf     レイヤのバッファ
 * @param {int}           xsize   レイヤのX方向サイズ
 * @param {int}           ysize   レイヤのY方向サイズ
 * @param {int}           col_inv レイヤの透明色
 */
void layer_setbuf(struct LAYER *lyr, unsigned char *buf, int xsize, int ysize, int col_inv){
    lyr->buf = buf;
    lyr->bxsize = xsize;
    lyr->bysize = ysize;
    lyr->col_inv = col_inv;
    return;
}


/**
 * 未使用のレイヤ取得
 * @param   {struct LYRCTL} ctl    レイヤ制御用構造体
 * @returns {struct LAYER}  lyr    未使用のレイヤ．失敗の場合は0を返す
 */
struct LAYER *layer_alloc(struct LYRCTL *ctl){
    struct LAYER *lyr;
    int i;

    // 全てのレイヤを調べる
    for(i=0; i<MAX_LAYERS; i++){
        if(ctl->layers0[i].flags == LAYER_UNUSED){
            // 未使用なレイヤを発見した場合
            lyr = &ctl->layers0[i];
            lyr->flags = LAYER_USE;    // 使用中
            lyr->height = -1;          // 非表示
            return lyr;
        }
    }
    return 0;
}


/**
 * レイヤの優先度を変更する
 * @param {struct LYRCTL} ctl
 * @param {struct LAYER} lyr
 * @param {int height} height
 */
void layer_updown(struct LYRCTL *ctl, struct LAYER *lyr, int height){
    int h;
    int old_height = lyr->height;    // 設定前の優先度

    // 優先度の修正
    if (height > ctl->top + 1){
        height = ctl->top + 1;
    }
    if (height < -1){
        height = -1;
    }
    lyr->height = height;

    // layers[]の並べ替え
    if (old_height > height){
        // レイヤの優先度が以前よりも高くなる
        if (height >= 0){
            // 表示の場合
            // 移動レイヤより優先度が高かった全てのレイヤの優先度を1つ下げる
            for(h=old_height; h>height; h--){
                ctl->layers[h] = ctl->layers[h-1];
                ctl->layers[h]->height = h;
            }
            // 移動先である優先度に移動レイヤの情報を格納する
            ctl->layers[height] = lyr;
        } else {
            // 非表示の場合
            if(ctl->top > old_height){
                // 移動レイヤよりも優先度が高かった全てのレイヤの優先度を1つ下げる
                for (h=old_height; h>height; h--){
                    ctl->layers[h] = ctl->layers[h-1];
                    ctl->layers[h]->height = h;
                }
            }
            // 表示中のレイヤ枚数の上限が減る
            ctl->top--;
        }
        layer_refresh(ctl);    // 再描画

    } else if (old_height < height){
        // レイヤの優先度が以前よりも低くなる
        if (old_height >= 0){
            // 表示の場合
            // 移動レイヤより優先度が高かった全てのレイヤの優先度を1つ下げる
            for(h=old_height; h<height; h++){
                ctl->layers[h] = ctl->layers[h+1];
                ctl->layers[h]->height = h;
            }
            // 移動先である優先度に移動レイヤの情報を格納する
            ctl->layers[height] = lyr;
        } else {
            // 非表示から表示の場合
            // 移動レイヤよりも優先度が低かった全てのレイヤの優先度を1上げる
            for (h=ctl->top; h>=height; h--){
                ctl->layers[h+1] = ctl->layers[h];
                ctl->layers[h+1]->height = h+1;
            }
            ctl->layers[height] = lyr;
            // 表示中のレイヤ枚数の上限が増える
            ctl->top++;
        }
        layer_refresh(ctl);    // 再描画
    }
    return;
}


/**
 * レイヤの描画を更新
 * @param {struct LYRCTL} ctl    レイヤ情報制御のための構造体
 */
void layer_refresh(struct LYRCTL *ctl){
    int h;
    int bx, by;                        // 座標描画のためのループ用
    int vx, vy;                        // 画面上の座標
    unsigned char *buf;                // レイヤー情報のバッファ
    unsigned char c;                   // 描画のためのバッファ格納用
    unsigned char *vram = ctl->vram;   // VRAMのアドレス
    struct LAYER *lyr;                 // レイヤー情報を格納する構造体

    // 全てのレイヤーに関してループ
    for(h=0; h<=ctl->top; h++){
        lyr = ctl->layers[h];        // レイヤー情報取得
        buf = lyr->buf;              // レイヤー情報から描画情報取得
        for(by=0; by<lyr->bysize; by++){
            vy = lyr->vy0 + by;      // 描画座標
            for (bx=0; bx<lyr->bxsize; bx++){
                vx = lyr->vx0 + bx;  // 描画座標

                // 描画
                c = buf[by * lyr->bxsize + bx];
                if(c != lyr->col_inv){
                    // 透明色でなければ描画
                    vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
    return;
}


/**
 * レイヤーを上下左右に移動する
 * @param {struct LYRCTL}   ctl    レイヤ制御のための構造体
 * @param {struct LAYER}    lyr    レイヤ情報格納のための構造体
 * @param {int}             vx0    画面上における移動先のX座標
 * @param {int}             vy0    画面上における移動先のY座標
 */
void layer_slide(struct LYRCTL *ctl, struct LAYER *lyr, int vx0, int vy0){
    lyr->vx0 = vx0;
    lyr->vy0 = vy0;
    if (lyr->height >= 0){
        // 表示中の場合
        layer_refresh(ctl);
    }
    return;
}


/**
 * レイヤの解放
 * @param {struct LYRCTL} ctl    レイヤ制御のための構造体
 * @param {struct LAYRE}  lyr    レイヤ情報格納のための構造体
 */
void layer_free(struct LYRCTL *ctl, struct LAYER *lyr){
    if(lyr->height >= 0){
        layer_updown(ctl, lyr, -1);
    }
    lyr->flags = LAYER_UNUSED;
    return;
}
