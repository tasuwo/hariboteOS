/**
 * bootpack.c
 * main
 */

#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

struct MOUSEINFO{
    unsigned char phase,buf[3];
    int x, y, btn;
};

/* main */
void HariMain(void)
{
    /* asmhead.nas�Ń��������l���擾 */
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSEINFO minfo;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, key;

    /* ���������� */
    init_gdtidt();                                          // GDT�CIDT������
    init_pic();                                             // PIC������
    io_sti();                                               // ���荞�݋֎~������
    fifo8_init(&keyfifo, 32, keybuf);                       // �L�[�{�[�h�p�o�b�t�@������
    fifo8_init(&mousefifo, 32, mousebuf);                   // �}�E�X�p�o�b�t�@������
    io_out8(PIC0_IMR, 0xf9);                                // ���荞�݋��F�L�[�{�[�h��PIC1(11111001)
    io_out8(PIC1_IMR, 0xef);                                // ���荞�݋��F�}�E�X(11101111)
    init_keyboard();                                        // KBC�̏�����(�}�E�X�g�p���[�h�ɐݒ�)
    init_palette();                                         // �p���b�g������
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);  // OS������ʕ`��
    enable_mouse(&minfo);                                   // �}�E�X�L����

    /* �}�E�X�J�[�\���̕`�� */
    mx = (binfo->scrnx - 16) / 2;                                           // ���W�v�Z(��ʒ���)
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);                               // mcursor�Ƀ}�E�X��������
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);    // mcursor��`��
    sprintf(s, "(%d, %d)", mx, my);                                         // ���W���������ɏ�������
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);         // ���W�̕`��

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

#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064  // KBC
#define KEYSTA_SEND_NOTREADY    0x02    // KBC�̐��䖽�ߎ�t�\����p
#define KEYCMD_WRITE_MODE       0x60    // ���[�h�ݒ�̂��߂̃��[�h
#define KBC_MODE                0x47    // �}�E�X�𗘗p���邽�߂̃��[�h

/**
 * KBC���f�[�^���M�\�ɂȂ�̂�҂�
 */
void wait_KBC_sendready(void){
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

/**
 * KBC�̏�����
 */
void init_keyboard(void){
    wait_KBC_sendready();                       // �҂����
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);    // ���[�h��ݒ肷�邱�Ƃ�ʒm
    wait_KBC_sendready();                       // �҂����
    io_out8(PORT_KEYDAT, KBC_MODE);             // ���[�h�ݒ�(�}�E�X�g�p���[�h)
    return;
}

#define KEYCMD_SENDTO_MOUSE     0xd4    // �}�E�X�ւ̖��ߑ��M
#define MOUSECMD_ENABLE         0xf4    // �}�E�X�ւ̗L��������

/**
 * �}�E�X�ɗL�������߂𑗐M����
 */
void enable_mouse(struct MOUSEINFO *mf){
    wait_KBC_sendready();                       // �҂����
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);  // �}�E�X�Ƀf�[�^�𑗐M����
    wait_KBC_sendready();                       // �҂����
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);      // �L�������߂𑗐M
    mf->phase = 0;
    return; /* �}�E�X����ACK(0xfa)�����M����Ă��� */
}

int mouse_decode(struct MOUSEINFO *mf, unsigned char data){
    switch (mf->phase){
        case 0:
            if(data == 0xfa){
                mf->phase++;
            }
            return 0;
            break;
        case 1:
            /**
             * 1byte�ځF�ړ��ɔ�������D0�`3��
             * 2bute�ځF�N���b�N�ɔ�������D8�`F��
             */
            if((data & 0xc8) == 0x08){
                mf->buf[0] = data;
                mf->phase++;
            }
            return 0;
            break;
        case 2:
            mf->buf[1] = data;
            mf->phase++;
            return 0;
            break;
        case 3:
            mf->buf[2] = data;
            mf->phase = 1;
            /* �f�[�^�̉��� */
            mf->btn = mf->buf[0] & 0x07;
            mf->x = mf->buf[1];
            mf->y = mf->buf[2];
            if ((mf->buf[0] & 0x10) != 0) {
                mf->x |= 0xffffff00;
            }
            if ((mf->buf[0] & 0x20) != 0) {
                mf->y |= 0xffffff00;
            }
            mf->y = - mf->y; /* �}�E�X�ł�y�����̕�������ʂƔ��� */
            return 1;
            break;
    }
    return -1;
}
