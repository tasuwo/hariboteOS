; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; オブジェクトファイル作成モード
[INSTRSET "i486p"]      ; 486用のプログラム
                        ; CPUの系譜
                        ; 8086 > 80186 > 286 > 386 > 486 > ...
[BITS 32]               ; 32bitモード用の機械語を作成する

; objファイル用の情報
[FILE "naskfunc.nas"]               ; ソースファイル名

    ; 関数宣言
    ; 引数はESP(extra stack pointer)に格納される
    ; 第一引数[ESP+ 4]
    ; 第二引数[ESP+ 8]
    ; 第三引数[ESP+12]
    ; また，C言語の規約ではRETされた時にFAXに入っていた値が関数の値とみなされる
    GLOBAL  _io_hlt, _io_cli, _io_sti, _io_stihlt
    GLOBAL  _io_in8,  _io_in16,  _io_in32
    GLOBAL  _io_out8, _io_out16, _io_out32
    GLOBAL  _io_load_eflags, _io_store_eflags
    GLOBAL  _load_gdtr, _load_idtr
    GLOBAL  _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
    EXTERN  _inthandler21, _inthandler27, _inthandler2c

; 関数定義
[SECTION .text]                 ; プログラム記述のための宣言

; void io_hlt(void)
; HLTを呼び出す
_io_hlt:
    HLT
    RET

; void io_cli(void)
; clear interrupt flag
_io_cli:
    CLI
    RET

; void io_sti(void)
; set interrupt flag
_io_sti:
    STI
    RET

; void io_stihlt(void)
; 割り込みフラグをセットし，HLTする
_io_stihlt:
    STI
    HLT
    RET

; int io_in8(int port)
; 指定したポートから1byteの情報を受信する
; @param port ポート番号
_io_in8:
    MOV     EDX,[ESP+4]         ; 引数をデータレジスタに格納
    MOV     EAX,0               ; 累積演算器レジスタの初期化
    IN      AL,DX               ; 返り値？
    RET

; int io_in16(int port)
; 指定したポートから2byteの情報を受信する
; @param port ポート番号
_io_in16:   ; int io_in16(int port);
        MOV     EDX,[ESP+4]     ; 引数をデータレジスタに格納
        MOV     EAX,0           ; 累積演算器レジスタの初期化
        IN      AX,DX
        RET

; int io_in32(int port)
; 指定したポートから4byteの情報を受信する
; @param port ポート番号
_io_in32:
        MOV     EDX,[ESP+4]     ; 引数をデータレジスタに格納
        IN      EAX,DX
        RET

; int io_out8(int port)
; 指定したポートへ1byteの情報を送信する
; @param port ポート番号
_io_out8:
        MOV     EDX,[ESP+4]     ; port
        MOV     AL,[ESP+8]      ; data
        OUT     DX,AL
        RET

; int io_out16(int port)
; 指定したポートへ2byteの情報を送信する
; @param port ポート番号
_io_out16:
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,[ESP+8]     ; data
        OUT     DX,AX
        RET

; int io_out32(int port)
; 指定したポートへ4byteの情報を送信する
; @param port ポート番号
_io_out32:
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,[ESP+8]     ; data
        OUT     DX,EAX
        RET

; int io_load_eflags(void)
; EFLAGSレジスタをロードする
_io_load_eflags:
        PUSHFD                  ; スタックにEFLAGSの中身をpush
        POP     EAX             ; 返り値にpop
        RET

; void io_store_eflags(int eflags)
; EFLAGSレジスタを格納する
_io_store_eflags:
        MOV     EAX,[ESP+4]
        PUSH    EAX             ; 引数をスタックに積む
        POPFD                   ; EFLAGSの中身にpop
        RET

; void load_gdtr(int limit, int addr);
; GDTRレジスタに設定を格納する
; GDTRはセグメントを利用するための設定テーブル
; セグメントの先頭番地と有効設定個数を設定する
; GDTRは6byteあり，上位4byteはGDTの置いてある番地，下位2byteはGCTの有効byte数を表す
; LGDT命令は，番地を指定すると，そこから6byte(48bit)を読み取ってGDTRレジスタに格納する
; 第一引数は[ESP+4]，第二引数は[ESP+8]に存在する
; 今回，第一引数は有効byte数，第二引数はGDTを置く番地を表す
; 第一引数を下位2byte，第二引数を上位4vyteとする必要がある
; [ESP+4]から2つ番地をずらして[ESP+6]へ書き込むと，さらに番地を2つずらした[ESP+8]からちょうど第二引数が始まる
; よって，[ESP+6]に第一引数を書き込み，LGDTに[ESP+6]を渡せば良い
_load_gdtr:
        MOV     AX,[ESP+4]
        MOV     [ESP+6],AX
        LGDT    [ESP+6]
        RET

; void load_idtr(int limit, int addr);
; IDTRレジスタに設定を格納する
; IDTRレジスタは割り込み処理のためのテーブル
_load_idtr:
        MOV     AX,[ESP+4]      ; limit
        MOV     [ESP+6],AX
        LIDT    [ESP+6]
        RET

_asm_inthandler21:
        PUSH    ES              ;
        PUSH    DS              ;
        PUSHAD                  ; 32bitレジスタを全てpush
        MOV     EAX,ESP
        PUSH    EAX
        MOV     AX,SS           ; DSとESをSSと同等になるよう調整
        MOV     DS,AX
        MOV     ES,AX
        CALL    _inthandler21   ; 外部プログラムの実行
        POP     EAX
        POPAD                   ; すべてのレジスタを元に戻す
        POP     DS
        POP     ES
        IRETD

_asm_inthandler27:
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX,ESP
        PUSH    EAX
        MOV     AX,SS
        MOV     DS,AX
        MOV     ES,AX
        CALL    _inthandler27
        POP     EAX
        POPAD
        POP     DS
        POP     ES
        IRETD

_asm_inthandler2c:
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX,ESP
        PUSH    EAX
        MOV     AX,SS
        MOV     DS,AX
        MOV     ES,AX
        CALL    _inthandler2c
        POP     EAX
        POPAD
        POP     DS
        POP     ES
        IRETD