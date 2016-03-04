; haribote-os
; TAB=4

BOTPAK  EQU     0x00280000              ; bootpackのロード先
DSKCAC  EQU     0x00100000              ; ディスクキャッシュの場所
DSKCAC0 EQU     0x00008000              ; ディスクキャッシュの場所（リアルモード）


;----------- BOOT_INFO：起動時情報 -----------;
CYLS    EQU     0x0ff0                  ; 読み込むシリンダー数．IPLにより設定される．
LEDS    EQU     0x0ff1                  ; キーボードのLED状態
VMODE   EQU     0x0ff2                  ; 色数に関する情報．何bitカラーか？
SCRNX   EQU     0x0ff4                  ; 解像度のX
SCRNY   EQU     0x0ff6                  ; 解像度のY
VRAM    EQU     0x0ff8                  ; グラフィックバッファの開始番地
;---------------------------------------------;


    ORG     0xc200          ; このプログラムがどこに読み込まれるのか


    ;------------ INT：ビデオモード設定 ----------------;
    MOV     AL,0x13                     ; モード指定
                                        ; 0x03：16色
                                        ; 0x12：640*480*4bitカラー
                                        ; 0x13：320*200*8bitカラー
    MOV     AH,0x00                     ; 準備
    INT     0x10                        ; ビデオモード設定
    ;--------------------------------------------------;


    MOV     BYTE [VMODE],8              ; 画面モードのメモ
    MOV     WORD [SCRNX],320            ; スクリーンのx幅
    MOV     WORD [SCRNY],200            ; スクリーンのy幅
    MOV     DWORD [VRAM],0x000a0000     ; VRAMは0xa0000〜0xaffffの64KB
                                        ; 今回の画面モードでは，の話
                                        ; この64KB内に値を書き込めば描画できる


    ;------------- INT：キーボード状態取得 ------------------;
    MOV     AH,0x02                     ; キーボードのLED状態を取得
    INT     0x16                        ; keyboard BIOS
    ;-------------------------------------------------------;


    MOV     [LEDS],AL                   ; キーボード状態のメモ

;---------------------- 追記部分 ---------------------;
; 理解は後回し

        ; マスタ，スレーブの割り込みを禁止する(bootpack.h参照)
        ;   AT互換機の仕様では、PICの初期化をするなら、
        ;   こいつをCLI前にやっておかないと、たまにハングアップする
        ;   PICの初期化はあとでやる
        MOV     AL,0xff
        OUT     0x21,AL         ; PIC0のIMRに割り込み禁止を設定
        NOP                     ; OUT命令を連続させるとうまくいかない機種があるらしいので，1クロックCPU休止
        OUT     0xa1,AL         ; PIC1のIMRに割り込み禁止を設定
        CLI                     ; さらにCPUレベルでも割り込み禁止

        ; CPUから1MB以上のメモリにアクセスできるように、A20GATEを設定
        CALL    waitkbdout      ; 待ち状態
        MOV     AL,0xd1
        OUT     0x64,AL         ; KBCにモード設定することを通知
        CALL    waitkbdout      ; 待ち状態
        MOV     AL,0xdf
        OUT     0x60,AL         ; A20GATE信号線をONにする(これを有効化しないと，1MB以上のメモリにアクセスできない)
        CALL    waitkbdout      ; 待ち状態

[INSTRSET "i486p"]              ; 486の命令まで使いたいという記述
        LGDT    [GDTR0]         ; 暫定GDTを設定
        MOV     EAX,CR0         ; 設定のため，CR0の内容をEAXに格納
        AND     EAX,0x7fffffff  ; bit31を0にする（ページング禁止のため）
        OR      EAX,0x00000001  ; bit0を1にする（プロテクトモード移行のため）
        MOV     CR0,EAX         ; CR0にEAXの値を戻し，設定完了
        JMP     pipelineflush
pipelineflush:                  ; セグメントレジスタの意味が変わったため，各セグメントレジスタを初期化する
        MOV     AX,1*8          ;  読み書き可能セグメント32bit
        MOV     DS,AX
        MOV     ES,AX
        MOV     FS,AX
        MOV     GS,AX
        MOV     SS,AX

        ; 各種メモリの転送処理
        MOV     ESI,bootpack    ; 転送元(admhead.nas(このファイル)のラスト)
        MOV     EDI,BOTPAK      ; 転送先
        MOV     ECX,512*1024/4
        CALL    memcpy          ; bootpackを0x28000に読み込む

        ; ディスクデータを本来の位置へ転送
        MOV     ESI,0x7c00      ; 転送元
        MOV     EDI,DSKCAC      ; 転送先
        MOV     ECX,512/4
        CALL    memcpy          ; 0x7c00からの512byte(ブートセクタ)をDSKCAC移行へコピー

        MOV     ESI,DSKCAC0+512 ; 転送元(ブートセクタの直後から)
        MOV     EDI,DSKCAC+512  ; 転送先(ブートセクタを読み込んだ直後から)
        MOV     ECX,0
        MOV     CL,BYTE [CYLS]
        IMUL    ECX,512*18*2/4  ; シリンダ数からバイト数/4に変換
        SUB     ECX,512/4       ; IPLの分だけ差し引く
        CALL    memcpy          ; ディスクの残り全てを移動

; asmheadでしなければいけないことは全部し終わったので、
;   あとはbootpackに任せる

; bootpackの起動

        MOV     EBX,BOTPAK
        MOV     ECX,[EBX+16]
        ADD     ECX,3           ; ECX += 3;
        SHR     ECX,2           ; ECX /= 4;
        JZ      skip            ; 転送するべきものがない
        MOV     ESI,[EBX+20]    ; 転送元
        ADD     ESI,EBX
        MOV     EDI,[EBX+12]    ; 転送先
        CALL    memcpy
skip:
        MOV     ESP,[EBX+12]    ; スタック初期値
        JMP     DWORD 2*8:0x0000001b

waitkbdout:
        IN       AL,0x64
        AND      AL,0x02
        JNZ     waitkbdout      ; ANDの結果が0でなければwaitkbdoutへ
        RET

memcpy:
        MOV     EAX,[ESI]
        ADD     ESI,4
        MOV     [EDI],EAX
        ADD     EDI,4
        SUB     ECX,1
        JNZ     memcpy          ; 引き算した結果が0でなければmemcpyへ
        RET
; memcpyはアドレスサイズプリフィクスを入れ忘れなければ、ストリング命令でも書ける

        ALIGNB  16
GDT0:
        RESB    8               ; ヌルセレクタ
        DW      0xffff,0x0000,0x9200,0x00cf ; 読み書き可能セグメント32bit
        DW      0xffff,0x0000,0x9a28,0x0047 ; 実行可能セグメント32bit（bootpack用）

        DW      0
GDTR0:
        DW      8*3-1
        DD      GDT0

        ALIGNB  16
bootpack:
