/**
 * bootpack.c
 * main
 */

#include "bootpack.h"
#include <stdio.h>

/* main */
void HariMain(void)
{
    /* asmhead.nas�Ń��������l���擾 */
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct FIFO8 timerfifo, timerfifo2, timerfifo3;
    char s[40], keybuf[32], mousebuf[128], timerbuf[8], timerbuf2[8], timerbuf3[8];
    struct TIMER *timer, *timer2, *timer3;
    int mx, my, key, i;
    unsigned int memtotal;
    struct MOUSEINFO minfo;
    struct MEMMANAGE *memman = (struct MEMMANAGE *) MEMMANAGE_ADDR;
    struct LYRCTL *lyrctl;                                            // ���C������p�̍\����
    struct LAYER *lyr_back, *lyr_mouse, *lyr_win;                     // �e���C��
    unsigned char *buf_back, buf_mouse[256], *buf_win;                // �e���C���̃o�b�t�@
    unsigned int count = 0;

    /* ���������� */
    init_gdtidt();                                          // GDT�CIDT������
    init_pic();                                             // PIC������
    io_sti();                                               // ���荞�݋֎~������
    fifo8_init(&keyfifo, 32, keybuf);                       // �L�[�{�[�h�p�o�b�t�@������
    fifo8_init(&mousefifo, 128, mousebuf);                  // �}�E�X�p�o�b�t�@������
    fifo8_init(&timerfifo, 8, timerbuf);                    // �^�C�}�p�o�b�t�@������
    fifo8_init(&timerfifo2, 8, timerbuf2);
    fifo8_init(&timerfifo3, 8, timerbuf3);
    timer = timer_alloc();
    timer2 = timer_alloc();
    timer3 = timer_alloc();
    timer_init(timer, &timerfifo, 1);
    timer_init(timer2, &timerfifo2, 1);
    timer_init(timer3, &timerfifo3, 1);
    init_pit();                                             // PIT(�^�C�}���荞��)�̎���������
    io_out8(PIC0_IMR, 0xf8);                                // ���荞�݋��F�L�[�{�[�h�CPIC1(11111000)�CPIT
    io_out8(PIC1_IMR, 0xef);                                // ���荞�݋��F�}�E�X(11101111)
    timer_settime(timer, 1000);                          // �^�C���A�E�g�����ݒ�
    timer_settime(timer2, 300);
    timer_settime(timer3, 50);

    init_keyboard();                                        // KBC�̏�����(�}�E�X�g�p���[�h�ɐݒ�)
    enable_mouse(&minfo);                                   // �}�E�X�L����

    memtotal = memtest(0x00400000, 0xbfffffff);             // �g�p�\�ȃ��������`�F�b�N����
    memman_init(memman);                                    // �������Ǘ��p�̍\���̏�����
    memman_free(memman, 0x00001000, 0x0009e000);            // �H�H�H
    memman_free(memman, 0x00400000, memtotal - 0x00400000); // 0x00400000�ȍ~�̃����������

    /* �`��̂��߂̏��������� */
    init_palette();                                                         // �p���b�g������
    lyrctl = lyrctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  // ���C������p�̍\���̏�����

    /* �������m�� */
    // �e���C��
    lyr_back  = layer_alloc(lyrctl);
    lyr_mouse = layer_alloc(lyrctl);
    lyr_win   = layer_alloc(lyrctl);
    // �e���C���̃o�b�t�@(�}�E�X��256�Œ�)
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    buf_win   = (unsigned char *) memman_alloc_4k(memman,160 * 52);

    /* ���C���ݒ� */
    // ���C������\���̂Ƃ̃o�C���f�B���O
    layer_setbuf(lyr_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);
    layer_setbuf(lyr_mouse, buf_mouse, 16,  16, 99);
    layer_setbuf(lyr_win,   buf_win,   160, 52, -1);
    // �o�b�t�@��������
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);  // OS�������
    init_mouse_cursor8(buf_mouse, 99);                   // �}�E�X
    make_window8(buf_win, 160, 52, "counter");            // �E�C���h�E
    // �D��x��ݒ�
    layer_updown(lyr_back,  0);
    layer_updown(lyr_mouse, 2);
    layer_updown(lyr_win,   1);

    /******************************** �`�� ***********************************/
    /*** �E�C���h�E���C�� ***/
    /** ����C������ƃE�C���h�E�͓��ɕR�t���Ă��Ȃ� **/
    //putfonts8_asc(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
    //putfonts8_asc(buf_win, 160, 24, 44, COL8_000000, "  Haribote-OS!");
    layer_slide(lyr_win, 80, 72);

    /*** �}�E�X���C�� ***/
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    layer_slide(lyr_mouse, mx, my);                                    // �}�E�X�`��ʒu

    /*** �w�i���C�� ***/
    layer_slide(lyr_back, 0, 0);                                       // �w�i�`��ʒu
    // �}�E�X�J�[�\���̍��W
    sprintf(s, "(%3d, %3d)", mx, my);                                  // �������ɏ�������
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);       // �o�b�t�@�Ɋi�[
    // �������e�ʁC�������󂫏��
    sprintf(s, "memory %dMB  free : %dKB",
            memtotal / (1024 * 1024), memman_total(memman) / 1024);    // �������ɏ�������
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);      // �o�b�t�@�Ɋi�[

    /*** �e���C���̕`�� ***/
    layer_refresh(lyr_back, 0, 0, binfo->scrnx, 48);
    /*************************************************************************/

    /* CPU�x�~ */
    while(1){
        /* �J�E���^ */
        sprintf(s, "%010d", timerctl.count);
        boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
        putfonts8_asc(buf_win, 160, 40, 28, COL8_000000, s);
        layer_refresh(lyr_win, 40, 28, 120, 44);

        /* ���荞�݋֎~�ɂ��� */
        io_cli();

        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timerfifo)
            + fifo8_status(&timerfifo2) + fifo8_status(&timerfifo3) == 0){
            // ���荞�݂��Ȃ����
            /* ���荞�݃t���O���Z�b�g���Chlt���� */
            //io_stihlt();
            io_sti();
        } else {
            /* �L�[�{�[�h�C�}�E�X�̏��ɒ��ׂ� */
            if (fifo8_status(&keyfifo) != 0) {
                /* �L�[��ǂݍ��� */
                key = fifo8_dequeue(&keyfifo);
                /* ���荞�݃t���O���Z�b�g���� */
                io_sti();

                /* �������ꂽ�L�[�̕`�� */
                sprintf(s, "%02X", key);                                         // �������ɕ�����������o��
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);    // �o�b�t�@�̃N���A(�����F�œh��Ԃ�)
                putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);    // �������o�b�t�@�Ɋi�[
                layer_refresh(lyr_back, 0, 16, 16, 32);                          // ������̕\���͈͂̕`����X�V

            } else if (fifo8_status(&mousefifo) != 0) {
                /* �f�[�^�̎擾 */
                key = fifo8_dequeue(&mousefifo);
                /* ���荞�݃t���O���Z�b�g���� */
                io_sti();

                // �}�E�X�̍��W�`��
                if (mouse_decode(&minfo, key) != 0){
                    /* �f�[�^��3�o�C�g�������̂ŕ\�� */

                    /* �`��F�}�E�X����̓��� */
                    sprintf(s, "[lcr %4d %4d]", minfo.x, minfo.y);                                 // �������ɕ�����������o��
                    if ((minfo.btn & 0x01) != 0) { s[1] = 'L'; }
                    if ((minfo.btn & 0x02) != 0) { s[3] = 'R'; }
                    if ((minfo.btn & 0x04) != 0) { s[2] = 'C'; }
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);    // �o�b�t�@�̃N���A(�����F�œh��Ԃ�)
                    putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);                 // �������o�b�t�@�Ɋi�[
                    layer_refresh(lyr_back, 32, 16, 32 + 15 * 8, 32);                              // ������̕\���͈͂̕`����X�V

                    /* �}�E�X�J�[�\���̈ړ��������W */
                    mx += minfo.x;
                    my += minfo.y;
                    if (mx < 0) { mx = 0; }
                    if (my < 0) { my = 0; }
                    if (mx > binfo->scrnx - 1) { mx = binfo->scrnx - 1; }
                    if (my > binfo->scrny - 1) { my = binfo->scrny - 1; }

                    /* �`��F�}�E�X�J�[�\���̍��W */
                    sprintf(s, "(%3d, %3d)", mx, my);                            // �������ɕ�����������o��
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // �`��͈͂̃N���A(�����F�œh��Ԃ�)
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); // �������o�b�t�@�Ɋi�[
                    layer_refresh(lyr_back, 0, 0, 80, 16);                       // �`��͈͂̕\���X�V

                    /* �}�E�X�J�[�\���𓮂��� */
                    layer_slide(lyr_mouse, mx, my);
                }
            } else if (fifo8_status(&timerfifo) != 0){
                i = fifo8_dequeue(&timerfifo);
                io_sti();
                putfonts8_asc(buf_back, binfo->scrnx, 0, 64, COL8_FFFFFF, "10[sec]");
                layer_refresh(lyr_back, 0, 64, 56, 80);
            } else if (fifo8_status(&timerfifo2) != 0){
                i = fifo8_dequeue(&timerfifo2);
                io_sti();
                putfonts8_asc(buf_back, binfo->scrnx, 0, 80, COL8_FFFFFF, "3[sec]");
                layer_refresh(lyr_back, 0, 80, 48, 96);
            } else if (fifo8_status(&timerfifo3) != 0){
                i = fifo8_dequeue(&timerfifo3);
                io_sti();
                if (i != 0){
                    timer_init(timer3, &timerfifo3, 0);
                    boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
                } else {
                    timer_init(timer3, &timerfifo3, 1);
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
                }
                timer_settime(timer3, 50);
                layer_refresh(lyr_back, 8, 96, 16, 112);
            }
        }
    }
}
