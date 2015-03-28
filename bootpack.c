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
    struct MOUSEINFO minfo;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, key, i;
    unsigned int memtotal;
    struct MEMMANAGE *memman = (struct MEMMANAGE *) MEMMANAGE_ADDR;

    /* ���������� */
    init_gdtidt();                                          // GDT�CIDT������
    init_pic();                                             // PIC������
    io_sti();                                               // ���荞�݋֎~������
    fifo8_init(&keyfifo, 32, keybuf);                       // �L�[�{�[�h�p�o�b�t�@������
    fifo8_init(&mousefifo, 32, mousebuf);                   // �}�E�X�p�o�b�t�@������
    io_out8(PIC0_IMR, 0xf9);                                // ���荞�݋��F�L�[�{�[�h��PIC1(11111001)
    io_out8(PIC1_IMR, 0xef);                                // ���荞�݋��F�}�E�X(11101111)

    init_keyboard();                                        // KBC�̏�����(�}�E�X�g�p���[�h�ɐݒ�)
    enable_mouse(&minfo);                                   // �}�E�X�L����

    init_palette();                                         // �p���b�g������
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);  // OS������ʕ`��

    /* �}�E�X�J�[�\���̕`�� */
    mx = (binfo->scrnx - 16) / 2;                                           // ���W�v�Z(��ʒ���)
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);                               // mcursor�Ƀ}�E�X��������
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);    // mcursor��`��
    sprintf(s, "(%d, %d)", mx, my);                                         // ���W���������ɏ�������
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);         // ���W�̕`��

    /* �������`�F�b�N */
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    sprintf(s, "memory %dMB  free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

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
                sprintf(s, "%02X", key);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                /* �f�[�^�̎擾 */
                key = fifo8_dequeue(&mousefifo);
                /* ���荞�݃t���O���Z�b�g���� */
                io_sti();

                // �}�E�X�̍��W�`��
                if (mouse_decode(&minfo, key) != 0){
                    /* �f�[�^��3�o�C�g�������̂ŕ\�� */
                    sprintf(s, "[lcr %4d %4d]", minfo.x, minfo.y);
                    if ((minfo.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((minfo.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((minfo.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                }

                /* �}�E�X�ړ� */
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* �}�E�X���� */
                    mx += minfo.x;
                    my += minfo.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16) {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16) {
                        my = binfo->scrny - 16;
                    }
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); /* ���W���� */
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); /* ���W���� */
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); /* �}�E�X�`�� */
            }
        }
    }
}
