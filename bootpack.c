/**
 * bootpack.c
 * main
 */

#include <stdio.h>
#include "bootpack.h"

/* main */
void HariMain(void)
{
    /* asmhead.nas�Ń��������l���擾 */
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], keybuf[32], mousebuf[128];
    int mx, my, key;
    unsigned int memtotal;
    struct MOUSEINFO minfo;
    struct MEMMANAGE *memman = (struct MEMMANAGE *) MEMMANAGE_ADDR;
    struct LYRCTL *lyrctl;
    struct LAYER *lyr_back, *lyr_mouse;
    unsigned char *buf_back, buf_mouse[256];

    /* ���������� */
    init_gdtidt();                                          // GDT�CIDT������
    init_pic();                                             // PIC������
    io_sti();                                               // ���荞�݋֎~������
    fifo8_init(&keyfifo, 32, keybuf);                       // �L�[�{�[�h�p�o�b�t�@������
    fifo8_init(&mousefifo, 128, mousebuf);                  // �}�E�X�p�o�b�t�@������
    io_out8(PIC0_IMR, 0xf9);                                // ���荞�݋��F�L�[�{�[�h��PIC1(11111001)
    io_out8(PIC1_IMR, 0xef);                                // ���荞�݋��F�}�E�X(11101111)

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
    // �e���C���̃o�b�t�@(�}�E�X��256�Œ�)
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);

    /* ���C���ݒ� */
    // ���C������\���̂Ƃ̃o�C���f�B���O
    layer_setbuf(lyr_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);
    layer_setbuf(lyr_mouse, buf_mouse, 16, 16, 99);
    // �o�b�t�@��������
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);  // OS�������
    init_mouse_cursor8(buf_mouse, 99);                   // �}�E�X
    // �D��x��ݒ�
    layer_updown(lyrctl, lyr_back,  0);
    layer_updown(lyrctl, lyr_mouse, 1);

    /******************************** �`�� ***********************************/
    /*** �}�E�X���C�� ***/
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    layer_slide(lyrctl, lyr_mouse, mx, my);                            // �}�E�X�`��ʒu
    /*** �w�i���C�� ***/
    layer_slide(lyrctl, lyr_back, 0, 0);                               // �w�i�`��ʒu
    // �}�E�X�J�[�\���̍��W
    sprintf(s, "(%3d, %3d)", mx, my);                                  // �������ɏ�������
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);       // �o�b�t�@�Ɋi�[
    // �������e�ʁC�������󂫏��
    sprintf(s, "memory %dMB  free : %dKB",
            memtotal / (1024 * 1024), memman_total(memman) / 1024);    // �������ɏ�������
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);      // �o�b�t�@�Ɋi�[
    /*** �e���C���̕`�� ***/
    layer_refresh(lyrctl, lyr_back, 0, 0, binfo->scrnx, 48);
    /*************************************************************************/

    /* CPU�x�~ */
    while(1){
        /* ���荞�݋֎~�ɂ��� */
        io_cli();

        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
            // ���荞�݂��Ȃ����
            /* ���荞�݃t���O���Z�b�g���Chlt���� */
            io_stihlt();
        } else {
            /* �L�[�{�[�h�C�}�E�X�̏��ɒ��ׂ� */
            if (fifo8_status(&keyfifo) != 0) {
                /* �L�[��ǂݍ��� */
                key = fifo8_dequeue(&keyfifo);
                /* ���荞�݃t���O���Z�b�g���� */
                io_sti();
                /* �L�[�̕\�� */
                sprintf(s, "%02X", key);                                         // �������ɕ�����������o��
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);    // �o�b�t�@�̃N���A
                putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);    // �������o�b�t�@�Ɋi�[
                layer_refresh(lyrctl, lyr_back, 0, 16, 16, 32);                  // ������̕\���͈͂̕`����X�V
            } else if (fifo8_status(&mousefifo) != 0) {
                /* �f�[�^�̎擾 */
                key = fifo8_dequeue(&mousefifo);
                /* ���荞�݃t���O���Z�b�g���� */
                io_sti();

                // �}�E�X�̍��W�`��
                if (mouse_decode(&minfo, key) != 0){
                    /* �f�[�^��3�o�C�g�������̂ŕ\�� */
                    sprintf(s, "[lcr %4d %4d]", minfo.x, minfo.y);                                 // �������ɕ�����������o��
                    if ((minfo.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((minfo.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((minfo.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);    // �o�b�t�@�̃N���A
                    putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);                 // �������o�b�t�@�Ɋi�[
                    layer_refresh(lyrctl, lyr_back, 32, 16, 32 + 15 * 8, 32);                      // ������̕\���͈͂̕`����X�V
                    /* �}�E�X�J�[�\���̈ړ� */
                    mx += minfo.x;
                    my += minfo.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }
                    sprintf(s, "(%3d, %3d)", mx, my);                            // �������ɕ�����������o��
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // �`��͈͂̃N���A
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); // �������o�b�t�@�Ɋi�[
                    layer_refresh(lyrctl, lyr_back, 0, 0, 80, 16);               // �`��͈͂̕\���X�V
                    layer_slide(lyrctl, lyr_mouse, mx, my);
                }
            }
        }
    }
}
