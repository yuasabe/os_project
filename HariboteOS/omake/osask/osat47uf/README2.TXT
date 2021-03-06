        OSASK/AT ver.4.7(USB-FDD image版)の説明 (アプリケーション編)
                                                 2004/12/27 川合秀実

  このドキュメントは最初に読むべき内容ではありません。まずはCOPYRIGH.TXTを読み
、その後にREADME0.TXTを読んで、その次にREADME1.TXT、そして最後にこの説明を読め
ば分かりやすいと思います。

  このドキュメントでは、OSAIMGUF.BIN内に収録されているアプリケーションの説明を
しています。OSASK/ATそのものの使い方やディスクイメージをディスクに書き込む方
法などは、README0.TXTとREADME1.TXTを参照してください。

  ここに収録されているアプリケーションは、2004/12/27の時点で「川合秀実推奨」と
されているものです。川合秀実推奨でないアプリケーションも存在し、それらも容易に
ダウンロードできます。

  なお、説明文中に(要FPU)と書いてあるものは、i387のないi386マシンや、i486SXマ
シンでは実行しないでください。

  以下に、各アプリケーションの簡単な説明を挙げます。

---

・cnTdwn3
  このプログラムは、100から数を数え下げるものです。数え下げは1秒ごとに行われる
ので100秒を測るのに利用できるかもしれませんが、実用性は無いでしょう。

・revers03
  リバーシプログラムです。マウスに対応していない上に、すべてキャラクターでの表
示のため、使いにくいですし見にくいのですが、それなりには楽しめます。

  起動すると最初に対戦相手を設定します。選択肢は、青も赤もそれぞれ、「human」
、「com-0」〜「com-4」です。カーソルの左右で選択できます。決定はEnter。訂正し
たいときは、BackSpaceでできるようになっています。

  人間vs人間もできますし、コンピューターvsコンピューターもできます。com-0〜com
-4の思考レベルは以下の通りです。

○com-0：ランダム打ち。とても弱い。
○com-1：ランダム打ちよりはまし。全く手を読んでいない。
○com-2：空きマスが10個以下なら最善手を打ちます。それ以外は1手先を読んで考えて
         います。
○com-3：空きマスが12個以下なら最善手を打ちます。それ以外は3手先を読んで考えて
         います。
○com-4：空きマスが13個以下なら最善手を打ちます。それ以外は4手先を読んで考えて
         います。

  対戦が始まって、人間の番になったら、着手可能な場所が控えめに表示されます。こ
のアドバイスが不要な場合は、Homeキーを押して下さい。再び必要になったら、またHo
meキーを押して下さい。

  着手可能な場所がある場合、ルール上、パスは出来ません。アルファベットと数字で
入力して下さい。2文字入力すれば、Enterは不要です。BackSpaceも使えます。

  着手可能な場所が一つもない場合、パスしか出来ません。画面には、「ps」と表示さ
れます。確認のため、Enterを押して下さい。

  「待った」をすることもできます。PageUp/PageDownで、undo/redoです。

  対戦中、試合放棄したくなったらEndキーを押して下さい。

  試合が終わったら、画面に「GAME SET.」と表示されます。Enterを入力すれば、最初
の対戦相手選択画面に戻ります。

  全ての場面で、Enterの代わりにスペースキーを使うことができます。

・msgk01c
  この2つのプログラムは、画面上に動くメッセージを表示するものです。起動すれば
、メッセージが表示されます。

・s_world
  これはBEEP音で簡単なメロディーを演奏するプログラムです。著作者である小柳さん
の許可をいただいて、収録させていただきました。表示が凝っています。

・invader5
  ちょっとしたインベーダーゲームです。カーソルの左右で自機が動き、スペースキー
でミサイルを発射します。ミサイルを撃つと次のミサイルの発射の準備があるので、し
ばらくの間撃てなくなります。しばらくとはいっても1秒にも満たない短い時間です。
しかしこの制約のためにミサイルを連射することはできず、ゲームが難しくなっていま
す。インベーダーをやっつけたときに加算される得点は、ミスが少ないほうが大きいで
す。できるだけミスしないように心がけて、高得点を狙ってください。

  インベーダーが一匹でも最下段に達するとゲームオーバーです。

  現在川合の知るところでは、26520点が最高得点です（川合秀実 2002.06.15）。

・mt_xor1
  山(？)が描かれて、色が変化するだけです。グラフィック表示のサンプルです。

・ktetrac1 (要FPU : 386や486SXでは実行できません)
  ワイヤーフレーム表示の正四面体がくるくる回ります。主著作者であるSolidさんの
許可をいただいて、収録させていただきました。

・moskey
  キーボードを鍵盤に見立てて、音楽を演奏するアプリケーションです。著作者である
Myurikaさんの許可をいただいて、収録させていただきました。

  キーボード文字キー、上から奇数段を黒鍵、偶数段を白鍵に見立てます。基準になる
