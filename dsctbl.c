/**
 * dsctbl.c
 * CDT，IDT等の descriptor table 関係
 */

#include "bootpack.h"

/**
 * GDTの初期化処理
 * セグメントを利用するための設定と，割り込み処理を利用するための設定の初期化を行う
 */
void init_gdtidt(void){
    // 0x00270000〜0x0027ffffをGDTとする
    // どこでもよい
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    // 0x0026f800〜0x0026ffffをIDTとする
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;
    int i;

    /**
     * GDTの初期化
     * 全てののセグメントを
     *     リミット      ：0
     *     ベースアドレス：0
     *     アクセス権属性：0(未使用のセグメント)
     * として初期化する
     */
    for (i = 0; i <= LIMIT_GDT / 8; i++) {
        set_segmdesc(gdt + i, 0, 0, 0);
    }

    /**
     * セグメント番号1への設定．メモリ全体を指す．
     * リミット      ：0xffffffff = 4GB
     * ベースアドレス：0
     * アクセス権属性：0x4092
     */
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);

    /**
     * セグメント番号2への設定．bootpack.hrbのためのもの．
     * リミット      ：0x0007ffff = 0.5MB = 512KB
     * ベースアドレス：0x00280000
     * アクセス権属性：0x409a
     */
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);

    /**
     * GDTの容量とアドレスをGDTRに格納
     */
    load_gdtr(LIMIT_GDT, ADR_GDT);

    /* IDTの初期化 */
    for (i = 0; i <= LIMIT_IDT / 8; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }

    /**
     * IDTの容量とアドレスをIDTRに格納
     */
    load_idtr(LIMIT_IDT, ADR_IDT);

    /**
     * 割込みハンドラをIDTに登録する
     */
    set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);

    return;
}


/**
 * メモリ領域に存在するGDTにセグメントの設定を書き込む
 * @param sd    セグメントの設定を格納する構造体
 * @param limit セグメントの大きさ
 * @param base  セグメントがどの番地から始まるか
 * @param ar    セグメントの管理用属性
 */
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
    if (limit > 0xfffff) {
        ar |= 0x8000; /* G_bit = 1 */
        limit /= 0x1000;
    }
    sd->limit_low    = limit & 0xffff;
    sd->base_low     = base & 0xffff;
    sd->base_mid     = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high    = (base >> 24) & 0xff;
    return;
}


/**
 * メモリ領域に存在するIDTに割り込みの設定を書き込む？
 * @param gd       IDTのアドレス
 * @param offset   オフセット
 * @param selector 割り込み番号
 * @param ar       割り込みの管理用属性
 */
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
    gd->offset_low   = offset & 0xffff;
    gd->selector     = selector;
    gd->dw_count     = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high  = (offset >> 16) & 0xffff;
    return;
}
