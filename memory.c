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
