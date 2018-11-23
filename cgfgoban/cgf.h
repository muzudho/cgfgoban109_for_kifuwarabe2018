/*** cgf.h ***/
#define CGF_E		// 英語版はこれを定義。リソースは英語版、日本語版を統一。
//#define CGF_DEV	// 開発版

#define BAN_9   9 // ９路盤
#define BAN_13 13 // 13路盤
#define BAN_19 19 // 19路盤
#define BANMAX ((19+2)*256)	// = 21*256		将来は21*32=672にしたい。8分の1になるし。10bitで座標を表現。やはり面倒か・・・

#define KIFU_MAX 2048	// 記録可能な最大手数

extern int ban_size;	// 盤の大きさ 9,13,19;
extern int tesuu;		// 手数
extern int all_tesuu;	// 最後までの手数
extern int kifu[KIFU_MAX][3];		// 棋譜	 [0]...位置、[1]...色、[2]...思考時間（その手の）


// make_move()関数で手を進めた時の結果
enum MoveResult {
	MOVE_SUCCESS,	// 成功
	MOVE_SUICIDE,	// 自殺手
	MOVE_KOU,		// コウ
	MOVE_EXIST,		// 既に石が存在
	MOVE_FATAL,		// それ以外

	// 以下はset_kifu_time()で利用
	MOVE_PASS_PASS,	// 連続パス、対局終了
	MOVE_KIFU_OVER,	// 手数が長すぎ
	MOVE_RESIGN,	// 投了
};


extern int kou_iti;			// 劫になる場合の位置
extern int hama[2];			// 黒と白の揚浜（取り石の数）

extern int ban[];			// 囲碁盤の状態。空白 0、黒 1、白 2、壁 3
extern int ban_start[];		// 囲碁盤の開始状態。
extern int dll_col;			// dllが黒か(1)白か(2)

extern double komi;			// コミ。6.5で6目半。0で互先

extern int thinking_utikiri_flag;	// 中断フラグ
extern int total_time[2];	// 累計の思考時間
						
extern int ban_9[][11];		// 初期盤面配置状態
extern int ban_13[][15];
extern int ban_19[][21];


#define UN_COL(col) (3-(col))	// 相手の石の色


#define SGF_BUF_MAX 65536			// SGFを読み込むためのバッファ
extern unsigned long nSgfBufSize;	// バッファにロードしたサイズ
extern unsigned long nSgfBufNum;	// バッファの先頭位置を示す
extern char SgfBuf[SGF_BUF_MAX];	// バッファ

#define IsKanji(c) (( 0x80<(c) && (c)<0xa0 ) || ( 0xdf<(c) && (c)<0xfd ))	// 漢字の先頭バイトか？ c はunsigned charでないとバグる

#define TMP_BUF_LEN	256			// 文字列用の適当な配列長さ

// cgf_term.cpp
extern int RetZ;		// 通信で受け取る手の座標
extern int RetComm;		// 帰ってきたデータの種類
extern int nCommNumber;	// COMポートの番号。


//////////////////////////////////////////////////////////////////////////



// 関数のプロトタイプ宣言
// cgf_win.cpp
void PRT(char *fmt, ...);			// 可変のリストを持つprintf代用関数
void PassWindowsSystem(void);		// Windowsへ一時的に制御を渡す。User からの指示による思考停止。
void LoadSetSGF(int fSgf);			// SGFをロードして盤面を構成する
void SaveSGF(void);					// SGFを書き出す
void move_tesuu(int n);				// 任意の手数の局面に盤面を移動させる
int ThinkingDLL(int endgame_type);	// 思考ルーチン
int count_endgame_score(void);		// 終局処理をした後で、地の数のみを数える。(黒地 － 白地)

// cgf.cpp
void reconstruct_ban(void);		// 盤面の情報のみから
void hyouji_clipboard(void);	// クリップボードに盤面をコピー
void hyouji(void);				// テキストでの盤面表示	
void debug(void);				// エラー処理
void print_tejun(void);			// 今までの探索手順を表示
void set_tejun_kifu(void);		// 今までの手順を棋譜にセット。遡れるように。
void hyouji_saizen(void);		// 最善応手手順を表示する
void set_saizen_kifu(void);
void init_ban(void);

// cgf_main.cpp
int make_move(int z, int col);	// 手を進める。z ... 座標、col ... 石の色

int count_total_time(void);				// 累計の思考時間を計算しなおすだけ。現在の手番の累計思考時間を返す
int set_kifu_time(int z,int col,int t);	// 棋譜に手を書き込んで局面を進めて画面を更新 
double GetSpendTime(clock_t ct1);		// 経過時間を秒で返す
int GetTurnCol(void);					// 現在の手番の色を返す（現在手数と初手が黒番かどうかで判断する）
int IsBlackTurn(void);					// 黒の手番か？


