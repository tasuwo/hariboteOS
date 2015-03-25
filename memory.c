/**
 * メモリ関連
 * memory.c
 */

#include "bootpack.h"

unsigned int memtest(unsigned int start, unsigned int end){
    char flg486 = 0;            // 486以降のCPUであるかどうかのフラグ
    unsigned int eflg, cr0, i;

    /**
     * 386か，486以降かの確認
     * ACフラグ(18bit)が常に0となるかチェックする
     */
    eflg = io_load_eflags();    // EFLAGレジスタ取得
    eflg |= EFLAGS_AC_BIT;      // ACフラグに1をセット
    io_store_eflags(eflg);     // EFLAGレジスタにセット
    eflg = io_load_eflags();    // EFLAGレジスタ取得
    if ((eflg & EFLAGS_AC_BIT) != 0){
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT;     // ACフラグを0に戻す
    io_store_eflags(eflg);

    /**
     * キャッシュ機能を禁止にする
     */
    if (flg486 != 0){
        cr0 = load_cr0();        // CR0レジスタ取得
        cr0 |= CR0_CACHE_DIABLE; // キャッシュ禁止のためのフラグセット
        store_cr0(cr0);          // CR0レジスタセット
    }

    /**
     * メモリチェック作業本体
     */
    i = memtest_sub(start, end);

    /**
     * キャッシュ機能を許可する
     */
    if (flg486 != 0){
        cr0 = load_cr0();        // CR0レジスタ取得
        cr0 &= ~CR0_CACHE_DIABLE;// キャッシュ許可のためのフラグセット
        store_cr0(cr0);          // CR0レジスタセット
    }

    return i;
}

/**
 * start番地からend番地までの範囲で，使えるメモリがどれだけあるか調べる
 * メモリに適当な値を書いた直後に，書かれた値を読み込む
 * 読み込んだ値と書き込んだ値が等しければメモリが繋がっているが，等しくなかった場合はでたらめな値になっている
 */
unsigned int memtest_sub(unsigned int start, unsigned int end){
    unsigned int pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;    // パターンマッチ用
    unsigned int i, *p, old;

    // 0x1000(4KB)ずつチェック
    for (i = start; i <= end; i += 0x1000){
        p = (unsigned int *) (i + 0xffc);    // 末尾4byteをチェック
        old = *p;           // 前の値を記録する
        *p = pat0;          // パターン0を書き込む
        *p ^= 0xffffffff;   // 反転する
        if (*p != pat1) {   // パターン1(パターン0の反転)と等しいか？
not_memory:
            *p = old;       // 等しくなければbreak
            break;
        }
        *p ^= 0xffffffff;   // もう一度反転して，等しいか調べる
        if (*p != pat0){
            goto not_memory;// 等しくなければbreak
        }
        *p = old;           // 前の値に戻す
    }
    return i;
}