キーは4段目がC、2段目がQで、標準ではそれぞれオクターブ4のc、5のcとなります。文
字キーは全て使うため、全体で2オクターブ+αの音域をカバーします。

  左Altキーでホールドし、スペースキーのトグルでポルタメントをON/OFFします。

  基準となる音域を「←」キーで下げ、「→」キーで上げます。

・loadfac2
  アプリケーションの実行時に際して、どのくらいのCPU時間を消費しているのかを計
測するアプリケーションです。使い方は、全てのアプリケーションを閉じたあとに、こ
のアプリケーションを起動して数秒待ち(2〜5秒で充分です)、その後に目的のアプリケ
ーションを起動するだけです。負荷は2つの数値で表示されますが、上が1秒間の平均値
、下が16秒間での平均値です。なお、このプログラムのアルゴリズム上の問題で、負荷
が50%を超えるようなものは正確に測定できません(シェルの設定でタスクの実行時間比
をコントロールすれば50%以上の負荷もきちんと測れますが、操作がややこしくなるの
で説明は省略します)。

  また、loadfac2だけを起動している状態でも負荷が0でない事がありますが、それは
誤差です。アプリケーションを起動しなくても、マウスの移動や、キー入力だけでも負
荷は変化します。それくらい敏感には作ってあります。OSASKの性能を測るための参考
にしていただければと思います(いくつかの測定結果とその考察が、FOR_PROG.TXTにあ
ります)。

・kcube02 (要FPU : 386や486SXでは実行できません)
  このアプリケーションを実行すると、ポリゴンで描かれた立方体がゆっくり回ります
。そういうデモです。きれいなので僕は気に入っています(笑)。

・teditc02
  任意のテキストファイルを表示・編集します。とりあえず、このディスクイメージの
中にREADMEJP.TXTというファイルを入れておいたので、それを選択してみてください。
詳しい説明は長くなるのでここには書いてありません。READMEJP.TXTに書いてあるので
読んでください。

  teditc02では日本語だけではなく韓国語も表示できますが、この機能を利用するには
韓国語のフォントファイルKOR16V00.FNTが必要です。このフォントファイルは追加パッ
ケージに含まれています。追加パッケージは

    http://osask.jp/download.html

からダウンロードできます。韓国語モードに切り替える方法などはREADMEJP.TXTを参照
してください。

・kmandel5 (要FPU : 386や486SXでは実行できません)
  マンデルブロー表示アプリケーションです。カーソルキーでカーソルを動かし(Ctrl
と併用すると移動速度が上がります)「z」で拡大、「x」で縮小です(必ず小文字で入力
してください)。模様が込み入っている部分を拡大するときれいです。

・beditc00
  バイナリーエディターです。任意のファイルの内容を書き換えることができます。

  アプリケーションを起動すると、ファイルセレクターが出ます。それでファイルを選
んで中身を見てください。

  使用可能なキーは、以下の通りです。

  O (open) :
    現在扱っているファイルを閉じて、違うファイルを開きます。
  M (mode) :
    モードを変更します。read-onlyとwritableがあります。
  R (refresh) :
    他のタスクで変更された部分を画面に反映します。
  カーソルキー :
    カーソルの移動。シフトキーと併用すると移動量が増えます。
  0〜9, A〜F :
    16進数の入力。writableの時のみ有効。

  なお、ファイルサイズの変更を伴うような作業は、今のバージョンでは出来ません。

  これはどんなファイルでも書き換えられる危険なアプリケーションです。READMEJP.T
XTを書き換えてみるくらいが無難でしょう。

・resizer0
  ファイルサイズ変更ツールです。

  アプリケーションを起動すると、ファイルセレクターが出ます。ファイルを選ぶと現
在のサイズが表示されます。そして新しいサイズを10進数で入力してEnterを押せば、
ファイルサイズが変更されます。beditc00にファイルサイズ変更能力がないために、用
意しました。本来ファイルサイズの変更は必要に応じてアプリケーションが行うべきで
resizer0のようなアプリは必要ないのですが、beditc00の改良が間に合わなかったので
とりあえず入れておきました。

・calm21
  カレンダー表示アプリケーションです。著作者であるかりんさんの許可をいただいて
収録させていただきました。カーソルの左右で月が変わり、上下で年が変わります。

・mswp08
  マインスイーパーのゲームです。主にマウスを使って操作します。著作者であるかり
んさんの許可をいただいて収録させていただきました。
    左クリックでマス目を開きます。
    右クリック1回でマス目に旗マークをつけます。
    右クリック2回でマス目に？マークをつけます。
    数字が表示されたマス目の周囲に同じ数の旗マークを付けたら、左右同時クリック
      で囲に残っているマス目が一度に開きます。
