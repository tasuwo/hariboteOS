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


void memman_init(struct MEMMANAGE *man){
    man->frees = 0;    // 空き情報の個数
    man->maxfrees = 0; // freeの最大値
    man->lostsize = 0; // 解放に失敗した合計サイズ
    man->losts = 0;    // 解放に失敗した回数
}

unsigned int memman_total(struct MEMMANAGE *man){
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++){
        t += man->free[i].size;        // メモリの空きサイズの合計を計算
    }
    return t;
}

unsigned int memman_alloc(struct MEMMANAGE *man, unsigned int size){
    unsigned int i, addr;
    for (i = 0; i < man->frees; i++){
        if (man->free[i].size >= size){
            // 空きサイズを発見したら
            addr = man->free[i].addr;    // 空き情報のアドレスを取得
            man->free[i].addr += size;   // 空き情報のアドレスをずらす
            man->free[i].size -= size;   // 空き情報のサイズを減らす
            if (man->free[i].size == 0){
                // 空き情報がなくなったら
                man->frees--;
                for (; i < man->frees; i++){
                    man->free[i] = man->free[i+1];
                }
            }
            return addr;
        }
    }
}

int memman_free(struct MEMMANAGE *man, unsigned int addr, unsigned int size){
    int i,j;

    // 空き情報を挿入する箇所を決定する(addr順)
    for (i = 0; i < man->frees; i++){
        if (man->free[i].addr > addr){
            break;
        }
    }

    //
    if (i > 0){
        if (man->free[i-1].addr + man->free[i-1].size == addr){
            // 前の空き領域とまとめられる
            man->free[i-1].size += size;
            if (i < man->frees){
                // 後ろもある
                if (addr + size == man->free[i].addr){
                    // 後ろともまとめられる
                    man->free[i-1].size += man->free[i].size;
                    // 前へつめる
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i+1];
                    }
                }
            }
            return 0;
        }
    }

    // 前とまとめられなかった
    if (i < man->frees) {
        // 後ろがある
        if (addr + size == man->free[i].addr) {
            // 後ろとはまとめる
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;
        }
    }

    // 前にも後ろにもまとめられない
    if (man->frees < MEMMANAGE_FREES) {
        for (j = man->frees; j>i; j--){
            man->free[j] = man->free[j-1];
        }
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees;
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }

    // 後ろにずらせなかった
    man->losts++;
    man->lostsize += size;
    return -1;
}
