# AO\_builtin

## これは何？

伺かのAOプロトコルを利用したシェル描画プログラムのリファレンス実装です。
ベースウェアから処理を切り離すことを目的として作られています。

現状ninix-kagariのみの対応です。

## Requirements

- SDL3
- SDL3-image
- SDL3-ttf
- fontconfig(Linux)
- wayland-client(Linux)
- onnxruntime(optional)

## How to Build

```bash
$ git clone --recursive https://github.com/Tatakinov/ao_builtin
$ cd ao_builtin
$ make
```

## 使い方

`ao_builtin.exe`を適当なディレクトリ`/path/to/ao`に放り込みます。

ninixを次の様に呼び出します。

```
NINIX_ENABLE_SORAKADO=1 AO_PATH="/path/to/ao" ninix
```

## シェル倍率の変更に超解像を利用する

`make`の代わりに`make -f Makefile.onnx`してバイナリ生成を行います。

`ao_builtin.exe`と*同じ*ディレクトリに`model.onnx`を置くことで
シェルサイズの変更に超解像を用いて綺麗な拡大を行います。

## かろうじて出来ること

- サーフェスの移動(に伴うバルーンの移動)
- OnMouseClick
- ディスプレイ間の移動

## 注意

1. 時々落ちる
2. 作りが雑
3. OpenGL周りがさっぱり分かってない人の書いたコード
4. マルチスレッドのこともよく分かってない

仕様以外の部分はあまり参考にしないように。

## LICENSE

基本的にはMIT-0ですが、ソースファイルにライセンスの記述があるものはそちらが優先されます。

libfontlist(MIT)を利用しています。

また、以下のソースコードを参考にしています。
[ONNXRuntime-example](https://github.com/microsoft/onnxruntime-inference-examples/blob/main/c_cxx/MNIST/MNIST.cpp) / MIT License (C) Microsoft
