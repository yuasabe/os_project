IMGTOLの使い方                                     2003.12.27 川合秀実

１．これはなにか？

  IMGTOLはDOS汎用ファンクションを使ってディスクイメージを作ったり、ディスクイメ
ージを書き込んだりするツールです。その他にもディスクイメージを扱う上で便利では
ないかと思われる機能や、OS開発に使えそうな機能を持っています。

  しかし何といっても最大の特徴は、OSの配布時にバンドルしても全く問題にならない
ライセンスが適用されていることと、バンドルしても邪魔にはならないそのサイズでし
ょう。2.33KBバイトです（笑）。

２．簡単な使い方の表

    read.   >imgtol r [opt] drive: filename  size  (drv -> file)
    write.  >imgtol w [opt] drive: filename [size] (file -> drv)
    exp-w.f.>imgtol f [opt] drive: filename (file -> drv)
    exp-w.  >imgtol F [opt] drive: filename (file -> drv)
    ovrcopy.>imgtol c inputfile outputfile  [size]
    expand. >imgtol e inputfile outputfile   size
    release.>imgtol R sourceimage compedimage
    exe2sys.>imgtol s inputfile outputfile base(dec)
      opt(sector-bytes) = -512(default), -1024
      size unit : kilobyte

３．ディスクイメージをディスクに書き込む方法

  PCATの場合（というか1440KB-FDの場合）
    prompt>imgtol w a: fdimage.bin

  TOWNSやNEC98の場合（というか1232KB-FDの場合）
    prompt>imgtol w -1024 a: fdimage.bin

  この場合だとファイルが尽きるまで書き込みます。ディスクが尽きてもDOSからエラー
が来ない限り、書き込み命令を送り続けます（笑）。ディスク容量よりも大きなファイ
ルを扱うときなどは、

  PCATの場合（というか1440KB-FDの場合）
    prompt>imgtol w a: fdimage.bin 1440

  TOWNSやNEC98の場合（というか1232KB-FDの場合）
    prompt>imgtol w -1024 a: fdimage.bin 1232

などとしてください。上限が設定され、それ以上はアクセスしません。なおアクセスは
高速化のために36KB(-512時)、もしくは16KB(-1024時)単位で行なっています。以前の
バージョンではこの単位でのオーバーランがありえましたが、このバージョンはオーバ
ランしません。

  「a:」とか「fdimage.bin」の部分は自分の使いたい状況に合わせて書き換えてくださ
い。HDDのドライブや他のデバイスにもできます（註1）。なお31MB以上にアクセスする
ことはできず、そのうち勝手に正常終了してしまいます。末尾のサイズ指定も32767以上
を指定しないようにしてください。

  なお、途中経過は一切表示されません。気長に待ちましょう。また「ディスクの内容
が失われます、よろしいですか？」みたいなこともいいません。そういうのを付けたけ
れば、バッチファイルを作ってそこでechoやpauseをしてください。

<註1>
  HDDやCFへ書き込もうとすると、Win95などではエラーになります。しかしこれは回避
する方法があります。例えば、ドライブE:へ書き込みたい場合、

    prompt>lock e:
    prompt>imgtol w e: cfimage.bin
    prompt>unlock e:

とすれば問題なく書き込めます。


４．ディスクを読んでディスクイメージを作る方法

  PCATの場合（というか1440KB-FDの場合）
    prompt>imgtol r a: fdimage.bin 1440

  TOWNSやNEC98の場合（というか1232KB-FDの場合）
    prompt>imgtol r -1024 a: fdimage.bin 1232 

  サイズ指定は省略できません。実ディスクよりも少ない値も指定できます。その場合
はそこまでしか読みません。

  「a:」とか「fdimage.bin」の部分は自分の使いたい状況に合わせて書き換えてくださ
い。HDDのドライブや他のデバイスにも問題なくできますが、31MB以上にはアクセスでき
ないので、それはご了承ください。31MBを超えてアクセスできるのはpcctolです。

  なお、途中経過は一切表示されません。気長に待ちましょう。

５．ディスクイメージのサイズの変更の方法

  たとえば600KBのAT互換機用ディスクイメージがあって、「これじゃあ不便じゃあ〜、
僕は1440KBのイメージがほしいんだあ！」というときは次のようにします。

    prompt>imgtol e src.bin dest.bin 1440

こうすると、600KBのsrc.binを拡張して1440KBにしたものがdest.binとして出力されま
す。同じファイルを指定することはできません。必ず入力ファイル名と出力ファイル名
は別々にしてください。誤って同じファイルを指定するとファイルが壊れます。なお、
1440KBに拡張と言っても、要するにただ「00」で埋まったセクタを要求サイズになるま
で付け足しているだけです。なにもいじっていません。

  なお、指定したファイルよりも小さいサイズを指定することもでき、その場合は、フ
ァイルの後ろがカットされた結果が出てきます。作ったディスクイメージの後