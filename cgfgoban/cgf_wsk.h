/* cgf_wsk.h */
//#define WM_USER_GETSERV (WM_USER+1)
//#define WM_USER_ASYNC_GETHOST (WM_USER+2)	// ホスト情報を取得したときに通知
#define WM_USER_ASYNC_SELECT  (WM_USER+3)	// ソケットイベントを通知

// nngs
extern int fUseNngs;		// nngs経由で対戦する場合
extern int fPassWindows;	// Windowsに処理を渡してる。表示以外を全てカット

//extern char sNNGS_IP[TMP_BUF_LEN];
extern char sNngsIP[][TMP_BUF_LEN];	// nngsのサーバ名
extern char nngs_player_name[2][TMP_BUF_LEN];
extern int nngs_minutes;		// 40分切れ負け
extern int fAIRyusei;

// 関数のプロトタイプ宣言
void OnNngsSelect(HWND hWnd, WPARAM wParam, LPARAM lParam);	// 非同期のSelectを受け取る。nngs用
BOOL CALLBACK NngsWaitProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);		// nngsからの情報を待つ
void close_nngs(void);	// 接続の切断

void change_z_str(char *str, int z);
void NngsSend(char *ptr);

