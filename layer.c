/**
 * レイヤー関連
 * layer.c
 */

#include "bootpack.h"


/**
 * レイヤ制御用構造体初期化
 * @param   {struct MEMMANAGE} memman    メモリ管理用構造体
 * @param   {unsigned int}     vram      VRAM
 * @param   {unsigned int}     xsize     レイヤのX方向のサイズ
 * @param   {unsigned int}     ysize     レイヤのY方向のサイズ
 * @returns {struct LYRCTL}    ctl       レイヤ制御用構造体
 */
struct LYRCTL *lyrctl_init(struct MEMMANAGE *memman, unsigned char *vram, int xsize,int ysize){
    struct LYRCTL *ctl;
    int i;

    // メモリ確保
    ctl = (struct LYRCTL *)memman_alloc_4k(memman, sizeof(struct LYRCTL));
    if (ctl == 0){ goto err; }

    // 構造体の初期化
    ctl->vram  = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top   = -1;
    for (i=0; i<MAX_LAYERS; i++){ ctl->layers0[i].flags = LAYER_UNUSED; }

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
    lyr->buf     = buf;
    lyr->bxsize  = xsize;
    lyr->bysize  = ysize;
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
    for (i = 0; i < MAX_LAYERS; i++) {
        if(ctl->layers0[i].flags == LAYER_UNUSED) {
            // 未使用なレイヤを発見した場合
            lyr         = &ctl->layers0[i];
            lyr->flags  = LAYER_USE;    // 使用中
            lyr->height = -1;           // 非表示
            return lyr;
        }
    }
    return 0;
}


/**
 * レイヤの優先度を変更する
 * @param {struct LYRCTL} ctl       優先度を変更するレイヤを制御する構造体
 * @param {struct LAYER}  lyr       優先度を変更するレイヤ
 * @param {int height}    height    設定する優先度
 */
void layer_updown(struct LYRCTL *ctl, struct LAYER *lyr, int height){
    int h;                           // ループ用変数
    int old_height = lyr->height;    // 設定前の優先度

    // 優先度の修正
    if (height > ctl->top + 1) { height = ctl->top + 1; }    // 最上層に修正
    if (height < -1)           { height = -1; }              // 最下層に修正
    if (height == old_height)  { return; }                   // 優先度に変更がない場合，終了
    lyr->height = height;

    // layers[]の並べ替え
    if (old_height > height) {
        // レイヤの優先度が以前よりも低くなる
        if (height == -1) {
            // 非表示の場合
            if(ctl->top > old_height){
                // 移動レイヤより上位のレイヤの優先度を下げる
                for (h = old_height; h > height; h--){
                    ctl->layers[h] = ctl->layers[h-1];
                    ctl->layers[h]->height = h;
                }
            }
            // レイヤの枚数を減らす
            ctl->top--;
        } else {
            // 移動レイヤより上位のレイヤの優先度を下げる
            for (h = old_height; h > height; h--) {
                ctl->layers[h] = ctl->layers[h-1];
                ctl->layers[h]->height = h;
            }
            // レイヤを移動する
            ctl->layers[height] = lyr;
        }
    } else if (old_height < height) {
        // レイヤの優先度が以前よりも高くなる
        if (old_height == -1) {
            // 非表示のレイヤを表示する
            // 移動レイヤより上位のレイヤの優先度をあげる
            for (h = ctl->top; h >= height; h--) {
                ctl->layers[h+1] = ctl->layers[h];
                ctl->layers[h+1]->height = h+1;
            }
            // 表示中のレイヤ枚数の上限が増える
            ctl->top++;
        } else {
            // 移動レイヤより下位のレイヤの優先度を下げる
            for(h = old_height; h > height; h++) {
                ctl->layers[h] = ctl->layers[h+1];
                ctl->layers[h]->height = h;
            }
        }
        // レイヤを移動する
        ctl->layers[height] = lyr;
    }

    // 再描画
    layer_refreshsub(ctl, lyr->vx0, lyr->vy0, lyr->vx0 + lyr->bxsize, lyr->vy0 + lyr->bysize);

    return;
}


