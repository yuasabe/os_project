----------------------------------------------------------------------
                      OSASK/AT ver.4.7 操作説明書
----------------------------------------------------------------------

  OSASKの使い方の説明です。インストールや準備については, インストール
説明書をご覧ください。

1.終了方法
‾‾‾‾‾‾‾‾‾‾
　Ctrl+Alt+Deleteでリセットできます。あるいは, リセットスイッチか電源
スイッチを押してください。ディスクアクセス中でなければ, いつ電源を切っ
てもいいように設計してあります。


2.ウィンドウについて
‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
　一番上にあるウィンドウのみタイトルバーが青くなり, 操作できます。これ
をアクティブになっているといいます。アクティブではないウィンドウは灰色
になったり黒くなったりします。
　ウィンドウが重なると, 下になったウィンドウはタイトルバーが黒くなり,
一切描画されません。これはバグではなくて仕様です。他のOSのように下にな
ってもちゃんと描画するようにもできますし, 将来はそうすることになるでし
ょうが, 今はこの仕様で我慢してください。
　なお, 下にある灰色の領域は「雰囲気」です (笑) 。


3.マウス操作
‾‾‾‾‾‾‾‾‾‾‾‾
　PS/2マウスが接続されていればマウスカーソルが動きます。USBマウスは使
えません。また, 画面の端にカーソルを持っていくと, 仮想画面のある限りス
クロールします。
　マウスのどのボタンでも, クリックしたウィンドウをアクティブにすること
ができます。その後でウィンドウを閉じたり移動したりできます。


4.キー操作
‾‾‾‾‾‾‾‾‾‾
　現在、OSASKが認識するキー操作は, 以下の10種類です。
他のキーは基本的にアプリケーションへの入力として扱われます。

  ・Ctrl+Alt+Delete
    リセットします。

  ・F1
    画面モードを640x480 16色に切り換えます。

  ・F2
    画面モードを800x600 16色に切り換えます。

  ・F3
    画面モードを1024x768 64K色に切り換えます (pokonのsetvesaコマンド
    で割り当てを変えられます)。起動するとこの画面モードに切り替わる
    はずです (非対応のビデオカードでは上の640x480 16色になります)。

  ・F4
    画面モードを1280x1024 256色に切り換えます (pokonのsetvesaコマンド
    で割り当てを変えられます)。

※画面からはみ出すウィンドウがある場合, 画面モード切り替えはできません。

  ・F5
    フォントファイルを読み込みます (この操作をしなくても必要に応じて自
    動的に読み込みます) 。

  ・F7
    壁紙の表示・非表示を切り換えます。壁紙については 8.壁紙 を見てくだ
    さい。

  ・F9 または Alt+Tab
    一番下のウィンドウを一番上に出します。

  ・F10
    一番上と二番目のウィンドウを入れ換えます。

  ・F11
    ウィンドウを移動します。カーソルキーで移動, Enterで終わります。

  ・F12 または Alt+F4
    ウィンドウを閉じます。


5.pokonランチャー
‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
　OSASK ver.4.7が起動すると, すぐにアプリケーションランチャーpokonが出
ます。pokonはファイルを選択して起動することなどができます。
　またアプリケーションが出すファイル選択ウィンドウもpokonの一部ですの
で, 以下の説明と同じ操作 (ディスク入れ換えなど) ができます。


5.1.pokonの操作
‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
  まずはキー操作から。

  ・カーソルキー上下
    ファイルを選択します。

  ・カーソルキー左右
    ページ単位でカーソルを移動します。左で上に, 右で下に行きます。

  ・PageUp, PageDown
    ページ単位でスクロールします。

  ・適当な文字
    その文字で始まるファイルを順に選択します (サーチ) 。今のところファ
    イル名は全て大文字で始まっています。大文字を入力してください。

  ・Enter
    選択したファイルを実行します (開きます) 。

  ・Ctrl+0 から Ctrl+9
    CFやハードディスクを使っているとき, 使うディスクイメージを切り換え
    ます (0から9がFDIMAGE0.BINからFDIMAGE9.BINに対応)。ディスク入れ換え
    と同じように扱われます。

  ・Ctrl+T
    選択したファイルをteditc02で開きます。

  ・Ctrl+B
    選択したファイルをbeditc00で開きます。

  ・Ctrl+U
    選択したファイルをmcopyc1でコピーします。

  ・Ctrl+C
    へっぽこコンソールが出ます。

※このCD-ROM版では、以下のInsert/Shift+Insert/Delete/Shift+Deleteの
  操作はしないでください。

  ・Insert
    ディスクの内容を読み込み直します。ディスクを入れ換えたら入力してく
    ださい。ただし, 書き換えたファイルがあると使えません。その時はディ
    スクを戻してDeleteキーを押してください。
    また, この操作をすると開いてあるファイル