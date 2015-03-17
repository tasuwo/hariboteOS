/**
 * fifo.c
 * FIFOバッファ関連
 */

#include "bootpack.h"
#define FLAGS_OVERRUN 0x0001

/**
 * FIFOバッファの初期化関数
 * @param fifo バッファ情報(構造体)のアドレス
 * @param size バッファのサイズ
 * @param buf  バッファのアドレス
 */
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf){
    fifo->size      = size; // バッファのサイズ
    fifo->buf       = buf;  // バッファのアドレス
    fifo->free      = size; // バッファの空き
    fifo->flags     = 0;    // あふれのフラグ
    fifo->readpt    = 0;    // 読み込みポインタ
    fifo->writept   = 0;    // 書き込みポインタ
}

/**
 * FIFOバッファにキューする
 * @param  fifo バッファ情報(構造体)のアドレス
 * @param  data キューするデータ
 * @return      0：成功，-1：あふれ
 */
int fifo8_queue(struct FIFO8 *fifo, unsigned char data){
    if (fifo->free == 0){
        /* あふれの判定 */
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    if (fifo->writept > fifo->size){
        /* writeptの初期化 */
        fifo->writept = 0;
    }
    /* データを書き込む */
    fifo->buf[fifo->writept++] = data;
    /* 空き領域を減らす */
    fifo->free--;

    return 0;
}

/**
 * FIFOバッファからデキューする
 * @param  fifo バッファ情報(構造体)のアドレス
 * @return      デキューしたデータ
 */
int fifo8_dequeue(struct FIFO8 *fifo){
    int data;
    if (fifo->size == fifo->free){
        /* バッファが空 */
        return -1;
    }
    if (fifo->readpt > fifo->size){
        /* readptの初期化 */
        fifo->readpt = 0;
    }
    /* データを読み込む */
    data = fifo->buf[fifo->readpt++];
    /* 空き領域を増やす */
    fifo->free++;

    return data;
}

/**
 * FIFOバッファに溜まっているデータ量を返す
 * @param  fifo バッファ情報(構造体)のアドレス
 * @return      バッファに溜まっているデータ量
 */
int fifo8_status(struct FIFO8 *fifo){
    return fifo->size - fifo->free;
}