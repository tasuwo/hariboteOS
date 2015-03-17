; haribote-os
; TAB=4

BOTPAK  EQU     0x00280000              ; bootpack�̃��[�h��
DSKCAC  EQU     0x00100000              ; �f�B�X�N�L���b�V���̏ꏊ
DSKCAC0 EQU     0x00008000              ; �f�B�X�N�L���b�V���̏ꏊ�i���A�����[�h�j


;----------- BOOT_INFO�F�N������� -----------;
CYLS    EQU     0x0ff0                  ; �ǂݍ��ރV�����_�[���DIPL�ɂ��ݒ肳���D
LEDS    EQU     0x0ff1                  ; �L�[�{�[�h��LED���
VMODE   EQU     0x0ff2                  ; �F���Ɋւ�����D��bit�J���[���H
SCRNX   EQU     0x0ff4                  ; �𑜓x��X
SCRNY   EQU     0x0ff6                  ; �𑜓x��Y
VRAM    EQU     0x0ff8                  ; �O���t�B�b�N�o�b�t�@�̊J�n�Ԓn
;---------------------------------------------;


    ORG     0xc200          ; ���̃v���O�������ǂ��ɓǂݍ��܂��̂�


    ;------------ INT�F�r�f�I���[�h�ݒ� ----------------;
    MOV     AL,0x13                     ; ���[�h�w��
                                        ; 0x03�F16�F
                                        ; 0x12�F640*480*4bit�J���[
                                        ; 0x13�F320*200*8bit�J���[
    MOV     AH,0x00                     ; ����
    INT     0x10                        ; �r�f�I���[�h�ݒ�
    ;--------------------------------------------------;


    MOV     BYTE [VMODE],8              ; ��ʃ��[�h�̃���
    MOV     WORD [SCRNX],320            ; �X�N���[����x��
    MOV     WORD [SCRNY],200            ; �X�N���[����y��
    MOV     DWORD [VRAM],0x000a0000     ; VRAM��0xa0000�`0xaffff��64KB
                                        ; ����̉�ʃ��[�h�ł́C�̘b
                                        ; ����64KB���ɒl���������߂Ε`��ł���


    ;------------- INT�F�L�[�{�[�h��Ԏ擾 ------------------;
    MOV     AH,0x02                     ; �L�[�{�[�h��LED��Ԃ��擾
    INT     0x16                        ; keyboard BIOS
    ;-------------------------------------------------------;


    MOV     [LEDS],AL                   ; �L�[�{�[�h��Ԃ̃���

;---------------------- �ǋL���� ---------------------;
; �����͌��

        ; �}�X�^�C�X���[�u�̊��荞�݂��֎~����(bootpack.h�Q��)
        ;   AT�݊��@�̎d�l�ł́APIC�̏�����������Ȃ�A
        ;   ������CLI�O�ɂ���Ă����Ȃ��ƁA���܂Ƀn���O�A�b�v����
        ;   PIC�̏������͂��Ƃł��
        MOV     AL,0xff
        OUT     0x21,AL         ; PIC0��IMR�Ɋ��荞�݋֎~��ݒ�
        NOP                     ; OUT���߂�A��������Ƃ��܂������Ȃ��@�킪����炵���̂ŁC1�N���b�NCPU�x�~
        OUT     0xa1,AL         ; PIC1��IMR�Ɋ��荞�݋֎~��ݒ�
        CLI                     ; �����CPU���x���ł����荞�݋֎~

        ; CPU����1MB�ȏ�̃������ɃA�N�Z�X�ł���悤�ɁAA20GATE��ݒ�
        CALL    waitkbdout      ; �҂����
        MOV     AL,0xd1
        OUT     0x64,AL         ; KBC�Ƀ��[�h�ݒ肷�邱�Ƃ�ʒm
        CALL    waitkbdout      ; �҂����
        MOV     AL,0xdf
        OUT     0x60,AL         ; A20GATE�M������ON�ɂ���(�����L�������Ȃ��ƁC1MB�ȏ�̃������ɃA�N�Z�X�ł��Ȃ�)
        CALL    waitkbdout      ; �҂����

[INSTRSET "i486p"]              ; 486�̖��߂܂Ŏg�������Ƃ����L�q
        LGDT    [GDTR0]         ; �b��GDT��ݒ�
        MOV     EAX,CR0         ; �ݒ�̂��߁CCR0�̓��e��EAX�Ɋi�[
        AND     EAX,0x7fffffff  ; bit31��0�ɂ���i�y�[�W���O�֎~�̂��߁j
        OR      EAX,0x00000001  ; bit0��1�ɂ���i�v���e�N�g���[�h�ڍs�̂��߁j
        MOV     CR0,EAX         ; CR0��EAX�̒l��߂��C�ݒ芮��
        JMP     pipelineflush
pipelineflush:                  ; �Z�O�����g���W�X�^�̈Ӗ����ς�������߁C�e�Z�O�����g���W�X�^������������
        MOV     AX,1*8          ;  �ǂݏ����\�Z�O�����g32bit
        MOV     DS,AX
        MOV     ES,AX
        MOV     FS,AX
        MOV     GS,AX
        MOV     SS,AX

        ; �e�탁�����̓]������
        MOV     ESI,bootpack    ; �]����(admhead.nas(���̃t�@�C��)�̃��X�g)
        MOV     EDI,BOTPAK      ; �]����
        MOV     ECX,512*1024/4
        CALL    memcpy          ; bootpack��0x28000�ɓǂݍ���

        ; �f�B�X�N�f�[�^��{���̈ʒu�֓]��
        MOV     ESI,0x7c00      ; �]����
        MOV     EDI,DSKCAC      ; �]����
        MOV     ECX,512/4
        CALL    memcpy          ; 0x7c00�����512byte(�u�[�g�Z�N�^)��DSKCAC�ڍs�փR�s�[

        MOV     ESI,DSKCAC0+512 ; �]����(�u�[�g�Z�N�^�̒��ォ��)
        MOV     EDI,DSKCAC+512  ; �]����(�u�[�g�Z�N�^��ǂݍ��񂾒��ォ��)
        MOV     ECX,0
        MOV     CL,BYTE [CYLS]
        IMUL    ECX,512*18*2/4  ; �V�����_������o�C�g��/4�ɕϊ�
        SUB     ECX,512/4       ; IPL�̕�������������
        CALL    memcpy          ; �f�B�X�N�̎c��S�Ă��ړ�

; asmhead�ł��Ȃ���΂����Ȃ����Ƃ͑S�����I������̂ŁA
;   ���Ƃ�bootpack�ɔC����

; bootpack�̋N��

        MOV     EBX,BOTPAK
        MOV     ECX,[EBX+16]
        ADD     ECX,3           ; ECX += 3;
        SHR     ECX,2           ; ECX /= 4;
        JZ      skip            ; �]������ׂ����̂��Ȃ�
        MOV     ESI,[EBX+20]    ; �]����
        ADD     ESI,EBX
        MOV     EDI,[EBX+12]    ; �]����
        CALL    memcpy
skip:
        MOV     ESP,[EBX+12]    ; �X�^�b�N�����l
        JMP     DWORD 2*8:0x0000001b

waitkbdout:
        IN       AL,0x64
        AND      AL,0x02
        JNZ     waitkbdout      ; AND�̌��ʂ�0�łȂ����waitkbdout��
        RET

memcpy:
        MOV     EAX,[ESI]
        ADD     ESI,4
        MOV     [EDI],EAX
        ADD     EDI,4
        SUB     ECX,1
        JNZ     memcpy          ; �����Z�������ʂ�0�łȂ����memcpy��
        RET
; memcpy�̓A�h���X�T�C�Y�v���t�B�N�X�����Y��Ȃ���΁A�X�g�����O���߂ł�������

        ALIGNB  16
GDT0:
        RESB    8               ; �k���Z���N�^
        DW      0xffff,0x0000,0x9200,0x00cf ; �ǂݏ����\�Z�O�����g32bit
        DW      0xffff,0x0000,0x9a28,0x0047 ; ���s�\�Z�O�����g32bit�ibootpack�p�j

        DW      0
GDTR0:
        DW      8*3-1
        DD      GDT0

        ALIGNB  16
bootpack:
