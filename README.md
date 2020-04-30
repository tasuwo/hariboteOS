# これは何

下記の書籍を参考に実装中の自作OS.

> [30日でできる! OS自作入門](https://www.amazon.co.jp/dp/B00IR1HYI0/ref=cm_sw_em_r_mt_dp_U_mlSQEbW278A5Z)

# 開発環境

動作確認環境は macOS Catalina。

``` shell
$ sw_vers
ProductName:	Mac OS X
ProductVersion:	10.15.4
BuildVersion:	19E287
```

書籍で対象としているのは Windows であることと、macOS Catalina から 32bit ソフトウェアは起動できなくなってしまった。そのため、Docker で Haribote OS の開発環境を構築できる [tolenv](https://github.com/HariboteOS/tolenv) を利用させていただくことにした。

## Requirements

### Docker

下記からインストールする。

> https://docs.docker.com/get-docker/

### qemu

```shell
$ brew install qemu
```

### tolenv

``` shell
$ git clone https://github.com/HariboteOS/tolenv
$ cd tolenv

# Dockerイメージの取得
$ make pull

# コンテナの起動
$ make up

# コンテナの終了
$ make down
```

## hariboteOSの起動

``` shell
# `tolenv` の `z_tools` 以下に本プロジェクトを clone
$ cd z_tools
$ git clone https://github.com/tasuwo/hariboteOS

# qemu 起動
$ cd hariboteOS
$ make run
```