/**
 * 指定範囲内を再描画(レイヤ上の座標)
 * @param {struct LYRCTL} ctl    レイヤ情報制御のための構造体
 * @param {struct LAYER}  lyr    範囲指定を行うレイヤ
 * @param {int}           x_str  描画範囲(X座標始点)
 * @param {int}           y_str  描画範囲(Y座標始点)
 * @param {int}           x_end  描画範囲(X座標終点)
 * @param {int}           y_end  描画範囲(Y座標終点)
 */
void layer_refresh(struct LYRCTL *ctl, struct LAYER *lyr, int x_str, int y_str, int x_end, int y_end){
    if (lyr->height != -1){
        layer_refreshsub(ctl, lyr->vx0 + x_str, lyr->vy0 + y_str, lyr->vx0 + x_end, lyr->vy0 + y_end);
    }
}


/**
 * 指定範囲内を再描画(画面上の座標)
 * @param {struct LYRCTL} ctl    レイヤ情報制御のための構造体
 * @param {int}           x_str  描画範囲(X座標始点)
 * @param {int}           y_str  描画範囲(Y座標始点)
 * @param {int}           x_end  描画範囲(X座標終点)
 * @param {int}           y_end  描画範囲(Y座標終点)
 */
void layer_refreshsub(struct LYRCTL *ctl, int x_str, int y_str, int x_end, int y_end){
    int h;
    int bx, by;                        // 座標描画のためのループ用
    int vx, vy;                        // 画面上の座標
    unsigned char *buf;                // レイヤー情報のバッファ
    unsigned char c;                   // 描画のためのバッファ格納用
    unsigned char *vram = ctl->vram;   // VRAMのアドレス
    struct LAYER *lyr;                 // レイヤー情報を格納する構造体

    // 全てのレイヤーに関してループ
    for (h = 0; h <= ctl->top; h++) {
        lyr = ctl->layers[h];        // レイヤー情報取得
        buf = lyr->buf;              // レイヤー情報から描画情報取得
        // レイヤーの描画のため，X，Y座標についてループ
        for (by = 0; by < lyr->bysize; by++) {
            vy = lyr->vy0 + by;      // 描画座標
            for (bx = 0; bx < lyr->bxsize; bx++) {
                vx = lyr->vx0 + bx;  // 描画座標

                /************************* 描画 ************************/
                // 描画範囲内であるか？
                if (x_str <= vx && vx < x_end && y_str <= vy && vy < y_end){
                    // 描画用バッファ
                    c = buf[by * lyr->bxsize + bx];
                    // 透明色でなければ描画
                    if(c != lyr->col_inv){ vram[vy * ctl->xsize + vx] = c; }
                }
                /*******************************************************/
            }
        }
    }
    return;
}


/**
 * レイヤーを上下左右に移動する
 * @param {struct LYRCTL}   ctl    移動レイヤの制御情報を格納した構造体
 * @param {struct LAYER}    lyr    移動するレイヤ
 * @param {int}             vx0    画面上における移動先のX座標
 * @param {int}             vy0    画面上における移動先のY座標
 */
void layer_slide(struct LYRCTL *ctl, struct LAYER *lyr, int vx0, int vy0){
    // 移動前の座標を保持
    int old_vx0 = lyr->vx0;
    int old_vy0 = lyr->vy0;
    // 移動後の座標を格納
    lyr->vx0 = vx0;
    lyr->vy0 = vy0;
    // 非表示状態でなければ，描画する
    if (lyr->height != -1){
        // 移動前の範囲を再描画
        layer_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + lyr->bxsize, old_vy0 + lyr->bysize);
        // 移動後の範囲を再描画
        layer_refreshsub(ctl, vx0, vy0, vx0 + lyr->bxsize, vy0 + lyr->bysize);
    }
    return;
}


/**
 * レイヤの解放
 * @param {struct LYRCTL} ctl    非表示レイヤの制御情報を格納した構造体
 * @param {struct LAYRE}  lyr    非表示にするレイヤ
 */
void layer_free(struct LYRCTL *ctl, struct LAYER *lyr){
    // 非表示状態でなければ，非表示状態にする
    if(lyr->height != -1){ layer_updown(ctl, lyr, -1); }
    // レイヤを未使用状態にする
    lyr->flags = LAYER_UNUSED;
    return;
}
