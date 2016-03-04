; hello-os
; TAB=4

    CYLS    EQU 10              ; 読み込むシリンダー数
    SCTS    EQU 18              ; 読み込むセクタ数

    ORG     0x7c00              ; このプログラムがどこに読み込まれるのか


;------- 標準的なFATフォーマットフロッピーディスクのための記述 -------;
    JMP     entry               ; ラベル[entry]へジャンプ
    DB      0x90
    DB      "HARIBOTE"          ; ブートセクタの大きさ
    DW      512                 ; 1セクタの大きさ
    DB      1                   ; クラスタの大きさ
    DW      1                   ; FATがどこから始まるか
    DB      2                   ; FATの個数
    DW      224                 ; ルートディレクトリ領域の大きさ
    DW      2880                ; このドライブの大きさ
    DB      0xf0                ; メディアのタイプ
    DW      9                   ; FAT領域の長さ
    DW      18                  ; 1トラックに幾つのセクタがあるか
    DW      2                   ; ヘッドの数
    DD      0                   ; パーティションをつかていない場合は0
    DD      2880                ; このドライブの大きさをもう一度書く
    DB      0,0,0x29            ; よくわからない(?)
    DD      0xffffffff          ; たぶんボリュームシリアル番号
    DB      "HARIBOTEOS "       ; フォーマットの名前(11byte)
    DB      "FAT12   "          ; フォーマットの名前(8byte)
    RESB    18                  ; とりあえず18byteあけておく
;---------------------------------------------------------------------;


;---------------------------- プログラム本体 --------------------------;
entry:
    MOV     AX,0                ; レジスタ初期化
    MOV     SS,AX               ; スタックセグメント初期化
    MOV     SP,0x7c00           ; スタックポインタにプログラムのよみこまれる番地を格納
    MOV     DS,AX               ; データセグメント初期化

; ディスクを読む

    MOV     AX,0x0820
    MOV     ES,AX               ; バッファアドレス 0x0820 を格納
    MOV     CH,0                ; シリンダ0
    MOV     DH,0                ; ヘッド0
    MOV     CL,2                ; セクタ2．IPLがセクタ1によみこまれるので，ディスクの中身はセクタ2毎に読み込む
readloop:
    MOV     SI,0                ; 失敗回数をカウントする
retry:
    MOV     AH,0x02             ; AH=0x02 : ディスク読み込み
    MOV     AL,1                ; 1セクタ
    MOV     BX,0                ; バッファアドレス ES=0x0820, BX=0のため，
                                ; ディスクデータは0x8200〜0x83ffまでに読み込まれる
                                ; 0x8000〜0x8200にはブートセクタの内容を入れる予定
    MOV     DL,0x00             ; Aドライブ
    INT     0x13                ; ディスクBIOS呼び出し
    JNC     next                ; エラーが起きなければnextへjump
    ADD     SI,1                ; エラー回数をカウント
    CMP     SI,5                ; 失敗回数が5回であれば
    JAE     error               ; errorへジャンプ
    MOV     AH,0                ; リセット準備
    MOV     DL,0x00             ; リセット準備
    INT     0x13                ; ドライブのリセット
    JMP     retry
next:
    MOV     AX,ES
    ADD     AX,0x0020
    MOV     ES,AX               ; 読み込む番地を0x0020ずらす
    ADD     CL,1                ; セクタを1ずらす
    CMP     CL,SCTS             ; すべてのセクタを読み込んだか？
    JBE     readloop            ; 読み込んでいなければreadloopへjump
    MOV     CL,1                ; セクタを1にセット
    ADD     DH,1                ; ヘッドを1ずらす
    CMP     DH,2                ; ヘッドを裏表読み込んだか？
    JB      readloop            ; 読み込んでいなければreadloopへjump
    MOV     DH,0                ; ヘッドを0にセット
    ADD     CH,1                ; シリンダを1つずらす
    CMP     CH,CYLS             ; すべてのシリンダを読み込んだか？
    JB      readloop            ; 読み込んでいなければreadloopへjump
    MOV     [0x0ff0],CH         ; IPLがどこまで読んだのかをメモリにメモしておく
                                ; なぜこの番地なのかはわからない．とりあえずメモリマップを確認してみると空いているようだ．
    JMP     0xc200              ; 0x8000からディスクの読み込みが始まっている
                                ; ファイルの中身は0x004200に入るらしい(実際にバイナリエディタで確かめてみたところ)
                                ; よって，OSの実態は 0x8000 + 0x4200 = 0xc200から始まるので，そこへjump


fin:
    HLT                         ; CPU休止
    JMP     fin

error:
    MOV     SI,msg              ; エラーの場合は，メッセージ表示
                                ; SIにメッセージの格納された番地を指定
putloop:
    MOV     AL,[SI]             ; メモリのSI番地を読み込む
    ADD     SI,1                ; SIに1を足す
    CMP     AL,0                ; 文字を表示し終えたならfinへjump
    JE      fin
    MOV     AH,0x0e             ; 1文字表示関数呼び出しのための指定
    MOV     BX,15               ; カラーコード
    INT     0x10                ; ビデオBIOS呼び出し
    JMP     putloop
msg:
    DB      0x0a,0x0a           ; 改行を2つ
    DB      "ERROR!"
    DB      0x0a                ; 改行
    DB      0

    RESB    0x7dfe-$             ; 0x001feまでを0x00で埋める

    DB      0x55, 0xaa
;---------------------------------------------------------------------;