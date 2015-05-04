(main

 ; １回目の実行
 (begin (define state 0) ; 変数を定義
	(chg-grp 0)      ; 画像0番を表示
	100)              ; 次の呼び出しは100ミリ秒後

 ; ２回目以降の実行
 ; 目パチのアニメーション
 ;  100ミリ秒毎に画像を切り替えて
 ;  最後にランダムなウェイトを入れる
 (cond ((= state 0) (begin (chg-grp 1)
			   (setq state 1)
			   100))
       ((= state 1) (begin (chg-grp 2)
			   (setq state 2)
			   100))
       ((= state 2) (begin (chg-grp 3)
			   (setq state 3)
			   100))
       ((= state 3) (begin (chg-grp 0)
			   (setq state 0)
			   (+ (rand 5000) 1)))))

