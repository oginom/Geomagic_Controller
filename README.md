# Geomagic_Controller
## about
Geomagic Touchをコントロールするプログラム。
## environments
### HW
#### Geomagic Touch
### SW
#### OS: Windows 10
* Windows 8だとより簡単に動くと思われる。
## requirements
### Microsoft Visual Studio 2017
### Geomagic Touch Devide Driver v2016.10.13
https://3dssupport.microsoftcrmportals.com/knowledgebase/article/KA-01460/en-us
### OpenHaptics for Windows Developer Edition v3.4
installer: https://3dssupport.microsoftcrmportals.com/knowledgebase/article/KA-03283/en-us

Windows 10の場合、DLLファイルをリビルドする必要がある。

"C:\OpenHaptics\Developer\3.4.0\utilities\src\Utilities_VS2010.sln"などのソリューションを開いて、
HapticMouse、HDU、HLU、SnapConstraints、の4つのプロジェクトをそれぞれ右クリック→リビルドする。
### msvcp120d.dllとmsvcr120d.dll
(Windows 8の場合は不要)

適当に取得して配置すること。

Visual Studio上から動かす場合はvcxprojファイルと同じ階層、
ビルドしたものを実行する場合はexeファイルと同じ階層に置く。
## run
* まずGeomagic TouchをPCにLANケーブルで繋いで電源を入れる。
* Geomagic Touch Setupを使ってペアリングする。
* 実行して何かキーを押すと開始。
* 別プログラムからコマンドを送る。
* ウィンドウのxボタンで終了
## protocol
* かかる力は、現在の座標と目標座標の間にばね定数K1=0.05N/mmの仮想ばねを置いている。
* 通信はソケット通信サーバーで行っている。デフォルトのIPとPortはlocalhostの54321
* 一度に1つのクライアントと接続する。解除されたら再び接続待ちになる。
* 1コマンドに対して1レスポンス
### commands
#### t %lf,%lf,%lf
* 目標座標を更新
* resp: OK or NG
* 可動範囲内にもうけた立方体の範囲外ならば更新せずNGを返す。
#### p
* 現在座標を取得
* resp: %.3f,%.3f,%.3f
* 現在座標を返す。
#### b
* ボタンの状態を取得
* resp: %d
* ボタン1→1、ボタン2→2、の和を返す。
## TODO
* rotationを取得
* 提示する力の種類を増やす
  * 仮想質量の重力と加速度に対する反作用力
  * ばねに加えて仮想ダンパ
  * 通知用のバイブレーション
* マシン内通信としてソケット通信はそんなに速くないらしいので、問題が起こる可能性あり
  * 必要に応じてセマフォやFIFO、共有メモリなどにする
* PhantomIOLib42.pdb は読み込まれていません問題
* 1回おきにdevice initialization failする問題
