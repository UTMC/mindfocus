/****************************
 *                          *
 * mindfocus configure file *
 *                          *
 ****************************/

/*
 *
 * インストール場所や初期化ファイル名の設定
 *
 */

/* インストールディレクトリ */
#define INSTALLDIR ${HOME}/bin

/* デフォルトデータをインストールする場所 */
#define DATADIR ${HOME}/share/mindfocus

/* 個人環境保存ファイル(設定ファイル)の名前 */
#define DOTFILENAME ".mindfocus"


/*
 *
 * ライブラリの使用を制御
 *
 */


/* 様々な形の窓を実現する拡張。現在、これなしでは作動しません。*/
#define SHAPE

/* XPMフォーマットに対応させるときは定義する。今のところ事実上必須。 */
#define XPM
#define XPMLIBRARY -L/usr/local/X11R6/lib -lXpm


/*
 *
 * その他
 *
 */

/* for SunOS */
/* SunOSで使用する際は定義して下さい。 */
/*#define SUNOS*/

/* アニメーション処理をするかどうか */
/* 管理がずさんなので定義しないとコンパイルが通らない？ */
#define ANIMATION

/* ヴァージョン情報 */
#define VERSION 0.87

/* デバッグフラグ */
/* 各種デバッグメッセージを表示するようになる */
/*#define DEBUG*/

/*
 *
 * ここから先は編集しないで下さい。
 *
 */

#ifdef SUNOS
#define RAND_MAX 65535
#endif /* SUNOS */
