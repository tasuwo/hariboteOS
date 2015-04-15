/************************ asmhead.nas **********************/

/* 起動時情報格納のための構造体 */
struct BOOTINFO {
    char cyls; /* ブートセクタはどこまでディスクを読んだのか */
    char leds; /* ブート時のキーボードのLEDの状態 */
    char vmode; /* ビデオモード  何ビットカラーか */
    char reserve;
    short scrnx, scrny; /* 画面解像度 */
    char *vram;
};
#define ADR_BOOTINFO    0x00000ff0

/*********************** naskfunc.nas ***********************/
void io_hlt(void);
 void io_cli(void);
void io_sti(void);
void io_stihlt(void);
void io_out8(int port, int data);
int io_in8(int port);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);

/************************* fifo.c ****************************/
struct FIFO8 {
    unsigned char *buf;
    int readpt, writept, size, free, flags;
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_queue(struct FIFO8 *fifo, unsigned char data);
int fifo8_dequeue(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo);

/*********************** graphic.c ***************************/
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x_org, int y_org, int x_end, int y_end);
void init_screen8(char *vram, int xsize, int ysize);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *str);
void init_mouse_cursor8(char *mouse, char bc);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
#define COL8_000000     0
#define COL8_FF0000     1
#define COL8_00FF00     2
#define COL8_FFFF00     3
#define COL8_0000FF     4
#define COL8_FF00FF     5
#define COL8_00FFFF     6
#define COL8_FFFFFF     7
#define COL8_C6C6C6     8
#define COL8_840000     9
#define COL8_008400     10
#define COL8_848400     11
#define COL8_000084     12
#define COL8_840084     13
#define COL8_008484     14
#define COL8_848484     15



/***************** dsctbl.c ********************/
/**
 * GDT(global (segment) descriptor table)の中身
 */
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
/**
 * IDT(interrupt descriptor table)の中身
 */
struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT         0x0026f800      // IDTを置くアドレス
#define LIMIT_IDT       0x000007ff      // IDTの有効byte数
#define ADR_GDT         0x00270000      // GDTを置くアドレス
#define LIMIT_GDT       0x0000ffff      // GDTの有効byte数
#define ADR_BOTPAK      0x00280000      // bootpackを置くアドレス
#define LIMIT_BOTPAK    0x0007ffff      // bootpackの有効byte数
#define AR_DATA32_RW    0x4092
#define AR_CODE32_ER    0x409a
#define AR_INTGATE32    0x008e
// void load_gdtr(int limit, int addr);
// void load_idtr(int limit, int addr);


/************************* int.c ****************************/
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
// PIC0：マスタPIC
// PIC1：スレーブPIC
#define PIC0_ICW1       0x0020
#define PIC0_OCW2       0x0020
#define PIC0_IMR        0x0021
#define PIC0_ICW2       0x0021
#define PIC0_ICW3       0x0021
#define PIC0_ICW4       0x0021
#define PIC1_ICW1       0x00a0
#define PIC1_OCW2       0x00a0
#define PIC1_IMR        0x00a1
#define PIC1_ICW2       0x00a1
#define PIC1_ICW3       0x00a1
#define PIC1_ICW4       0x00a1


/************************ keyboard.c **************************/
void wait_KBC_sendready(void);
void init_keyboard(void);
void inthandler21(int *esp);
#define PORT_KEYDAT             0x0060  // モード設定のための装置番号
#define PORT_KEYCMD             0x0064  // モード設定通知のための装置番号
#define PORT_KEYSTA             0x0064  // KBCのコマンド受付可否判断のための装置番号
#define KEYSTA_SEND_NOTREADY    0x02    // 下位2bitが0であるか判断(KBCのコマンド受付可否判定)
#define KEYCMD_WRITE_MODE       0x60    // モード設定のためのコマンド
#define KBC_MODE                0x47    // マウスを利用するモードにするコマンド
extern struct FIFO8 keyfifo;


/*********************** mouse.c ******************************/
struct MOUSEINFO {
    unsigned char buf[3], phase;
    int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct MOUSEINFO *mf);
int mouse_decode(struct MOUSEINFO *mf, unsigned char data);
#define KEYCMD_SENDTO_MOUSE     0xd4    // マウスへの命令送信
#define MOUSECMD_ENABLE         0xf4    // マウスへの有効化命令
extern struct FIFO8 mousefifo;


/*********************** memory.c ******************************/
#define EFLAGS_AC_BIT       0x00040000    // EFLAGSのACフラグ確認用
#define CR0_CACHE_DIABLE    0x60000000    // キャッシュモードのON/OFF用
#define MEMMANAGE_FREES     4090          // 空き情報：約32KB
#define MEMMANAGE_ADDR      0x003c0000
struct FREEINFO {                         // メモリの空き情報
    unsigned int addr, size;
};
struct MEMMANAGE {                        // メモリ管理用
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMANAGE_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMANAGE *man);
unsigned int memman_total(struct MEMMANAGE *man);
unsigned int memman_alloc(struct MEMMANAGE *man, unsigned int size);
int memman_free(struct MEMMANAGE *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMANAGE *man, unsigned int size);
int memman_free_4k(struct MEMMANAGE *man, unsigned int addr, unsigned int size);


/*********************** layer.c ******************************/
#define MAX_LAYERS    256    // 使用するレイヤの上限
// レイヤ情報のフラグ用
#define LAYER_USE        1
#define LAYER_UNUSED     0
// レイヤ情報
struct LAYER {
    unsigned char *buf;    // レイヤー内容を保持するバッファ
    int bxsize, bysize;    // レイヤーサイズ
    int vx0, vy0;          // 画面上のレイヤーの座標
    int col_inv;           // 透明色番号
    int height;            // 優先度
    int flags;             // 使用中のフラグ
    struct LYRCTL *ctl;    // レイヤ制御用の構造体
};
// レイヤ制御
struct LYRCTL {
    unsigned char *vram;               // VRAMのアドレス
    int xsize, ysize;                  // 画面サイズ
    int top;                           // 優先度
    struct LAYER *layers[MAX_LAYERS];  // レイヤ情報(優先度順)
    struct LAYER layers0[MAX_LAYERS];  // レイヤ情報(順不同)
};
struct LYRCTL *lyrctl_init(struct MEMMANAGE *memman, unsigned char *vram, int xsize,int ysize);
void layer_setbuf(struct LAYER *lyr, unsigned char *buf, int xsize, int ysize, int col_inv);
struct LAYER *layer_alloc(struct LYRCTL *ctl);
void layer_updown(struct LAYER *lyr, int height);
void layer_refresh(struct LAYER *lyr, int x_str, int y_str, int x_end, int y_end);
void layer_refreshsub(struct LYRCTL *ctl, int x_str, int y_str, int x_end, int y_end, int h0);
void layer_slide(struct LAYER *lyr, int vx0, int vy0);
void layer_free(struct LAYER *lyr);
