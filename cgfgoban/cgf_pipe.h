// cgf_pipe.h

// cgf_gtp.cpp
void SendGTP(char *str);
int ReadGTP(void);	// パイプからGnuGoの出力を読み出す
int ReadCommmand(int *z);	// コマンドを受信する。手の受信も兼ねる。Errorの場合は0を返す。

int open_gnugo_pipe(void);	// パイプを作成
void close_gnugo_pipe(void);
int PlayGnugo(int endgame_type);
void UpdateGnugoBoard(int z);	// Gnugo側の盤面も動かす
void PutStoneGnuGo(int z, int black_turn);
int init_gnugo_gtp(char *sName);
void FinalStatusGTP(int *p_endgame);	// GTP Engineで死活判定で死活表示を行う

//void test_gnugo_loop(void);

extern int fGnugoPipe;
extern char sGtpPath[][TMP_BUF_LEN];	// GTPエンジンの場所
//extern char sGnugoDir[];

