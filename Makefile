SOURCEPATH   = src/
HEADERPATH   = $(SOURCEPATH)header/
OBJECTPATH   = obj/
RESOURCEPATH = resources/
TOOLPATH     = ../../z_tools/
LIBINCPATH   = ../../z_tools/haribote/
INCPATH      = include/

CSOURCES=$(wildcard $(SOURCEPATH)*.c)
ASEMBLESOURCES=$(wildcard $(SOURCEPATH)*.nas)
OBJS_BOOTPACK= \
	$(addprefix $(OBJECTPATH), $(notdir $(CSOURCES:.c=.obj))) \
	$(addprefix $(OBJECTPATH), $(notdir $(ASEMBLESOURCES:.nas=.obj))) \
	$(OBJECTPATH)hankaku.obj

MAKE     = make -r
NASK     = $(TOOLPATH)nask
CC1      = $(TOOLPATH)gocc1 -I$(LIBINCPATH) -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask -a
OBJ2BIM  = $(TOOLPATH)obj2bim
MAKEFONT = $(TOOLPATH)makefont
BIN2OBJ  = $(TOOLPATH)bin2obj
BIM2HRB  = $(TOOLPATH)bim2hrb
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg
IMGTOL   = $(TOOLPATH)imgtol
COPY     = cp
DEL      = rm -f

# デフォルト動作

default :
	$(MAKE) img

# ファイル生成規則

# nask でアセンブラを bin にコンパイル
$(OBJECTPATH)ipl10.bin : $(HEADERPATH)ipl10.nas Makefile
	$(NASK) $(HEADERPATH)ipl10.nas $(OBJECTPATH)ipl10.bin $(OBJECTPATH)ipl10.lst

$(OBJECTPATH)asmhead.bin : $(HEADERPATH)asmhead.nas Makefile
	$(NASK) $(HEADERPATH)asmhead.nas $(OBJECTPATH)asmhead.bin $(OBJECTPATH)asmhead.lst

# makefont でフォントの bin -> obj 生成
$(OBJECTPATH)hankaku.bin : $(RESOURCEPATH)hankaku.txt Makefile
	$(MAKEFONT) $(RESOURCEPATH)hankaku.txt $(OBJECTPATH)hankaku.bin

$(OBJECTPATH)hankaku.obj : $(OBJECTPATH)hankaku.bin Makefile
	$(BIN2OBJ) $(OBJECTPATH)hankaku.bin $(OBJECTPATH)hankaku.obj _hankaku

# obj 同士をリンクし，bim を生成
$(OBJECTPATH)bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) \
		out:$(OBJECTPATH)bootpack.bim \
		stack:3136k \
		map:$(OBJECTPATH)bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

# 「はりぼてOS」用の形式に変換
$(OBJECTPATH)bootpack.hrb : $(OBJECTPATH)bootpack.bim Makefile
	$(BIM2HRB) $(OBJECTPATH)bootpack.bim $(OBJECTPATH)bootpack.hrb 0

# cat コマンドにより asmhead と bootpack を連結し，bootpack.hrb を生成
$(OBJECTPATH)haribote.sys : $(OBJECTPATH)asmhead.bin $(OBJECTPATH)bootpack.hrb Makefile
	cat $(OBJECTPATH)asmhead.bin $(OBJECTPATH)bootpack.hrb > $(OBJECTPATH)haribote.sys

# edimg によってディスクイメージに保存
haribote.img : $(OBJECTPATH)ipl10.bin $(OBJECTPATH)haribote.sys Makefile
	$(EDIMG)   imgin:../../z_tools/fdimg0at.tek \
		wbinimg src:$(OBJECTPATH)ipl10.bin len:512 from:0 to:0 \
		copy from:$(OBJECTPATH)haribote.sys to:@: \
		imgout:haribote.img

# 一般規則

# cc (Cコンパイラ) でCコードをアセンブリコード(.gas)にコンパイル
$(SOURCEPATH)%.gas : $(SOURCEPATH)%.c Makefile
	$(CC1) -o $(SOURCEPATH)$*.gas $(SOURCEPATH)$*.c

# アセンブラ gas 用のアセンブリコードを，nask 用に変換する
$(SOURCEPATH)%.nas : $(SOURCEPATH)%.gas Makefile
	$(GAS2NASK) $(SOURCEPATH)$*.gas $(SOURCEPATH)$*.nas

# アセンブラ nask によってアセンブルし obj を生成
$(OBJECTPATH)%.obj : $(SOURCEPATH)%.nas Makefile
	$(NASK) $(SOURCEPATH)$*.nas $(OBJECTPATH)$*.obj $(OBJECTPATH)$*.lst

# コマンド

img :
	$(MAKE) haribote.img

run :
	$(MAKE) img
	$(COPY) haribote.img ../../z_tools/qemu/fdimage0.bin
	$(MAKE) -C ../../z_tools/qemu

install :
	$(MAKE) img
	$(IMGTOL) w a: haribote.img

clean :
	-$(DEL) obj/*

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img