　　キーボードの「１、２、３」でレベルを変更できます。

・icalc1
  整数しか扱えない簡易電卓です。's'で符号反転、'c'でクリアできます。計算結果は
'='ではなくEnterを使ってください。Gakuさんの作ったプログラムをベースに川合がい
じって作りました。

・bball3
  ファイル名は"beautiful ball"のつもりですが、要するにラインだけで描いた簡単な
絵です。I.Tak.さんが初版のbballc1を改良してくれました。

・helo
  へらへらアニメプレイヤーです。へらへらアニメというのは、FM-TOWNSで有名なフリ
ーソフトで、まずはこのディスクイメージ内のKAOSC.HELやKAOS2C.HELを選択してみて
ください。このアプリケーションとデーターは著作者のI.Tak.さんの許可をいただいて
収録させていただいています。

・mcopyc1
  ファイルをコピーするだけのアプリです。最初の選択でコピー元を選択し、次の選択
でコピー先を選択します。コピー先のファイルはあらかじめファイルサイズが0になっ
ていなければコピー動作はやりません(安全のためです)。コンソールなどで新規ファイ
ルを生成するとサイズ0のファイルがすぐに作れます。

・mmlplay
  MMLファイルをBEEP音で演奏します。これも小柳さん作です。データのLALALU.MMLは
小柳さんが作曲したオリジナル曲のMMLです。DAIGO.MMLとDAIKU.MMLは川合が楽譜から
起こしたものです。

・onkan
  ZAKKYさん作の絶対音感ゲームです。鳴った音をC,D,E,F,G,A,Bで答えてください。正
解すると「Good!」間違うと「Bad!」と出ます。3回間違えるとゲームオーバーです。

・rrose
  I.Tak.さん作のデモソフトです。指定した.BMPファイルがきれいに回転します。とり
あえずROSE.BMPを選んでみてください。TEST128.BMPを選んでもいいでしょう。256x256
以下のサイズのものがいいと思います(それ以上でも自動でセンタリングされますが)。
カーソルの上下でズームができます。

・katakk
  くーみんさん作のぷよぷよ風ゲームです。ぷよぷよと違うのは、降ってくるのがぷよ
ではなくてカオちゃんの顔であることと、その顔の種類がぷよぷよよりも多いことです
。4つを並べれば消えるという点は同じです(計算式は違うかもしれませんが)。連鎖す
ると高得点が出やすいというのも同じです。ただ、同じ連鎖でもゲームのスピードが速
くなってからの方が高得点です。ゲームの速度が上がってくると、カーソルの下を押し
ているだけでも結構な点数になります(もっとも、そんな余裕はなくなるんですが)。

  カオちゃんが消えると、その消えた個数の合計を4で割った数に相当する数のねずみ
が降ってきます。このねずみはいわゆるおじゃまぷよです。降る位置はランダムではな
くカオちゃんが積もっていないところを優先して落ちてきます。同じ高さのところがあ
れば、左が優先です。これにより自分の好きな位置にねずみを降らせることができ、連
鎖に利用できますが、まあたいていは邪魔だと思うでしょうね(笑)。

  今までの最高得点は、106797点です(2002.09.05 くーみんさん)。もしこれを超えられ
たら教えてください。

・canvas
  I.Tak.さん作の簡易グラフィックエディタです。マウスで線を引けます。右クリック
はスポイトで、エディットしている絵から色が拾えるのはもちろんですが、画面下部の
色見本からもこのスポイト機能を使って色を拾えます。少し手間がかかりますが、既存
の絵をロードすることもできます。まずfキーを押してファイルセレクタを出して、BMP
ファイルを選びます。ディスクアクセスが終わったら、rを押してください。一度fでフ
ァイルを選んでおけば、いつでもrを押すだけで読み込めますし、wで書き込めます。詳
しいドキュメントは下記のI.Tak.さんのページからパッケージをダウンロードすると読
めます。奥が深いのです。

・oyaji
  I.Tak.さん作の目玉アプリです。目玉アプリっていうのは、X-WindowでおなじみのEye
sみたいなものです。マウスカーソルの方をきょろきょろと見てくれる環境アプリです。
oyajiは一つ目ですので、二つが好きな人は2つ起動して2個並べてください。

・btntest
  KIYOTOさん作のマウスボタンアプリです。アプリががんばれば今のAPIだってマウスボ
タンが作れるという例です(もちろんだからといって、API側の手抜きが正当化されるわ
けではありません・・・今後の課題です)。ぽちぽち押してみてください。

・test043a
  32bitグラフィックボックスのサンプルです。簡単なグラデーションパターンを表示し
ています。

・osagame
  さめがめゲームのOSASK版です。最初はスコアが-200点から始まります。上下左右に繋
がった丸をクリックすると消えます。なお、最低でも2個に繋がっていないと消せません
。3個以上繋がった状態で消す