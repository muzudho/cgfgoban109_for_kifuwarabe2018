/*** cgf_win.cpp --- Windowsに依存する部分を主に含む --- ***/
//#pragma warning( disable : 4115 4201 4214 4057 4514 )  // 警告メッセージ 4201 を無効にする。
#include <Windows.h>
#include <MMSystem.h>	// PlaySound()を使う場合に必要。Windows.h の後に組み込む。さらに WINMM.LIB を追加リンク
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <CommCtrl.h>	// ToolBar を使用するための定義 comctl32.lib をリンクする必要あり
#if (_MSC_VER >= 1100)	// VC++5.0以降でサポート
#include <zmouse.h>		// IntelliMouseをサポートするために無理やりリンク（大丈夫？）
#endif

#include "cgf.h"
#include "cgf_rc.h"
#include "cgf_wsk.h"
#include "cgf_pipe.h"

//#include "cgfthink.h"

// 初期盤面、棋譜(消費時間)、手数、手番、盤面のサイズ、コミ
int (*cgfgui_thinking)(
	int init_board[],	// 初期盤面
	int kifu[][3],		// 棋譜  [][0]...座標、[][1]...石の色、[][2]...消費時間（秒)
	int tesuu,			// 手数
	int black_turn,		// 手番(黒番...1、白番...0)
	int board_size,		// 盤面のサイズ
	double komi,		// コミ
	int endgame_type,	// 0...通常の思考、1...終局処理、2...図形を表示、3...数値を表示。
	int endgame_board[]	// 終局処理の結果を代入する。
);

// 対局開始時に一度だけ呼ばれます。
void (*cgfgui_thinking_init)(int *ptr_stop_thinking);

// 対局終了時に一度だけ呼ばれます。
void (*cgfgui_thinking_close)(void);

int InitCgfGuiDLL(void);	// DLLを初期化する。
void FreeCgfGuiDLL(void);	// DLLを解放する。


// 現在局面で何をするか、を指定
enum EndGameType {
	ENDGAME_MOVE,			// 通常の手
	ENDGAME_STATUS,			// 終局処理
	ENDGAME_DRAW_FIGURE,	// 図形を描く
	ENDGAME_DRAW_NUMBER 	// 数値を書く
};

// 図形を描く時の指定。盤面、石の上に記号を書ける。
// (形 | 色) で指定する。黒で四角を書く場合は (FIGURE_SQUARE | FIGURE_BLACK)
enum FigureType {
	FIGURE_NONE,			// 何も書かない
	FIGURE_TRIANGLE,		// 三角形
	FIGURE_SQUARE,			// 四角
	FIGURE_CIRCLE,			// 円
	FIGURE_CROSS,			// ×
	FIGURE_QUESTION,		// "？"の記号	
	FIGURE_HORIZON,			// 横線
	FIGURE_VERTICAL,		// 縦線
	FIGURE_LINE_LEFTUP,		// 斜め、左上から右下
	FIGURE_LINE_RIGHTUP,	// 斜め、左下から右上
	FIGURE_BLACK = 0x1000,	// 黒で書く（色指定)
	FIGURE_WHITE = 0x2000,	// 白で書く	(色指定）
};

// 死活情報でセットする値
// その位置の石が「活」か「死」か。不明な場合は「活」で。
// その位置の点が「黒地」「白地」「ダメ」か。
enum GtpStatusType {
	GTP_ALIVE,				// 活
	GTP_DEAD,				// 死
	GTP_ALIVE_IN_SEKI,		// セキで活（未使用、「活」で代用して下さい）
	GTP_WHITE_TERRITORY,	// 白地
	GTP_BLACK_TERRITORY,	// 黒地
	GTP_DAME				// ダメ
};

int endgame_board[BANMAX];	// 終局処理、図形、数値表示用の盤面
int endgame_board_type;		// 現在、終局、図形、数値のどれを表示中か

/* プロトタイプ宣言 ---------------------- 以下Windows固有の宣言を含む */
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK DrawWndProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK PrintProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK AboutDlgProc( HWND, UINT, WPARAM, LPARAM );
int init_comm(HWND hWnd);	// 通信対戦の初期化処理。失敗した場合は0を返す
void GameStopMessage(HWND hWnd,int ret, int z);	// メッセージを表示して対局を停止
void StopPlayingMode(void);				// 対局中断処理。通信対戦中ならポートなどを閉じる。
void PaintBackScreen( HWND hWnd );
void GetScreenParam( HWND hWnd,int *dx, int *sx,int *sy, int *db );
int ChangeXY(LONG lParam);
void OpenPrintWindow(HWND);
void KifuSave(void);
int  KifuOpen(void);
void SaveIG1(void);					// ig1ファイルを書き出す
int count_endgame_score_chinese(void);	// 中国ルールで計算（盤上に残っている活石の数の差のみが対象）
void endgame_score_str(char *str, int flag);	// 地を数えて結果を文字列で返す
int IsCommunication(void);	// 現在、通信対戦中か
void PaintUpdate(void);		// 部分書き換え
void UpdateRecentFile(char *sAdd);		// ファイルリストを更新。一番最近開いたものを先頭へ。
void UpdateRecentFileMenu(HWND hWnd);	// メニューを変更する
void DeleteRecentFile(int n);			// ファイルリストから1つだけ削除。ファイルが消えたので。
BOOL LoadRegCgfInfo(void);		// レジストリから情報を読み込む
BOOL SaveRegCgfInfo(void);		// レジストリに  情報を書きこむ
void PRT_to_clipboard(void);	// PRTの内容をクリップボードにコピー
int IsDlgDoneMsg(MSG msg);		// WinMain()のメッセージループで条件判定に使う
void LoadRegCpuInfo(void);		// レジストリからCPUの情報を読み込む
void SGF_clipout(void);			// SGFの棋譜をクリップへ
void MenuColorCheck(HWND hWnd);	// 盤面色のメニューにチェックを
void MenuEnablePlayMode(int);	// メニューを対局中かどうかで淡色表示に
void UpdateLifeDethButton(int check_on);	// 死活表示中は全てのメニューを禁止する
BOOL LoadMainWindowPlacement(HWND hWnd,int flag);	// レジストリから情報（画面サイズと位置）を読み込む
BOOL SaveMainWindowPlacement(HWND hWnd,int flag);	// レジストリ  に情報（画面サイズと位置）を書き込む
int IsGnugoBoard(void);	// Gnugoと対局中か？
void UpdateStrList(char *sAdd, int *pListNum, const int nListMax, char sList[][TMP_BUF_LEN] );	// 文字列配列の候補リストを更新。一番最近選んだものを先頭へ。
int SendUpdateSetKifu(HWND hWnd, int z, int gui_move );		// 着手を通信で送ったり、次の手番に移ったり

BOOL CALLBACK CommProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);	// 受信待ちをするダイアログボックス関数
BOOL CALLBACK StartProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GnuGoSettingProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// cgf_term.cpp
BOOL NEAR OpenConnection( void );
BOOL NEAR CloseConnection( void );
void ReturnOK(void);
int SendQuery(int qqq);
int SendNewGame(void);
void SendMove(int z);


BOOL CreateTBar(HWND hWnd);
LRESULT SetTooltipText(HWND hWnd, LPARAM lParam);
void MoveDrawWindow(HWND hWnd);
//////////////////

void PRT_Scroll(void);
void clipboard_to_board(void);	// クリップボードから盤面を読み込む


HWND ghWindow;					/* WINDOWSのハンドル  */
HINSTANCE ghInstance;			/* インスタンス       */
HWND hPrint = NULL;				/* 情報出力用のモードレスダイアログ */
extern HWND hWndToolBar;		// TOOLBAR用のハンドル
HWND hWndDraw;
HWND hWndMain;
HACCEL hAccel;					// アクセラレータキーのテープルのハンドル
HWND hWndToolBar;				// ToolBar

#define PMaxY 400				// 情報画面の縦サイズ（全長）
#define PHeight 42				// 画面の標準高さ(32が通常) 
#define PRetX 100				// 100行で改行
#define PMaxX 256				// 1行の最大文字数(意味なし？）

char cPrintBuff[PMaxY][PMaxX];	// バッファ			
static int nHeight;
static int nWidth;
static int PRT_y;
static int PRT_x = 0;

int ID_NowBanColor = ID_BAN_1;	// デフォルトの盤の色
int fFileWrite = 0;				// ファイルに書き込むフラグ
int UseDefaultScreenSize;		// 固定の画面サイズを使う
char cDragFile[MAX_PATH];		// ドラッグされた開くべきファイル名 ---> 今はダブルクリックで開くときに
char sCgfDir[MAX_PATH];			// cgfgoabn.exe があるディレクトリ
int SoundDrop = 0;				// 着手音なし
int fInfoWindow = 0;			// 情報表示あり
int fPassWindows;				// Windowsに処理を渡してる。表示以外を全てカット
int fChineseRule = 0;			// 中国ルールの場合
int thinking_utikiri_flag;
int turn_player[2] = { 0,1 };	// 対局者の種類が入る。基本は 黒:人間, 白:Computer
char sDllName[] = "CPU";		// 標準の名前です。適当に編集して下さい。
//char sGnugoDir[MAX_PATH];		// gnugo.exeがあるフルパス
int fGnugoLifeDeathView=0;		// gnugoの判定で死活表示をする
int fATView = 0;				// 座標表示をA～Tに。チェス形式（A-TでIがない）
int fAIRyusei = 1;				// AI竜星nngs
int fShowConsole = 0;

enum TurnPlayerType {
	PLAYER_HUMAN,
	PLAYER_DLL,
	PLAYER_NNGS,
	PLAYER_SGMP,
	PLAYER_GNUGO,
};

/*
2006/03/01 1.02 Visual Studio 2005 Express Edtionでもコンパイルできるように。
2007/01/22      名前やGTPエンジンPATH,nngsサーバ名を複数記憶するように。
2007/09/19 1.03 MoGoが動くように。stderrを無視するように。version名が長すぎる場合は削除
2008/01/31 1.04 UEC杯のnngsに対応。GTPエンジンが存在チェックを無視。Program Filesのように空白をはさむと認識できなかった。path欄を大きく。
2009/06/25 1.05 nngsの切れ負けの時間(分)を設定可能に。
2011/11/16 1.06 Win64でのコンパイルエラーを解消。警告はたくさん出ます・・・。
2013/11/14 1.07 コミを7.0を指定可能に。
2015/03/16 1.08 nngsでhumanが投了を送れるように。AT座標表示
2018/07/12 1.09 AI竜星のnngsに対応。起動時にConsoleを非表示可能に。
*/
char sVersion[32] = "CgfGoBan 1.09";	// CgfGobanのバージョン
char sVerDate[32] = "2018/07/15";		// 日付

int dll_col     = 2;	// DLLの色。通信対戦時に利用。
int fNowPlaying = 0;	// 現在対局中かどうか。

int total_time[2];		// 思考時間。先手[0]、後手[1]の累計思考時間。

int RetZ;				// 通信で受け取る手
int RetComm = 0;		// 通信状態を示す。0...Err, 1...手, 2...OK, 3...NewGame, 4...Answer

int nCommNumber = 1;	// COMポートの番号。

int nHandicap = 0;		// 置石はなし
int fAutoSave = 0;		// 棋譜の自動セーブ（通信対戦時）
int RealMem_MB;			// 実メモリ
int LatestScore = 0;	// 最後に計算した地
int fGtpClient = 0;		// GTPの傀儡として動く場合

static clock_t gct1;	// 時間計測用

#define RECENT_FILE_MAX 20		// 最近開いたファイルの最大数。
char sRecentFile[RECENT_FILE_MAX][TMP_BUF_LEN];	// ファイルの絶対パス
int nRecentFile = 0;			// ファイルの数
#define IDM_RECENT_FILE 2000	// メッセージの先頭
#define RECENT_POS 12			// 上から12番目に挿入

char sYourCPU[TMP_BUF_LEN];		// このマシンのCPUとFSB

#define RGB_BOARD_MAX 8	// 盤面色の数

COLORREF rgbBoard[RGB_BOARD_MAX] = {	// 盤面の色
	RGB(231,134, 49),	// 基本(こげ茶)
	RGB(255,128,  0),	// 淡
	RGB(198,105, 57),	// 濃い木目色
	RGB(192, 64,  9),	// 濃
	RGB(255,188,121),	// ゴム板（彩の色）
	RGB(  0,255,  0),	// 緑
	RGB(255,255,  0),	// 黄
	RGB(255,255,255),	// 白
};

COLORREF rgbBack  = RGB(  0,  0,  0);	// 黒、背景の色
//COLORREF rgbBack  = RGB(231,255,231);	// 背景の色
//COLORREF rgbBack  = RGB(247,215,181);	// 背景の色
//COLORREF rgbBack  = RGB(  0,128,  0);	// 濃緑
//COLORREF rgbBack  = RGB(  5, 75, 49);	// 濃緑(彩の背景)

COLORREF rgbText  = RGB(255,255,255);	// 位置、名前の色

COLORREF rgbBlack = RGB(  0,  0,  0);
COLORREF rgbWhite = RGB(255,255,255);

#define NAME_LIST_MAX 30
char sNameList[NAME_LIST_MAX][TMP_BUF_LEN];	// 対局者の名前の候補
int nNameList;								// 名前候補の数
//int nNameListSelect[2];					// 現在の対局者の番号

#define GTP_PATH_MAX 30
char sGtpPath[GTP_PATH_MAX][TMP_BUF_LEN];	// GTPエンジンの場所
int nGtpPath;								// 名前候補の数

#define NNGS_IP_MAX 30
char sNngsIP[NNGS_IP_MAX][TMP_BUF_LEN];	// nngsのサーバ名
int nNngsIP;							// 候補の数

#define GUI_DLL_MOVE  0
#define GUI_USER_PASS 1
#define GUI_GTP_MOVE  2

// SGF にあるデータ
char sPlayerName[2][TMP_BUF_LEN];	// 対局者の名前。[0] 黒、[1] 白。
char sDate[TMP_BUF_LEN];			// 日付
char sPlace[TMP_BUF_LEN];			// 場所
char sEvent[TMP_BUF_LEN];			// 大会名
char sRound[TMP_BUF_LEN];			// 何回戦か
char sKomi[TMP_BUF_LEN];			// こみ
char sTime[TMP_BUF_LEN];			// 時間
char sGameName[TMP_BUF_LEN];		// 対局名
char sResult[TMP_BUF_LEN];			// 結果

#define CGF_ICON "CGF1"


#ifdef CGF_E
char szTitleName[] = "CgfGoBan";
char szInfoWinName[] = "Information Window";
#else
char szTitleName[] = "CGF碁盤";
char szInfoWinName[] = "情報表示";
#endif

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow )
{
	MSG      msg;
	WNDCLASS wc;
	static char szAppName[] = "CGFGUI";
	static HBRUSH hBrush = NULL;

	ghInstance = hInstance;

#ifdef CGF_E 
	SetThreadLocale(LANG_ENGLISH);	// 無理やり場所を英語に。リソースが英語に変わる。テスト用。
#endif

	if ( !hPrevInstance ) {
		wc.lpszClassName = szAppName;
		wc.hInstance     = hInstance;
		wc.lpfnWndProc   = WndProc;
		wc.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wc.hIcon         = LoadIcon( hInstance, CGF_ICON );
		wc.lpszMenuName  = szAppName;
//		hBrush = CreateSolidBrush( RGB( 0,128,0 ) );	// 背景を緑に(wc.中のオブジェクトは終了時に自動的に削除される）
//		wc.hbrBackground = hBrush;//	GetStockObject(WHITE_BRUSH);
	    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH); 
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		if (!RegisterClass( &wc )) return FALSE;		/* クラスの登録 */


	    wc.style         = CS_HREDRAW | CS_VREDRAW;
	    wc.lpfnWndProc   = DrawWndProc;
	    wc.cbClsExtra    = 0;
	    wc.cbWndExtra    = 4;
	    wc.hInstance     = hInstance;
	    wc.hIcon         = NULL;
	    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		hBrush = CreateSolidBrush( rgbBack );	// 背景色(wc.中のオブジェクトは終了時に自動的に削除される）
		wc.hbrBackground = hBrush;//	GetStockObject(WHITE_BRUSH);
//		wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	    wc.lpszMenuName  = NULL;
	    wc.lpszClassName = "DrawWndClass";
	    if (! RegisterClass (&wc)) return FALSE;
	}
	else {
//		MessageBox(NULL,"２つ起動しようとしています\n終了させて下さい","注意",IDOK);
//		return FALSE;
	}

	// この行をCreateWindowの次に置くとだめ！。WM_CREATEを発行してしまうから！
	if ( lpszCmdLine != NULL ) {
		if ( strstr(lpszCmdLine,"--mode gtp") ) fGtpClient = 1;	// GTPの傀儡として動く
		else strcpy(cDragFile,lpszCmdLine);	// ダブルクリックされた開くべきファイル名
	}
	/********** ウインドウの作成 ********/
	hWndMain = CreateWindow(
						szAppName,						/* Class name.   */                                  
//						lpszCmdLine,        			/* 起動時 Pram.  */
						szTitleName,					/* Title         */		       					
						WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,	/* Style bits.   */
						  0, 20,						/* X,Y - default.*/
						640,580,						/* CX, CY        */
						NULL,							/* 親はなし      */
						NULL,							/* Class Menu.   */
						hInstance,						/* Creator       */
						NULL							/* Params.       */
					   );

	ghWindow = hWndMain;

	ShowWindow( hWndMain, cmdShow );
	UpdateWindow(hWndMain);

	hAccel = LoadAccelerators(hInstance, "CGF_ACCEL");	// アクセラレータキーのロード

	while( GetMessage( &msg, 0, 0, 0) ) {
		if ( IsDlgDoneMsg(msg) ) {	// アクセラレータキーやダイアログの処理
			TranslateMessage( &msg );					// keyboard input.
			DispatchMessage( &msg );
        }
	}
	return msg.wParam;
}


/************** ウィンドウ手続き ****************/						
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	HMENU hMenu;
	int i,k,z,ret;
	MEMORYSTATUS MemStat;	// 物理メモリ測定用

	if ( fPassWindows ) for (;;) {	// 思考中はすべてカットする。
		if ( msg == WM_CLOSE ) { PRT("\n\n現在思考中...別のボタンを。\n\n"); return 0; }
		if ( msg == WM_COMMAND && (LOWORD(wParam) == IDM_DLL_BREAK || LOWORD(wParam) == IDM_BREAK) ) {
			PRT("\n\n現在思考中...無理やり停止。\n\n");
			thinking_utikiri_flag = 1;
			return 0;
		}
		if ( msg == WM_USER_ASYNC_SELECT ) break;	// 通信情報は受け取る
		if ( msg == WM_COMMAND && LOWORD(wParam) == IDM_KIFU_SAVE ) break;
		return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}

	switch( msg ) {
		case WM_CREATE:											// 初期設定
#ifdef CGF_DEV
			UseDefaultScreenSize = 1;	// 固定の画面サイズを使う(公開時OFF)
#endif
            // 描画用Windowの生成
			hWndDraw = CreateWindowEx(WS_EX_CLIENTEDGE,
				"DrawWndClass", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
				hWnd, NULL, ghInstance, NULL);
			if ( !hWndDraw ) return FALSE;

            CreateTBar(hWnd);				// ツールバーを生成
//		    DragAcceptFiles(hWnd, TRUE);	// ドラッグ＆ドロップされるファイルを受け付ける。

			init_ban();

			// レジストリから情報（画面サイズと位置）を読み込む
			if ( LoadMainWindowPlacement(hWnd,1) == 0 ) PRT("Registory Error (If first time, it's OK)\n");
			// レジストリから情報を読み込む
			if ( LoadRegCgfInfo() == 0 ) PRT("Registory Error... something new (If first time, it's OK)\n");
			UpdateRecentFileMenu(hWnd);	// メニューを変更する

			if ( fInfoWindow ) SendMessage(hWnd,WM_COMMAND,IDM_PRINT,0L);		// 情報表示Windowの表示
			else CheckMenuItem( GetMenu(hWnd), IDM_PRINT, MF_UNCHECKED);
			MenuColorCheck(hWnd);	// 盤面色のメニューにチェックを

			GlobalMemoryStatus(&MemStat);
			RealMem_MB = MemStat.dwTotalPhys/(1024*1024);	// 積んでいる実メモリ
			LoadRegCpuInfo();	// レジストリからCPUの情報を読み込む
			PRT("物理メモリ= %d MB, ",MemStat.dwTotalPhys/(1024*1024));
			PRT("空物理=%dMB, 割合=%d%%\n",MemStat.dwAvailPhys/(1024*1024), MemStat.dwMemoryLoad);

			if ( SoundDrop ) { SoundDrop = 0; SendMessage(hWnd,WM_COMMAND,IDM_SOUND,0L); } // 効果音をオン
			if ( fATView   ) { fATView   = 0; SendMessage(hWnd,WM_COMMAND,IDM_ATVIEW,0L); }
			if ( fShowConsole ) { fShowConsole = 0; SendMessage(hWnd,WM_COMMAND,IDM_SHOW_CONSOLE,0L); }

			if ( UseDefaultScreenSize == 0 ) {
				SendMessage(hWnd,WM_COMMAND,IDM_BAN_CLEAR,0L);	// 画面消去(後、上のレジストリ登録をONに!!!）
			}

			sprintf( sPlayerName[0],"You");
			sprintf( sPlayerName[1],sDllName);

//			PRT("--------->%s\n",GetCommandLine());
			GetCurrentDirectory(MAX_PATH, sCgfDir);	// 現在のディレクトリを取得

			SendMessage(hWnd,WM_COMMAND,IDM_BAN_CLEAR,0L);	// 画面消去

			SetFocus(hWnd);	// 先にhPrintが書かれるとFocusがそっちに行ってしまうので。

			if ( strlen(cDragFile) ) PostMessage(hWnd,WM_COMMAND,IDM_KIFU_OPEN,0L);	// 棋譜を開く
//			SendMessage(hWnd,WM_COMMAND,IDM_THINKING,0L); DestroyWindow(hWnd);	// Profile用のテスト

			// DLLを読み込む
			ret = InitCgfGuiDLL();
			if ( ret != 0 ) {
				char sTmp[TMP_BUF_LEN];
				if ( ret == 1 ) sprintf(sTmp,"DLL is not found.");
				else            sprintf(sTmp,"Fail GetProcAddress() ... %d",ret);
				MessageBox(NULL,sTmp,"DLL initialize Error",MB_OK); 
				PRT("not able to use DLL.\n");
				return FALSE;
			} else {
				// DLLを初期化する。
				cgfgui_thinking_init(&thinking_utikiri_flag);
				HWND Stealth = FindWindowA("ConsoleWindowClass", NULL);
				if ( Stealth && fShowConsole == 0 ) {
					ShowWindow(Stealth, SW_HIDE);
// 					HWND hWnd = GetConsoleWindow();
				}
			}

			SendMessage(hWnd,WM_COMMAND,IDM_BREAK,0L);	// 対局中断状態に
/*
			PRT("fGtpClient = %d\n",fGtpClient);
			if ( fGtpClient ) {
				ShowWindow( hWnd, SW_HIDE );
				if ( hPrint && IsWindow(hPrint)==TRUE ) ShowWindow( hPrint, SW_HIDE );
				{
					extern void test_gtp_client(void);
					test_gtp_client();
				}
				DestroyWindow(hWnd);	// GTP経由の場合はすぐに終了する。
//				PRT_to_clipboard();		// PRTの内容をクリップボードにコピー
			}
*/
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ) {
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
			
				case IDM_TEST:
					gct1 = clock();	// 探索開始時間。1000分の１秒単位で測定。
//					{ void init_gnugo_pipe(int); init_gnugo_pipe(1); }	// GnuGoとGTP経由で連続対戦
					PRT("%2.1f 秒\n", GetSpendTime(gct1));
					break;

				case IDM_THINKING:			// 思考ルーチン呼び出し
					z = ThinkingDLL(ENDGAME_MOVE);	// 着手位置を返す
					ret = SendUpdateSetKifu(hWnd, z, GUI_DLL_MOVE);		// 着手を通信で送ったり、次の手番に移ったり
					break;
				
				case IDM_PASS:
					if ( fNowPlaying == 0 ) return 0;	 
					ret = SendUpdateSetKifu(hWnd, 0, GUI_USER_PASS);	// 着手を通信で送ったり、次の手番に移ったり
					break;

				case IDM_RESIGN:
					if ( fNowPlaying == 0 ) return 0;	 
					ret = SendUpdateSetKifu(hWnd, -1, GUI_USER_PASS);
					break;

				case IDM_GNUGO_PLAY:	// GnuGo(GTP Engine)に手を打たせる
					z = PlayGnugo(ENDGAME_MOVE);	// 着手位置を返す
					ret = SendUpdateSetKifu(hWnd, z, GUI_GTP_MOVE);		// 着手を通信で送ったり、次の手番に移ったり
					break;

				case IDM_INIT_COMM:		// 通信対戦の初期化処理
					if ( init_comm(hWnd) ) PostMessage(hWnd,WM_COMMAND,IDM_NEXT_TURN,0L);
					else StopPlayingMode();
					break;

				case IDM_NEXT_TURN:		// 次の手はどうすべきかを処理
					// COM同士の対戦中に停止するため(GnuGo同士でも、COM－GnuGoも)
					for (i=0;i<2;i++) {
						k = turn_player[i];
						if ( k == PLAYER_DLL || k == PLAYER_GNUGO ) continue;
						else break;
					}
					if ( i==2 ) {
						for (i=0;i<100;i++) PassWindowsSystem();
						if ( thinking_utikiri_flag ) {
							thinking_utikiri_flag = 0;
							StopPlayingMode();
							break;
						}
					}

					{
						gct1 = clock();	// 思考開始時間
						// 連続パスなら終局処理を呼んで中断
//						if ( kifu[tesuu-1][0]==0 && UseDefaultScreenSize == 0 ) SendMessage(hWnd,WM_COMMAND,IDM_DEADSTONE,0L);	// 終局処理を呼ぶ
						int next = turn_player[GetTurnCol()-1];
//						if ( next == PLAYER_HUMAN ) ;	// 何もしない
						if ( next == PLAYER_DLL   ) PostMessage(hWnd,WM_COMMAND,IDM_THINKING,0L);
						if ( next == PLAYER_NNGS  ) PostMessage(hWnd,WM_COMMAND,IDM_NNGS_WAIT,0L);
						if ( next == PLAYER_SGMP  ) PostMessage(hWnd,WM_COMMAND,IDM_SGMP_WAIT,0L);
						if ( next == PLAYER_GNUGO ) PostMessage(hWnd,WM_COMMAND,IDM_GNUGO_PLAY,0L);
					}
					break;

				case IDM_NNGS_WAIT:		// 通信相手の手を待つ
				case IDM_SGMP_WAIT:
				{
					int fBreak = 1;
					for (;;) {
						gct1 = clock();
						if ( fUseNngs ) {
							if ( DialogBoxParam(ghInstance, "NNGS_WAIT", hWnd, (DLGPROC)NngsWaitProc, 1) == FALSE ) {
								PRT("メインループに戻ります\n");
								break;
							}
							if ( RetZ < 0 ) break;	// nngsの場合は 0...PASS, 1以上...場所, マイナス...エラーを示す
							RetComm = 1;
						} else if ( DialogBox(ghInstance, "COMM_DIALOG", hWnd, (DLGPROC)CommProc) == FALSE ) {
							PRT("メインループに戻ります\n");
							break;
						}
						if ( RetComm == 2 ) continue;	// OKを受信して抜けた時はもう一度待つ
						if ( RetComm != 1 ) {	// move 以外は一度待つ
							if ( RetComm == 3 ) {
								PRT("NewGame がまた来ている！でもOKを返してあげよう\n");
								ReturnOK();	// 条件を満たしたのでOKを送り、手を待つ
							}
							continue;
						}
						PRT("RetZ = %x\n",RetZ);
						if ( IsGnugoBoard()    ) UpdateGnugoBoard(RetZ);

						int ret = set_kifu_time(RetZ,GetTurnCol(),(int)GetSpendTime(gct1));
						if ( ret ) {
							GameStopMessage(hWnd,ret,RetZ);
							break;
						}
						// 次の手番の処理に移る
						PostMessage(hWnd,WM_COMMAND,IDM_NEXT_TURN,0L);
						fBreak = 0;
						break;
					}
					if ( fBreak ) StopPlayingMode();
					break;
				}

				case IDM_PLAY_START:
					{
						int fRet = IDYES;
						if ( all_tesuu != 0 ) {
#ifdef CGF_E
							char sTmp[] = "Initialize Board?";
#else
							char sTmp[] = "局面を初期化しますか？";
#endif
							fRet = MessageBox(hWnd,sTmp,"Start Game",MB_YESNOCANCEL | MB_DEFBUTTON2);
							if ( fRet == IDCANCEL ) break;
						}
						DialogBoxParam(ghInstance, "START_DIALOG", hWnd, (DLGPROC)StartProc, fRet);
					}
					PaintUpdate();		// 先手後手の名前を更新するため全画面書き換え。
					break;

				case IDM_GNUGO_SETTING:
					DialogBox(ghInstance, "GTP_SETTING", hWnd, (DLGPROC)GnuGoSettingProc);
					break;

				case IDM_CLIPBOARD:
					hyouji_clipboard();
					break;

				case IDM_FROM_CLIPBOARD:
					clipboard_to_board();	// クリップボードから盤面を読み込む
					break;

				case IDM_PRT_TO_CLIP:
					PRT_to_clipboard();		// PRTの内容をクリップボードにコピー
					break;

				case IDM_SGF_CLIPOUT:
					SGF_clipout();			// SGFで棋譜をクリップボードへ
					break;

				case IDM_DEADSTONE:			// ツールバーのボタンが押された場合
				case IDM_VIEW_LIFEDEATH:	// 終局処理、死に石を表示
				case IDM_VIEW_FIGURE:		// 図形を表示
				case IDM_VIEW_NUMBER:		// 数値を表示
					{
						int id = LOWORD(wParam);
						int ids[3] = {IDM_VIEW_LIFEDEATH, IDM_VIEW_FIGURE, IDM_VIEW_NUMBER };  

						hMenu = GetMenu(hWnd);
						for (i=0;i<3;i++) CheckMenuItem(hMenu, ids[i], MF_UNCHECKED);	// メニューを全部OFF
						k = 0;
						if ( id==IDM_DEADSTONE ) {
							if ( endgame_board_type==0 ) id = ids[0];
							else k = endgame_board_type;
						}
						if ( id==IDM_VIEW_LIFEDEATH ) k = ENDGAME_STATUS;
						if ( id==IDM_VIEW_FIGURE    ) k = ENDGAME_DRAW_FIGURE;
						if ( id==IDM_VIEW_NUMBER    ) k = ENDGAME_DRAW_NUMBER;
						if ( k == endgame_board_type ) {	// 同じものが再度選択
							endgame_board_type = 0;
							UpdateLifeDethButton(0);
						} else {
							endgame_board_type = k;
							UpdateLifeDethButton(1);
							CheckMenuItem(hMenu, id, MF_CHECKED);
							if ( endgame_board_type == ENDGAME_STATUS && fGnugoLifeDeathView ) {
								FinalStatusGTP(endgame_board);
							} else {
								ThinkingDLL(endgame_board_type);
							}
						}
						PaintUpdate();
					}
					break;

				case IDM_PRINT:
					hMenu = GetMenu(hWnd);
					if ( hPrint==NULL || IsWindow(hPrint)==FALSE ) {
						CheckMenuItem(hMenu, IDM_PRINT, MF_CHECKED);
						OpenPrintWindow(hWnd);
					} else {
						DestroyWindow(hPrint);
						CheckMenuItem(hMenu, IDM_PRINT, MF_UNCHECKED);
					}
					fInfoWindow = (hPrint != NULL);
					break;

				case IDM_SOUND:
					SoundDrop = (SoundDrop == 0);	// 着手音フラグ
					CheckMenuItem(GetMenu(hWnd), IDM_SOUND, SoundDrop ? MF_CHECKED : MF_UNCHECKED);
					break;

				case ID_BAN_1:
				case ID_BAN_2:
				case ID_BAN_3:
				case ID_BAN_4:
				case ID_BAN_5:
				case ID_BAN_6:
				case ID_BAN_7:
				case ID_BAN_8:
					ID_NowBanColor = LOWORD(wParam);
					MenuColorCheck(hWnd);	// 盤面色のメニューにチェックを
					PaintUpdate();
					break;

				case IDM_BREAK:	// 対局中断
					fNowPlaying = 0;
					MenuEnablePlayMode(0);	// 対局中は棋譜移動メニューなどを禁止する
					break;

				case IDM_CHINESE_RULE:
					fChineseRule = (fChineseRule == 0);
					CheckMenuItem(GetMenu(hWnd), IDM_CHINESE_RULE, fChineseRule ? MF_CHECKED : MF_UNCHECKED);
					PaintUpdate();
					break;

				case IDM_ATVIEW:
					fATView = (fATView == 0);
					CheckMenuItem(GetMenu(hWnd), IDM_ATVIEW, fATView ? MF_CHECKED : MF_UNCHECKED);
					PaintUpdate();
					break;

				case IDM_SHOW_CONSOLE:
					fShowConsole = (fShowConsole == 0);
					CheckMenuItem(GetMenu(hWnd), IDM_SHOW_CONSOLE, fShowConsole ? MF_CHECKED : MF_UNCHECKED);
					break;

				case IDM_BAN_CLEAR:
					for (i=0;i<BANMAX;i++) {
						if ( ban[i] != 3 ) ban[i]=0;
						ban_start[i] = ban[i];
					}
					reconstruct_ban();
					PaintUpdate();
					break;

				case IDM_SET_BOARD:
					all_tesuu = 0;
					reconstruct_ban();
					PaintUpdate();
					break;

				case IDM_BACK1:
					move_tesuu(tesuu - 1);		// 任意の手数の局面に盤面を移動させる
					PaintUpdate();
					break;

				case IDM_BACK10:
					move_tesuu(tesuu - 10);		// 任意の手数の局面に盤面を移動させる
					PaintUpdate();
					break;

				case IDM_BACKFIRST:
					move_tesuu( 0 );			// 任意の手数の局面に盤面を移動させる
					PaintUpdate();
					break;

				case IDM_FORTH1:
					move_tesuu(tesuu + 1);		// 任意の手数の局面に盤面を移動させる
					PaintUpdate();
					break;

				case IDM_FORTH10:
					move_tesuu(tesuu + 10);		// 任意の手数の局面に盤面を移動させる
					PaintUpdate();
					break;

				case IDM_FORTHEND:
					move_tesuu(all_tesuu);		// 任意の手数の局面に盤面を移動させる
					PaintUpdate();
					break;

				case IDM_KIFU_SAVE:
					KifuSave();
					break;

				case IDM_KIFU_OPEN:
					KifuOpen();
					break;

				case IDM_FILE_WRITE:
					fFileWrite = (fFileWrite == 0);
					hMenu = GetMenu(hWnd);
					CheckMenuItem(hMenu, IDM_FILE_WRITE, (fFileWrite ? MF_CHECKED : MF_UNCHECKED) );
					break;

				case IDM_ABOUT:
					DialogBox( ghInstance, "ABOUT_DIALOG", hWnd, (DLGPROC)AboutDlgProc);
					break;
			}

			// RecentFileが選択された場合
			if ( IDM_RECENT_FILE <= wParam && wParam < IDM_RECENT_FILE+RECENT_FILE_MAX ) {
				int n =  wParam - IDM_RECENT_FILE;
				strcpy(cDragFile, sRecentFile[n]);
//				PRT("%s\n",InputDir);
				if ( KifuOpen() == FALSE ) {
					DeleteRecentFile(n);	// ファイルリストから削除。ファイルが消えたので。
					UpdateRecentFileMenu(hWnd);
				}
				cDragFile[0] = 0;
			}
			return 0;

		case WM_KEYDOWN:
			if ( wParam == VK_RIGHT ) PostMessage(hWnd, WM_COMMAND, IDM_FORTH1,  0L);
			if ( wParam == VK_LEFT  ) PostMessage(hWnd, WM_COMMAND, IDM_BACK1,   0L);
			if ( wParam == VK_UP    ) PostMessage(hWnd, WM_COMMAND, IDM_BACK10,  0L);
			if ( wParam == VK_DOWN  ) PostMessage(hWnd, WM_COMMAND, IDM_FORTH10, 0L);
   			if ( wParam == VK_PRIOR ) PostMessage(hWnd, WM_COMMAND, IDM_BACK10,  0L);
			if ( wParam == VK_NEXT  ) PostMessage(hWnd, WM_COMMAND, IDM_FORTH10, 0L);
			if ( wParam == VK_HOME  ) PostMessage(hWnd, WM_COMMAND, IDM_BACKFIRST, 0L);
			if ( wParam == VK_END   ) PostMessage(hWnd, WM_COMMAND, IDM_FORTHEND, 0L);

			if ( wParam == 'E' ) PostMessage(hWnd, WM_COMMAND, IDM_VIEW_LIFEDEATH, 0L);
			if ( wParam == 'F' ) PostMessage(hWnd, WM_COMMAND, IDM_VIEW_FIGURE, 0L);
			if ( wParam == 'N' ) PostMessage(hWnd, WM_COMMAND, IDM_VIEW_NUMBER, 0L);
			if ( wParam == 'P' ) PostMessage(hWnd, WM_COMMAND, IDM_PASS, 0L);


			if ( wParam == '1' ) PostMessage(hWnd, WM_COMMAND, IDM_VIEW_LIFEDEATH, 0L);
			if ( wParam == '2' ) PostMessage(hWnd, WM_COMMAND, IDM_VIEW_FIGURE, 0L);
			if ( wParam == '3' ) PostMessage(hWnd, WM_COMMAND, IDM_VIEW_NUMBER, 0L);
			break;

		case WM_USER_ASYNC_SELECT:	// 非同期のソケット通知(nngsサーバとの通信用)
			OnNngsSelect(hWnd,wParam,lParam);
			break;

        case WM_NOTIFY:
            SetTooltipText(hWnd, lParam);
            break;

        case WM_SIZE:
            SendMessage(hWndToolBar,msg,wParam,lParam);
            if (wParam != SIZE_MINIMIZED) MoveDrawWindow(hWnd);
	    break;

		case WM_DESTROY:
			if ( SaveMainWindowPlacement(hWnd  ,1) == 0 ) PRT("レジストリ１のエラー（書き込み）\n");
			SaveRegCgfInfo();	// レジストリに登録
			FreeCgfGuiDLL();	// DLLを解放する。
			PostQuitMessage(0);
			break;
		
		default:
			return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}
	return 0L;
}

// 通信対戦の初期化処理。失敗した場合は0を返す
int init_comm(HWND hWnd)
{
	if ( fUseNngs ) {
		// 最初にWinSockを初期化してサーバに接続まで。
		if ( DialogBoxParam(ghInstance, "NNGS_WAIT", hWnd, (DLGPROC)NngsWaitProc, 0) == FALSE ) {
			PRT("nngs接続失敗\n");
			return 0;
		}
		PRT("Loginまで正常終了\n");
		return 1;
	}

	if ( OpenConnection() == FALSE ) {
		PRT("通信ポートオープンの失敗！\n");
		return 0;
	}
	PRT("棋譜自動セーブ=%d\n",fAutoSave);
	if ( turn_player[1]==PLAYER_SGMP ) {	// こちらが先手（黒）の時は NEWGAME を送る
		if ( SendNewGame() == FALSE ) {	// NewGame を送る
			PRT("メインループに戻ります\n");
			CloseConnection();
			return 0;
		}
		// 先手で打つ
		return 1;
	} else {		// DLLが後手番の時
		if ( DialogBox(ghInstance, "COMM_DIALOG", hWnd, (DLGPROC)CommProc) == FALSE ) {
			PRT("メインループに戻ります\n");
			CloseConnection();
			return 0;
		}
		if ( RetComm != 3 ) {
			PRT("NewGame が来ないね・・・RetComm=%d\n",RetComm);
			CloseConnection();
			return 0;
		}
//		PRT("NewGame が来た来た！\n");

		int k = SendQuery(11);	// 石の色を質問
		if ( k == 1 ) {
			PRT("先手なのに白なの？\n");
			CloseConnection();
			return 0;
		}
		k = SendQuery(8);	// 互い先かどうかを質問
		if ( k >= 2 ) {
			PRT("置き碁はしませんよ k=%d だけど続行\n",k);
//			CloseConnection();
//			return 0;
		}
		ReturnOK();	// 条件を満たしたのでOKを送り、指し手を待つ
		return 1;
	}
//	return 0;
}

// 現在、通信対戦中か
int IsCommunication(void)
{
	int i,n;

	for (i=0;i<2;i++) {
		n = turn_player[i];
		if ( n == PLAYER_NNGS || n == PLAYER_SGMP ) return 1;
	}
	return 0;
}

// 対局中断処理。通信対戦中ならポートなどを閉じる。
void StopPlayingMode(void)
{
	if ( IsCommunication() ) CloseConnection();
	SendMessage(ghWindow,WM_COMMAND, IDM_BREAK, 0L);
	PaintUpdate();
}

// 着手を通信で送ったり、次の手番に移ったり
int SendUpdateSetKifu(HWND hWnd, int z, int gui_move )
{
	if ( IsCommunication() ) SendMove(z);
	if ( gui_move != GUI_GTP_MOVE && IsGnugoBoard() ) UpdateGnugoBoard(z);

	int ret = set_kifu_time(z,GetTurnCol(),(int)GetSpendTime(gct1));
	if ( ret ) {
		GameStopMessage(hWnd,ret,z);
		StopPlayingMode();
		return ret;
	}

	if ( gui_move == GUI_USER_PASS ) PaintUpdate();
	else {
		if ( z == 0 && turn_player[GetTurnCol()-1] == PLAYER_HUMAN ) {
			MessageBox(hWnd,"Computer PASS","Pass",MB_OK);
		}
		if ( SoundDrop ) sndPlaySound("stone.wav", SND_ASYNC | SND_NOSTOP);	// MMSYSTEM.H をインクルードする必要あり。またWinMM.libをリンク
	}

	// 次の手番の処理に移る
	PostMessage(hWnd,WM_COMMAND,IDM_NEXT_TURN,0L);
	return 0;
}

/*
// 置石は2子～9子までは下のように。

＋＋●	// 2子  
＋＋＋
●＋＋

＋＋●	// 3子
＋＋＋
●＋●

●＋●	// 5子
＋●＋
●＋●

●＋●	// 6子
●＋●
●＋●

●＋●	// 7子
●●●
●＋●

●●●	// 8子
●＋●
●●●

＋＋＋＋●	// 10子
＋●●●＋
＋●●●＋
＋●●●＋
＋＋＋＋＋

＋＋＋＋●	// 12子
＋●●●＋
＋●●●＋
＋●●●＋
●＋＋＋●
*/
// 置石を設定
void SetHandicapStones(void)
{
	int i,k;
	
	SendMessage(ghWindow,WM_COMMAND,IDM_BAN_CLEAR,0L);	// 盤面初期化
	k = nHandicap;
	if ( ban_size == BAN_19 ) {
		ban[0x0410] = 1;		// 対角線には必ず置く
		ban[0x1004] = 1;	
		if ( k >= 4 ) {
			ban[0x0404] = 1;	// 4子以上は星は全部
			ban[0x1010] = 1;	
		}
		if ( k >= 6 ) {
			ban[0x0a04] = 1;	// 6子以上は左右中央の星
			ban[0x0a10] = 1;	
		}
		if ( k >= 8 ) {
			ban[0x040a] = 1;	// 8子以上は上下中央の星
			ban[0x100a] = 1;
		}
		if ( k == 3 ) ban[0x1010] = 1;
		if ( k == 5 || k == 7 || k >= 9 ) ban[0x0a0a] = 1;
	} else if ( ban_size == 13 ) {
		ban[0x040a] = 1;		// 対角線には必ず置く
		ban[0x0a04] = 1;	
		if ( k >= 4 ) {
			ban[0x0404] = 1;	// 4子以上は星は全部
			ban[0x0a0a] = 1;	
		}
		if ( k >= 6 ) {
			ban[0x0704] = 1;	// 6子以上は左右中央の星
			ban[0x070a] = 1;	
		}
		if ( k >= 8 ) {
			ban[0x0407] = 1;	// 8子以上は上下中央の星
			ban[0x0a07] = 1;
		}
		if ( k == 3 ) ban[0x0a0a] = 1;
		if ( k == 5 || k == 7 || k >= 9 ) ban[0x0707] = 1;
	} else if ( ban_size == BAN_9 ) {
		ban[0x0307] = 1;		// 対角線には必ず置く
		ban[0x0703] = 1;	
		if ( k >= 4 ) {
			ban[0x0303] = 1;	// 4子以上は星は全部
			ban[0x0707] = 1;	
		}
		if ( k >= 6 ) {
			ban[0x0503] = 1;	// 6子以上は左右中央の星
			ban[0x0507] = 1;	
		}
		if ( k >= 8 ) {
			ban[0x0305] = 1;	// 8子以上は上下中央の星
			ban[0x0705] = 1;
		}
		if ( k == 3 ) ban[0x0707] = 1;
		if ( k == 5 || k == 7 || k >= 9 ) ban[0x0505] = 1;
	}

	for (i=0;i<BANMAX;i++) {
		ban_start[i] = ban[i];	// 開始盤面にも代入
	}
	SendMessage(ghWindow,WM_COMMAND,IDM_SET_BOARD,0L);	// 盤面初期化
}


/* 情報表示ダイアログ */
void OpenPrintWindow(HWND hWnd)
{
	WNDCLASS wc;
	HBRUSH hBrush;
//	RECT rc;
	static int bFirstTime = TRUE;

	if ( bFirstTime ) {
		bFirstTime = FALSE;
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;//PARENTDC;
		wc.lpszClassName = "PrintClass";
		wc.hInstance     = ghInstance;
		wc.lpfnWndProc   = PrintProc;
		wc.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wc.hIcon         = LoadIcon( ghInstance, CGF_ICON );
		wc.lpszMenuName  = NULL;
		hBrush = CreateSolidBrush( RGB( 0,0,128 ) );	// 背景を緑に(wc.中のオブジェクトは終了時に自動的に削除される）
		wc.hbrBackground = hBrush;//	GetStockObject(WHITE_BRUSH);
//		wc.hbrBackground = GetStockObject(WHITE_BRUSH);
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		if ( !RegisterClass(&wc) ) MessageBox(ghWindow,"情報表示Winodwの登録に失敗","エラー",MB_OK);
	}
	/********** ウインドウの作成 ********/
//	GetWindowRect( hWnd, &rc );	// 現在の画面の絶対位置を取得。
//	PRT("%d,%d,%d,%d\n",rc.left, rc.top,rc.right, rc.bottom);

	hPrint = CreateWindow(
				"PrintClass",						/* Class name.   */                                  
				szInfoWinName,						/* Title.        */
				WS_VSCROLL | WS_OVERLAPPEDWINDOW,	/* Style bits.   */
//				rc.right, rc.top,					/* X,Y  表示座標 */
				640, 20,							/* X,Y  表示座標 */
//				600, 50,							/* X,Y  表示座標 */
//				700,560,							/* CX, CY 大きさ */
				700,PHeight*17,						/* CX, CY 大きさ */
				hWnd,								/* 親はなし      */
				NULL,								/* Class Menu.   */
				ghInstance,							/* Creator       */
				NULL								/* Params.       */
			   );

	LoadMainWindowPlacement(hPrint,2);	// レジストリから情報を取得
	ShowWindow( hPrint, SW_SHOW);
}

LRESULT CALLBACK PrintProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	int i,y=0;
	PAINTSTRUCT ps;
	HMENU hMenu;
	TEXTMETRIC tm;
	static HDC hDC;
	static HFONT hFont,hOldFont;

	switch( msg ) {
		case WM_CREATE:
			PRT_y = PMaxY - PHeight;		// 基本は25行表示
			SetScrollRange( hWnd, SB_VERT, 0, PRT_y, FALSE);	
			SetScrollPos( hWnd, SB_VERT, PRT_y, FALSE );
			hDC = GetDC(hWnd);

//			hFont = GetStockObject(OEM_FIXED_FONT);
//			hFont = GetStockObject(SYSTEM_FONT);
//			hFont = GetStockObject(ANSI_FIXED_FONT);

			hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);	// Win98で文字幅が崩れないように
			hOldFont = (HFONT)SelectObject(hDC,hFont);

			GetTextMetrics(hDC, &tm);
			nHeight = tm.tmHeight;
			nHeight -= 2;	// わざと2ドット縦を減らしている。情報量が増える 上のFIXED_FONTで使用
			nWidth  = tm.tmAveCharWidth;
			SetTextColor( hDC, RGB(255,255,255) );	// 文字を白に
			SetBkColor( hDC, RGB(0,0,128) );		// 背景を濃い青に
			SetBkMode( hDC, TRANSPARENT);			// 背景を消さない（文字の上下が1ドット消されるのを防ぐ）
			ReleaseDC( hWnd, hDC );
			break;

		case WM_PAINT:
			hDC = BeginPaint(hWnd, &ps);
			for (i=PRT_y; i<PMaxY; i++) {
				TextOut(hDC,0,(i-PRT_y)*(nHeight),cPrintBuff[i],strlen(cPrintBuff[i]));
			}
			EndPaint(hWnd, &ps);
			break;
		
		case WM_KEYDOWN:
		case WM_VSCROLL:
			y = PRT_y;
			switch ( (int)LOWORD(wParam) ) {
				case SB_THUMBPOSITION:	// サム移動
					PRT_y = (short int) HIWORD( wParam );
//					PRT("wParam=%x ,(short int)HIWORD()=%d, lParam=%x\n",wParam,(short int )HIWORD(wParam ),lParam );
					break;
				case SB_LINEDOWN:
				case VK_DOWN:
					PRT_y++;
					break;
				case SB_PAGEDOWN:
				case VK_NEXT:
					PRT_y += PHeight - 1;
					break;
				case SB_LINEUP:
				case VK_UP:
					PRT_y--;
					break;
				case SB_PAGEUP:
				case VK_PRIOR:
					PRT_y -= PHeight - 1;
					break;
				case VK_HOME:
					PRT_y = 0;
					break;
				case VK_END:
					PRT_y = PMaxY;
					break;
			}

#if (_MSC_VER >= 1100)		// VC++5.0以降でサポート
		case WM_MOUSEWHEEL:	// ---> これは無理やり <zmouse.h> で定義している。
			if ( msg == WM_MOUSEWHEEL ) {
				y = PRT_y;
				PRT_y -= ( (short) HIWORD(wParam) / WHEEL_DELTA ) * 3; 
			}
#endif
			if ( PRT_y <= 0 ) PRT_y = 0;
			if ( PRT_y >= PMaxY - PHeight ) PRT_y = PMaxY - PHeight;
			
			SetScrollPos( hWnd, SB_VERT, PRT_y, TRUE);
			if ( y != PRT_y ) ScrollWindow( hWnd, 0, (nHeight) * (y-PRT_y), NULL, NULL );
			break;

		case WM_LBUTTONDOWN:
			break;
		
		case WM_LBUTTONUP:
			break;

		case WM_RBUTTONDOWN:
			{	POINT pt;
				HMENU hSubmenu;
				pt.x = LOWORD(lParam);
				pt.y = HIWORD(lParam);
//				PRT("右クリックされたね (x,y)=%d,%d\n",pt.x,pt.y);
				hMenu = LoadMenu(ghInstance, "PRT_POPUP");
				hSubmenu = GetSubMenu(hMenu, 0);
			
				ClientToScreen(hWnd, &pt);
				TrackPopupMenu(hSubmenu, TPM_LEFTALIGN, pt.x, pt.y, 0, ghWindow, NULL);	// POPUPの親をメインにしないとメッセージが正しく飛ばない。
				DestroyMenu(hMenu);
			}
			break;

		case WM_CLOSE:
			fInfoWindow = 0;	// ユーザがPrintWindowのみを閉じようとした場合。
			return( DefWindowProc( hWnd, msg, wParam, lParam) );

		case WM_DESTROY:
			hMenu = GetMenu(ghWindow);
			CheckMenuItem(hMenu, IDM_PRINT, MF_UNCHECKED);
			DeleteObject(SelectObject(hDC, hOldFont));	/* フォントを解放 */
			if ( SaveMainWindowPlacement(hPrint,2) == 0 ) PRT("レジストリ２のエラー（書き込み）\n");
			hPrint = NULL;
			break;

		default:
			return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}
	return 0L;
}


#define CGF_URL "http://www32.ocn.ne.jp/~yss/cgfgoban.html"

/* アバウトの表示 */
LRESULT CALLBACK AboutDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/ )
{
	char sTmp[TMP_BUF_LEN];

	switch( msg ) {
		case WM_INITDIALOG:
			sprintf(sTmp,"%s %s",sVersion,sVerDate);
			SetDlgItemText(hDlg, IDM_CGF_VERSION, sTmp);
			SetDlgItemText(hDlg, IDM_HOMEPAGE, CGF_URL);
			return TRUE;	// TRUE を返せば SetFocus(hDlg); は必要ない？

		case WM_COMMAND:
			if ( wParam == IDM_HOMEPAGE ) ShellExecute(hDlg, NULL, CGF_URL, NULL, NULL, SW_SHOWNORMAL);	// これだけでIE5.0?が起動される
			if ( wParam == IDOK || wParam == IDCANCEL ) EndDialog( hDlg, 0);
			break;
	}
	return FALSE;
}

// Gnugo用の設定をするダイアログの処理
BOOL CALLBACK GnuGoSettingProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/)
{
	HWND hWnd;
	int i;

	switch(msg) {
		case WM_INITDIALOG:
			if ( nGtpPath == 0 ) {	// 未登録なら標準を追加。最後が最新。
				UpdateStrList("c:\\go\\aya\\aya.exe --mode gtp",                    &nGtpPath,GTP_PATH_MAX, sGtpPath );
				UpdateStrList("c:\\go\\MoGo\\mogo.exe --19 --time 12",              &nGtpPath,GTP_PATH_MAX, sGtpPath );
				UpdateStrList("c:\\go\\mogo\\mogo.exe --9 --nbTotalSimulations 10000", &nGtpPath,GTP_PATH_MAX, sGtpPath );
//				UpdateStrList("c:\\go\\CrazyStone\\CrazyStone-0005.exe",            &nGtpPath,GTP_PATH_MAX, sGtpPath );
//				UpdateStrList("c:\\go\\cgfgtp\\Release\\cgfgtp.exe",                &nGtpPath,GTP_PATH_MAX, sGtpPath );
				UpdateStrList("c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign", &nGtpPath,GTP_PATH_MAX, sGtpPath );
				UpdateStrList("c:\\go\\gnugo\\gnugo.exe --mode gtp",                &nGtpPath,GTP_PATH_MAX, sGtpPath );
			}
			hWnd = GetDlgItem(hDlg, IDM_GTP_PATH);
			for (i=0;i<nGtpPath;i++) SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)sGtpPath[i]);
			SendMessage(hWnd, CB_SETCURSEL, 0,0);

//			SetDlgItemText(hDlg, IDM_GNUGO_DIRECTORY, sGnugoDir);
//			SetDlgItemText(hDlg, IDC_GNUGO_EX1, "c:\\go\\gnugo\\gnugo.exe --mode gtp");
//			SetDlgItemText(hDlg, IDC_GNUGO_EX2, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --level 12");
//			SetDlgItemText(hDlg, IDC_GNUGO_EX3, "c:\\go\\aya\\aya.exe --mode gtp");
			CheckDlgButton(hDlg, IDM_GNUGO_LIFEDEATH, fGnugoLifeDeathView);
			return TRUE;

		case WM_COMMAND:
			if ( wParam == IDCANCEL ) EndDialog(hDlg,FALSE);		// キャンセル
			if ( wParam == IDOK ) {
				char sTmp[TMP_BUF_LEN];
				GetDlgItemText(hDlg, IDM_GTP_PATH, sTmp, TMP_BUF_LEN-1);
				if ( strlen(sTmp) > 0 ) {	// 文字列配列の候補リストを更新。
					UpdateStrList(sTmp, &nGtpPath,GTP_PATH_MAX, sGtpPath );
				}
//				GetDlgItemText(hDlg, IDM_GNUGO_DIRECTORY, sGnugoDir, MAX_PATH-1);
				fGnugoLifeDeathView = IsDlgButtonChecked( hDlg, IDM_GNUGO_LIFEDEATH );
				EndDialog(hDlg,FALSE);
			}
			return TRUE;
	}
	return FALSE;
}



// 対局条件を設定するダイアログボックス関数
BOOL CALLBACK StartProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i,n;
	char sCom[TMP_BUF_LEN];
	const int nTypeMax = 5;
	char *sType[nTypeMax] = { "Human","Computer(DLL)","LAN(nngs)","SGMP(RS232C)","Computer(GTP)", };
	const int handi_max = 9;
#ifdef CGF_E
	static char *handi_str[handi_max] = { "None","2","3","4","5","6","7","8","9" };
	static char *err_str[] = { "One player must be human or computer." };
	static char *sKomi[3] = { "None","","" };
	static char *sSize[3] = { "9","13","19" };
#else
	static char *handi_str[handi_max] = { "なし","2子","3子","4子","5子","6子","7子","8子","9子" };
	static char *err_str[] = { "両者とも通信対局は指定できません" };
	static char *sKomi[3] = { "なし","半目","目半" };
	static char *sSize[3] = { "9路","13路","19路" };
#endif
	static int fInitBoard = 0;	// 局面を初期化する場合

	switch(msg) {
		case WM_INITDIALOG:
			SetFocus(hDlg);
//			PRT("lParam=%d\n",lParam);
			if ( lParam == IDYES ) {	// 初期化して対局
				fInitBoard = 1;
			} else {
				fInitBoard = 0;
				EnableWindow(GetDlgItem(hDlg, IDM_HANDICAP),   FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_BOARD_SIZE), FALSE);
			}

			for (n=0;n<2;n++) {
				int type_col[2] = { IDC_BLACK_TYPE, IDC_WHITE_TYPE };
				HWND hWnd = GetDlgItem(hDlg, type_col[n]);
				SendMessage(hWnd, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
				for (i=0;i<nTypeMax;i++) SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)sType[i]);
				SendMessage(hWnd, CB_SETCURSEL, turn_player[n], 0L);
			}

			// 最低でも1つの名前を登録する
			UpdateStrList(sPlayerName[1], &nNameList,NAME_LIST_MAX, sNameList );
			UpdateStrList(sPlayerName[0], &nNameList,NAME_LIST_MAX, sNameList );
			for (n=0;n<2;n++) {
				int type_col[2] = { IDC_BLACK_NAME, IDC_WHITE_NAME };
				HWND hWnd = GetDlgItem(hDlg, type_col[n]);
				SendMessage(hWnd, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
				for (i=0;i<nNameList;i++) SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)sNameList[i]);
				SendMessage(hWnd, CB_SETCURSEL, n, 0L);
			}
			SendMessage(hDlg, WM_COMMAND, MAKELONG(IDC_BLACK_TYPE,CBN_SELCHANGE), 0L);	// NNGS,SGMPの表示をdisenableにする
//			SetDlgItemText(hDlg, IDC_BLACK_NAME, sPlayerName[0] );
//			SetDlgItemText(hDlg, IDC_WHITE_NAME, sPlayerName[1] );

			{
				HWND hWnd = GetDlgItem(hDlg, IDM_COM_PORT);
				SendMessage(hWnd, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
				for (i=0;i<8;i++) {
					sprintf(sCom,"COM%d",i+1);
					SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)sCom);
				}
				SendMessage(hWnd, CB_SETCURSEL, 0, 0L);
			}
			
			// 置石
			{
				HWND hWnd = GetDlgItem(hDlg, IDM_HANDICAP);
				for (i=0;i<handi_max;i++) SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)handi_str[i]);
				SendMessage(hWnd, CB_SETCURSEL, 0, 0L);
//				if ( ban_size != BAN_19 ) EnableWindow(hWnd,FALSE);
				n = nHandicap;
				if ( n < 0 || n > handi_max ) n = 0;
				if ( n > 1 ) SendMessage(hWnd, CB_SETCURSEL, n-1, 0L);
			}

			// コミ
			{
				HWND hWnd = GetDlgItem(hDlg, IDM_KOMI);
				int n = 0;
				for (i=0;i<20;i++) {
					double k = i/2.0;
					sprintf(sCom,"%.1f",k);
					if ( komi == k ) n = i;
//					if ( k >= 2 ) sprintf(sCom,"%d%s",i-1,sKomi[2]);
//					else          sprintf(sCom,"%s",      sKomi[i]);
					SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)sCom);
				}
				SendMessage(hWnd, CB_SETCURSEL, n, 0L);
			}

			// 盤のサイズ
			{
				HWND hWnd = GetDlgItem(hDlg, IDC_BOARD_SIZE);
				for (i=0;i<3;i++) {
					sprintf(sCom,"%s",sSize[i]);
					SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)sCom);
				}
				n = 0;
				if ( ban_size == 13 ) n = 1;
				if ( ban_size == 19 ) n = 2;
				SendMessage(hWnd, CB_SETCURSEL, n, 0L);
			}

			if ( nNngsIP == 0 ) {	// 未登録なら標準を追加。最後が最新。
				UpdateStrList("192.168.0.1",         &nNngsIP,NNGS_IP_MAX, sNngsIP );
				UpdateStrList("nngs.computer-go.jp", &nNngsIP,NNGS_IP_MAX, sNngsIP );	// CGFのnngsサーバ
			}
			{
				HWND hWnd = GetDlgItem(hDlg, IDM_NNGS_SERVER);
				for (i=0;i<nNngsIP;i++) SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)sNngsIP[i]);
				SendMessage(hWnd, CB_SETCURSEL, 0, 0L);
				SetDlgItemText(hDlg, IDM_NNGS_NAME1,nngs_player_name[0]);
				SetDlgItemText(hDlg, IDM_NNGS_NAME2,nngs_player_name[1]);
				SetDlgItemInt(hDlg,  IDM_NNGS_MINUTES,nngs_minutes,FALSE);
			}
			CheckDlgButton(hDlg, IDM_AI_RYUSEI, fAIRyusei);

			break;

		case WM_COMMAND:
			if ( wParam == IDCANCEL ) EndDialog(hDlg,FALSE);		// キャンセル
			if ( wParam == IDOK ) {
				// 入力をチェック
				turn_player[0] = SendDlgItemMessage(hDlg,IDC_BLACK_TYPE, CB_GETCURSEL, 0, 0L);
				turn_player[1] = SendDlgItemMessage(hDlg,IDC_WHITE_TYPE, CB_GETCURSEL, 0, 0L);
				PRT("turn[0]=%d,%d\n",turn_player[0],turn_player[1]);
				fUseNngs = 0;
				int sgmp = 0;
				for (i=0;i<2;i++) {
					n = turn_player[i];
					if ( n == PLAYER_NNGS || n == PLAYER_SGMP ) sgmp |= (i+1);
					if ( n == PLAYER_NNGS ) fUseNngs = 1;
				}
				if ( sgmp == 3 ) {
					MessageBox(hDlg,err_str[0],"Player Select Error",MB_OK);
					break;
				}
				if ( sgmp ) dll_col = UN_COL(sgmp);

//				gct1 = clock();			// user の思考時間計測用
				
				for (n=0;n<2;n++) {
					int type_col[2] = { IDC_BLACK_NAME, IDC_WHITE_NAME };
					GetDlgItemText(hDlg, type_col[n], sPlayerName[n], TMP_BUF_LEN-1);
					if ( strlen(sPlayerName[n]) > 0 ) {	// 文字列配列の候補リストを更新。
						UpdateStrList(sPlayerName[n], &nNameList,NAME_LIST_MAX, sNameList );
					}
				}
			
				// 新規対局では盤面を初期化
				if ( fInitBoard ) {
					// サイズ
					n = SendDlgItemMessage(hDlg, IDC_BOARD_SIZE, CB_GETCURSEL, 0, 0L);
					ban_size = BAN_9;
					if ( n == 1 ) ban_size = BAN_13;
					if ( n == 2 ) ban_size = BAN_19;
					init_ban();
					SendMessage(ghWindow,WM_COMMAND,IDM_BAN_CLEAR,0L);	// 盤面初期化

					// 置石
					n = SendMessage(GetDlgItem(hDlg, IDM_HANDICAP), CB_GETCURSEL, 0, 0L);
					if ( n == CB_ERR ) { PRT("置石取得Err\n"); n = 0; }
					if ( n ) {	// 置石がある
						nHandicap = n + 1;	// 白が先手で打つ
						SetHandicapStones();
					} else nHandicap = 0;
				}

				// コミ
				n = SendDlgItemMessage(hDlg, IDM_KOMI, CB_GETCURSEL, 0, 0L);
				komi = n/2.0;
				if ( komi < 0 ) komi = 0;
				
				// NNGS情報の取得
				{
					char sTmp[TMP_BUF_LEN];
					GetDlgItemText(hDlg, IDM_NNGS_SERVER, sTmp, TMP_BUF_LEN-1);
					if ( strlen(sTmp) > 0 ) {
						UpdateStrList(sTmp, &nNngsIP,NNGS_IP_MAX, sNngsIP );
					}
					GetDlgItemText(hDlg, IDM_NNGS_NAME1,nngs_player_name[0],TMP_BUF_LEN-1);
					GetDlgItemText(hDlg, IDM_NNGS_NAME2,nngs_player_name[1],TMP_BUF_LEN-1);
					nngs_minutes = GetDlgItemInt(hDlg, IDM_NNGS_MINUTES,NULL,FALSE);
					fAIRyusei = IsDlgButtonChecked( hDlg, IDM_AI_RYUSEI );
				}

				// COMポートの取得
				nCommNumber = 1 + SendDlgItemMessage(hDlg,IDM_COM_PORT, CB_GETCURSEL, 0, 0L);
		
				// 対局中に入る
				fNowPlaying = 1;
				MenuEnablePlayMode(0);	// 対局中は棋譜移動メニューを禁止する

				// GnuGoの初期化
				for (i=0;i<2;i++) {
					if ( turn_player[i] == PLAYER_GNUGO ) break;
				}
				if ( i!=2 ) {
					if ( fGnugoPipe ) close_gnugo_pipe();
					if ( fGnugoPipe==0 ) {
						if ( open_gnugo_pipe() == 0 ) {
							MessageBox(hDlg,"GTP engine path error","GTP err",MB_OK);
							break;	
						}
					}
					char sName[TMP_BUF_LEN];
					if ( init_gnugo_gtp(sName) == 0 ) break;
					for (i=0;i<2;i++) {
						if ( turn_player[i] == PLAYER_GNUGO ) strcpy(sPlayerName[i],sName);
					}
					// Gnugo側の盤面を設定
					int x,y,z,k;
					for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
						z = ((y+1)<<8) + x+1 ;
						k = ban[z];
						if ( k==0 ) continue;
						PutStoneGnuGo(z, (k==1));
					}
				}

				if ( sgmp ) {
					PostMessage(ghWindow,WM_COMMAND,IDM_INIT_COMM,0L);	// 通信対戦の初期化処理
				} else {
					PostMessage(ghWindow,WM_COMMAND,IDM_NEXT_TURN,0L);
				}
				EndDialog(hDlg,FALSE);			
				PRT("ban=%d,nHandi=%d,komi=%.1f,nComm=%d\n",ban_size,nHandicap,komi,nCommNumber);
			}

			switch ( LOWORD(wParam) ) {
				case IDC_BLACK_TYPE:
				case IDC_WHITE_TYPE:
					if ( HIWORD(wParam) == CBN_SELCHANGE ) {
						int p[2];
						int kind = 0,flag;
						p[0] = SendDlgItemMessage(hDlg,IDC_BLACK_TYPE, CB_GETCURSEL, 0, 0L);
						p[1] = SendDlgItemMessage(hDlg,IDC_WHITE_TYPE, CB_GETCURSEL, 0, 0L);
						PRT("turn[0]=%d,%d\n",p[0],p[1]);
						for (i=0;i<2;i++) {
							n = p[i];
							if ( n == PLAYER_NNGS ) kind |= 1;
							if ( n == PLAYER_SGMP ) kind |= 2;
						}
						flag = FALSE;
						if ( kind & 2 ) flag = TRUE;
						EnableWindow(GetDlgItem(hDlg, IDM_COM_PORT), flag);
						flag = TRUE;
						if ( kind & 1 ) flag = FALSE;
						EnableWindow(GetDlgItem(hDlg, IDM_NNGS_SERVER), !flag);
						SendDlgItemMessage(hDlg,IDM_NNGS_NAME1,  EM_SETREADONLY, flag, 0L);
						SendDlgItemMessage(hDlg,IDM_NNGS_NAME2,  EM_SETREADONLY, flag, 0L);
						// 標準の名前をセット
						if ( LOWORD(wParam) == IDC_BLACK_TYPE ) SetDlgItemText(hDlg, IDC_BLACK_NAME, sType[p[0]] );
						if ( LOWORD(wParam) == IDC_WHITE_TYPE ) SetDlgItemText(hDlg, IDC_WHITE_NAME, sType[p[1]] );
						SendDlgItemMessage(hDlg,IDM_NNGS_MINUTES, EM_SETREADONLY, flag, 0L);
					}
					break;
/*
				case IDC_BLACK_NAME:
					PRT("%08x,%08x\n",wParam, HIWORD(wParam));
					if ( HIWORD(wParam) == CBN_SELCHANGE ) {
						GetDlgItemText(hDlg, IDC_BLACK_NAME, sTmp, TMP_BUF_LEN-1);
						SetDlgItemText(hDlg, IDM_NNGS_NAME1,sTmp);
					}
					break;

				case IDC_WHITE_NAME:
					if ( HIWORD(wParam) == CBN_SELCHANGE ) {
						GetDlgItemText(hDlg, IDC_WHITE_NAME, sTmp, TMP_BUF_LEN-1);
						SetDlgItemText(hDlg, IDM_NNGS_NAME2,sTmp);
					}
					break;
*/
			}
			return TRUE;
	}
	return FALSE;
}

// メッセージを表示して対局を停止
void GameStopMessage(HWND hWnd,int ret, int z)
{
	char sTmp[TMP_BUF_LEN];
	static char *err_str[] = {
#ifdef CGF_E
		"OK",
		"Suicide move",
		"Ko!",
		"Stone is already here!",
		"Unknown error",
		"Both player passed. Game end.",
		"Too many moves.",
		"resign",
#else
		"OK",
		"自殺手",
		"コウ",
		"既に石があります",
		"その他のエラー",
		"連続パスで終局します",
		"手数が長すぎるため中断します",
		"投了です",
#endif
	};
	if ( ret >= MOVE_PASS_PASS ) {
		MessageBox(hWnd,err_str[ret],"Game Stop",MB_OK);
	} else {
		int x = z & 0xff;
		int y = z >> 8;
		sprintf(sTmp,"(%2d,%2d)\n\n%s",x,y,err_str[ret]);
		MessageBox(hWnd,sTmp,"move err",MB_OK);
	}
}


/*** 描画用子WINDOWの処理。このWindowに描画を行う。 ***/
LRESULT CALLBACK DrawWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	int k,z,col;
	static char *err_str[] = {
#ifdef CGF_E
		"You can not put or get a stone whlie viewing life and death.\n",
		"Stone is already here!\n",
		"Suicide move!",
		"Ko!",
		"You can not put or get ston whlie viewing life and death stone.\n",
#else
		"死活表示中は石は置けません。\n",
		"そこには石がありますよ。\n",
		"自殺手です！",
		"劫ですよ！",
		"死活表示中は石は取れません。\n",
#endif
	};

	if ( fPassWindows ) {	// 思考中はすべてカットする。
		return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}

	switch( msg ) {
		case WM_PAINT:
			PaintBackScreen(hWnd);
			return 0;

		case WM_LBUTTONDOWN:
			if ( fNowPlaying == 0 ) {	// 盤面編集　
//				SetIshi( z, 1 );
				return 0; 
			}
			z = ChangeXY(lParam);
//			PRT("左＝%x  ",z);
			if ( z == 0 ) return 0;
			if ( endgame_board_type ) {
				PRT("%s",err_str[0]);
				return 0;
			}
			if ( ban[z] != 0 ) {
				PRT("%s",err_str[1]);
				return 0;
			}

			col = GetTurnCol();		// 手番と置石ありか？で色を決める

			k = make_move(z,col);
			if ( k==MOVE_SUICIDE ) {
				MessageBox(hWnd,err_str[2],"Move error",MB_OK);	// 自殺手
				return 0;
			}
			if ( k==MOVE_KOU ) {
				MessageBox(hWnd,err_str[3],"Move error",MB_OK);	// コウ
				return 0;
			}
			if ( k!=MOVE_SUCCESS ) {
				PRT("move_err\n");
				debug();
			}
			
			if ( IsGnugoBoard()    ) UpdateGnugoBoard(z);
			if ( IsCommunication() ) SendMove(z);

			tesuu++;	all_tesuu = tesuu;
			kifu[tesuu][0] = z;	// 棋譜を記憶
			kifu[tesuu][1] = col;	// 手番で色を決める
			kifu[tesuu][2] = (int)GetSpendTime(gct1);

			PaintUpdate();
			if ( SoundDrop ) {
//				MessageBeep(MB_OK);	// 音を鳴らしてみますか
				sndPlaySound("stone.wav", SND_ASYNC | SND_NOSTOP);	// MMSYSTEM.H をインクルードする必要あり。またWinMM.libをリンク
			}
			PostMessage(hWndMain, WM_COMMAND, IDM_NEXT_TURN, 0L);
			return 0;
/*
		case WM_RBUTTONDOWN:
			z = ChangeXY(lParam);
			if ( z == 0 ) return (0);
			if ( endgame_board_type ) {
				PRT("%s",err_str[4]);
				return 0;
			}
			if ( fNowPlaying == 0 ) {	// 盤面編集　
				SetIshi( z, 2 );
			}
			return 0;
*/
		default:
			return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}
//	return 0L;
}

// 描画用Windowを同時に動かす
void MoveDrawWindow(HWND hWnd)
{
    RECT rc,rcClient;

    GetClientRect(hWnd, &rcClient);

    GetWindowRect(hWndToolBar,&rc);
    ScreenToClient(hWnd,(LPPOINT)&rc.right);
    rcClient.top = rc.bottom;

     MoveWindow(hWndDraw,
                   rcClient.left,
                   rcClient.top,
                   rcClient.right-rcClient.left,
                   rcClient.bottom-rcClient.top,
                   TRUE);
}

// Windowsへ一時的に制御を渡す。User からの指示による思考停止。
void PassWindowsSystem(void)
{
	MSG msg;

	fPassWindows = 1;	// Windowsに処理を渡してる。
	if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) ) {
		TranslateMessage( &msg );						// keyboard input.
		DispatchMessage( &msg );
	}
	fPassWindows = 0;
}	


/*** 棋譜をセーブする ***/
void KifuSave(void)
{
	OPENFILENAME ofn;
	char szDirName[256];
	char szFile[256], szFileTitle[256];
	UINT  cbString;
	char  chReplace;    /* szFilter文字列の区切り文字 */
	char  szFilter[256];
	char  Title[256];	// Title
	HANDLE hFile;		// ファイルのハンドル
	int i;
	unsigned long RetByte;		// 書き込まれたバイト数（ダミー）
	static int OnlyOne = 0;	// 連続対戦の回数（最初の1回だけ書き込まれる)。

	time_t timer;
	struct tm *tblock;
	unsigned int month,day,hour,minute;

	/* システム ディレクトリ名を取得してszDirNameに格納 */
	GetSystemDirectory(szDirName, sizeof(szDirName));
	szFile[0] = '\0';

	if ((cbString = LoadString(ghInstance, IDS_SAVE_FILTER,
	        szFilter, sizeof(szFilter))) == 0) {
	    return;
	}
	chReplace = szFilter[cbString - 1]; /* ワイルド カードを取得 */

	for (i = 0; szFilter[i] != '\0'; i++) {
	    if (szFilter[i] == chReplace)
	       szFilter[i] = '\0';
	}


	/* 時刻の取得 */
	timer = time(NULL);
	tblock = localtime(&timer);   /* 日付／時刻を構造体に変換する */
//	PRT("Local time is: %s", asctime(tblock));
//	second = tblock->tm_sec;      /* 秒 */
	minute = tblock->tm_min;      /* 分 */
	hour   = tblock->tm_hour;     /* 時 (0～23) */
	day    = tblock->tm_mday;     /* 月内の通し日数 (1～31) */
	month  = tblock->tm_mon+1;    /* 月 (0～11) */
//	year   = tblock->tm_year + 1900;     /* 年 (西暦 - 1900) */
//	week   = tblock->tm_wday;     /* 曜日 (0～6; 日曜 = 0) */
//	PRT( "Date %02x-%02d-%02x / Time %02x:%02x:%02x\n", year, month, day, hour, minute, second );

//	sprintf( szFileTitle, "%02d%02d%02d%02d", month, day, hour, minute );	// ファイル名を日付＋秒に
//	if ( fAutoSave ) sprintf( szFile, "fig%03d.sgf",++OnlyOne );	// 自動セーブの時。
	if ( fAutoSave ) sprintf( szFile, "%x%02d@%02d%02d.sgf", month, day, hour,++OnlyOne );	// 自動セーブの時。
	else             sprintf( szFile, "%x%02d-%02d%02d",     month, day, hour, minute );	// ファイル名を日付＋秒に


	/* 構造体のすべてのメンバを0に設定 */
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = ghWindow;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile= szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
//	ofn.lpstrInitialDir = szDirName;
	ofn.lpstrInitialDir = NULL;		// カレントディレクトリに
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt	= "sgf";	// デフォルトの拡張子を ".sgf" に
	
	if ( fAutoSave || GetSaveFileName(&ofn) ) {
		hFile = CreateFile(ofn.lpstrFile,      /* create MYFILE.TXT  */
	             GENERIC_WRITE,                /* open for writing   */
	             0,                            /* do not share       */
		         (LPSECURITY_ATTRIBUTES) NULL, /* no security        */
			     CREATE_ALWAYS,                /* overwrite existing */
				 FILE_ATTRIBUTE_NORMAL,        /* normal file        */
	             (HANDLE) NULL);               /* no attr. template  */
		if (hFile == INVALID_HANDLE_VALUE) {
			PRT("Could not open file.");       /* process error      */
			return;
		}

		// タイトルにファイル名を付ける
		sprintf(Title,"%s - %s",ofn.lpstrFile, szTitleName);
		SetWindowText(ghWindow, Title);

		// ファイルリストを更新。一番最近開いたものを先頭へ。
		UpdateRecentFile(ofn.lpstrFile);
		UpdateRecentFileMenu(ghWindow);

		if ( stricmp(ofn.lpstrFile + ofn.nFileExtension, "sgf" ) == 0 || fAutoSave ) {
			SaveSGF();	// SGFで書き出す
		} else {
			SaveIG1();	// IG1 形式で書き込む。
		}
		WriteFile( hFile,(void *)SgfBuf, strlen(SgfBuf), &RetByte, NULL );
		CloseHandle(hFile);		// ファイルをクローズ
	}
}


/*** 棋譜をロードする ***/
int KifuOpen(void)
{
	OPENFILENAME ofn;
	char szDirName[256];
	char szFile[256], szFileTitle[256];
	UINT  cbString;
	char  chReplace;    /* szFilter文字列の区切り文字 */
	char  szFilter[256];
	char  Title[256];	// Title

	HANDLE hFile;		// ファイルのハンドル
	int i;

	szFile[0] = '\0';
	if ((cbString = LoadString(ghInstance, IDS_LOAD_FILTER, szFilter, sizeof(szFilter))) == 0) return FALSE;
	chReplace = szFilter[cbString - 1]; /* ワイルド カードを取得 */
	for (i = 0; szFilter[i] != '\0'; i++) if (szFilter[i] == chReplace) szFilter[i] = '\0';

	/* 構造体のすべてのメンバを0に設定 */
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = ghWindow;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile= szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	GetCurrentDirectory(sizeof(szDirName), szDirName);	// カレントディレクトリ名を取得してszDirNameに格納
	ofn.lpstrInitialDir = szDirName;	// カレントディレクトリに
//	ofn.lpstrInitialDir = NULL;			// ---> これだとWin98以降で*.amzファイルがないとMyDocumentsが標準にされる
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if ( strlen(cDragFile) || GetOpenFileName(&ofn) ) {
		if ( strlen(cDragFile) ) {	// ダブルクリックで起動された時
			char *p;

			if ( cDragFile[0] == '"' ) {	// 前後の "" を消す
				strcpy(szFile,cDragFile+1);
				strcpy(cDragFile,szFile);
				cDragFile[ strlen(cDragFile)-1 ] = 0;
			}

			strcpy(szFile,cDragFile);	// szFileにコピー
			ofn.lpstrFile = szFile;
			p = strrchr( ofn.lpstrFile, '.' );	// 最後のピリオドを探す
			if ( p == NULL ) ofn.nFileExtension = 0;	// どのみちエラー
			else             ofn.nFileExtension = (p - ofn.lpstrFile)+1;
			PRT("%s,%d,%s\n",ofn.lpstrFile,ofn.nFileExtension,ofn.lpstrFile + ofn.nFileExtension);
			strcpy(ofn.lpstrFileTitle,ofn.lpstrFile);
			cDragFile[0] = '\0';	// ファイル名を消す	
		}
		hFile = CreateFile(ofn.lpstrFile,      /* create MYFILE.TXT  */
	             GENERIC_READ,                 /* open for reading   */
	             0,                            /* do not share       */
		         (LPSECURITY_ATTRIBUTES) NULL, /* no security        */
			     OPEN_EXISTING,                /* overwrite existing */
				 FILE_ATTRIBUTE_NORMAL,        /* normal file        */
	             (HANDLE) NULL);               /* no attr. template  */
		if (hFile == INVALID_HANDLE_VALUE) {
			PRT("Could not open file =%s\n",ofn.lpstrFile);       /* process error      */
			return FALSE;
		}
 
		// タイトルにファイル名を付ける
		sprintf(Title,"%s - %s",ofn.lpstrFileTitle, szTitleName);
		SetWindowText(ghWindow, Title);

		// ファイルリストを更新。一番最近開いたものを先頭へ。
		UpdateRecentFile(ofn.lpstrFile);
		UpdateRecentFileMenu(ghWindow);


		PRT("\n%s\n",ofn.lpstrFile);

		// 全てのファイルをバッファに読み込む
		ReadFile( hFile, (void *)SgfBuf, SGF_BUF_MAX, &nSgfBufSize, NULL );
		CloseHandle(hFile);		// ファイルをクローズ

		int fSgf = 0;
		// SGF (Standard Go Format) を読み込む。
		if ( stricmp(ofn.lpstrFile + ofn.nFileExtension, "sgf" ) == 0 ) fSgf = 1;
		// IG1 (Nifty-Serve GOTERM) を読み込む。
		if ( stricmp(ofn.lpstrFile + ofn.nFileExtension, "ig1" ) == 0 ) fSgf = 0;

		LoadSetSGF(fSgf);
	}

//	PRT("読み込んだ手数=%d,全手数=%d\n",tesuu,all_tesuu);
	return TRUE;
}


unsigned long nSgfBufSize;	// バッファにロードしたサイズ
unsigned long nSgfBufNum;	// バッファの先頭位置を示す
char SgfBuf[SGF_BUF_MAX];	// バッファ

// SGF用のバッファから1バイト読み出す。
char GetSgfBuf(int *RetByte)
{
	*RetByte = 0;
	if ( nSgfBufNum >= nSgfBufSize ) return 0;
	*RetByte = 1;
	return SgfBuf[nSgfBufNum++];
}

// 一行読み込む
int ReadOneLine( char *lpLine ) 
{
	int i,RetByte;
	char c;

	i = 0;
	for ( ;; ) {
		c = GetSgfBuf(&RetByte);
		if ( RetByte == 0 ) break;	// EOF;
		if ( c == '\r' ) c = ' ';	// 小細工。たいした意味なし
		lpLine[i] = c;
		i++;
		if ( i >= 255 ) break;
		if ( c == '\r' || c == '\n' ) break;	// 改行はNULLに
	}
	lpLine[i] = 0;	// 最後にNULLをいれること！
	return (i);
}

// SGFを読み込む
int ReadSGF(void)
{
	int RetByte,nLen;
	int i,x,y,z;
	char c;
	char sCommand[256];	// SGF 用のコマンドを記憶
	BOOL fKanji;
	BOOL fAddBlack = 0;		// 置き石フラグ 


	nSgfBufNum = 0;

	for ( ;; ) {
		c = GetSgfBuf( &RetByte );
		if ( RetByte == 0 ) break;	// EOF;
		if ( c == '\r' || c == '\n' ) continue;	// 改行コードはパス
		if ( c == '(' ) break;
	}
	if ( c != '(' )	{
		PRT("SGFファイルの読み込み失敗。'(' が見つからない\n");
//		break;
	}

	nLen = 0; 
	fKanji = 0 ;
	for ( ;; ) {
		c = GetSgfBuf( &RetByte );
		if ( RetByte == 0 ) break;	// EOF;
		if ( fKanji ) {		// 漢字の場合は無条件に2文字目を読む。
			sCommand[nLen] = c;
			nLen++;
			fKanji = 0;
			continue;
		}
		if ( c == '\r' || c == '\n' ) continue;	// 改行コードはパス
		if ( 0 <= c && c < 0x20 ) continue;	// 空白以下のコード（TAB含む）もパス。
		if ( c == ';' ) {
			fAddBlack = 0;		// 置き石フラグオフ
			nLen = 0;			// 初期化してみる。
			continue;			// 区切り、現在は無視。
		}
		if ( c == ')' && nLen == 0 ) break;		// SGF File の終了。
	
		if ( fAddBlack != 0 && ( !(nLen==0 && c != '[') ) ) {	// 置き石の時の処理
//			PRT(":: %c ::\n",c);
			sCommand[nLen] = c;
			if ( nLen == 0 && c == ']' ) continue;
//			if ( nLen == 0 && c != '[' ) pass;
			nLen++;
			if ( nLen == 4 ) {
				nLen = 0;
				c = sCommand[1];
				x = c - 'a' + 1;
				c = sCommand[2];
				y = c - 'a' + 1;
				if ( x <= 0 || x >= 20 || y <= 0 || y >= 20 ) {
					PRT("置き石範囲エラー\n");
					break;
				}
				z = y * 256 + x;
				ban_start[z] = fAddBlack;
				PRT("A%c[%c%c]",'B'*(fAddBlack==1)+'W'*(fAddBlack==2), sCommand[1],sCommand[2]);
			}
			continue;
		}

				
		if ( nLen == 0 ) {	// 最初の一文字目。
			if ( c=='[' || c==']' ) {
				PRT("[]?,");
//				break;
				continue;
			}
//			if ( 'a' <= c && c <= 'z' ) continue;	// 小文字は無視。
			if ( 'A' <= c && c <= 'Z' ) {			// 大文字。コマンド。
				sCommand[nLen] = c;
				nLen++;
				fAddBlack = 0;		// 置き石フラグオフ
			}
			continue;
		}		
		if ( nLen == 1 ) {
			if ( 'A' <= c && c <= 'Z' ) {			// 大文字。コマンド。
				sCommand[nLen] = c;
				nLen++;

				if ( strncmp(sCommand,"AB",2)==0 ) {	// 置き石（黒を置く）
					// 次に大文字、または ";" が来るまで置き石を探す。
					// AB[jp][jd][pj][dj][jj][dp][pd][pp][dd]  ;W[qf];B[md];W[qn];B[mp];W[nj];
					fAddBlack = 1;
					nLen = 0;
//					PRT("ABを発見！\n");
				}
				if ( strncmp(sCommand,"AW",2)==0 ) {	// 置き石（白を置く）
					fAddBlack = 2;
					nLen = 0;
				}

				continue;
			}
			if ( c == '[' ) {	// B,W,C （黒、白、コメント）の時。
				sCommand[nLen] = c;
				nLen++;
			}
			continue;
		}
		if ( nLen >= 2 ) {
			if ( nLen == 2 && sCommand[1] != '[' && c != '[' ) continue;
			sCommand[nLen] = c;
			if ( nLen < 128 ) nLen++;	// 80文字以上は無視する
			if ( IsKanji( (unsigned char)c ) ) fKanji = 1;
//			PRT("%d:%d,",fKanji,(unsigned char)c );

			if ( c == ']' ) {	// データ終わり。
				if ( sCommand[1] != '[' || sCommand[0] == 'C' ) {	// 移動手以外は無視
					sCommand[nLen] = 0;

					if ( strncmp(sCommand,"WL",2)==0 || strncmp(sCommand,"BL",2)==0 ) {	// 白、黒の残り時間
						static int left_time[2];
						int et,diff,wb = tesuu & 1;
						sCommand[nLen-1] = 0;
						et = atoi( sCommand+3 );
						diff = left_time[wb] - et;
						left_time[wb] = et;
						if ( diff >= 0 ) kifu[tesuu][2] = diff;
						nLen = 0;
						continue;
					}

					if ( sCommand[0] == 'C' && tesuu > 0 ) PRT("%3d:",tesuu);	// 途中のコメント
					PRT("%s\n",sCommand);
																				
					if ( strncmp(sCommand,"PB",2)==0 ) {	// 対局者名を取り込む
						sCommand[nLen-1] = 0;
						strcpy( sPlayerName[0], &sCommand[3] );
					}
					if ( strncmp(sCommand,"PW",2)==0 ) {
						sCommand[nLen-1] = 0;
						strcpy( sPlayerName[1], &sCommand[3] );
					}

					if ( strncmp(sCommand,"SZ",2)==0 ) {	// 盤の大きさ
						sCommand[nLen-1] = 0;
						ban_size = atoi( &sCommand[3] );
//						if ( ban_size == BAN_19 || ban_size == BAN_13 || ban_size == BAN_9 ) ;
//						else ban_size = 19;
						init_ban();
						// 盤面を初期化 ---> 置き碁のため
						for (i=0;i<BANMAX;i++) {
							ban_start[i] = ban[i];	// 開始局面を設定。初期状態は全て０
							if ( ban_start[i] != 3 ) ban_start[i] = 0;
						}
						SendMessage(hWndMain,WM_COMMAND,IDM_BAN_CLEAR,0L);	// オープン版では盤面初期化
					}

					if ( strncmp(sCommand,"RE",2)==0 ) {	// 結果
						sCommand[nLen-1] = 0;
						strcpy( sResult, &sCommand[3] );
					}
					if ( strncmp(sCommand,"PC",2)==0 ) {	// 場所
						sCommand[nLen-1] = 0;
						strcpy( sPlace, &sCommand[3] );
					}
					if ( strncmp(sCommand,"GN",2)==0 ) {	// ラウンド
						sCommand[nLen-1] = 0;
						strcpy( sRound, &sCommand[3] );
					}
					if ( strncmp(sCommand,"EV",2)==0 ) {	// イベント名
						sCommand[nLen-1] = 0;
						strcpy( sEvent, &sCommand[3] );
					}
					if ( strncmp(sCommand,"DT",2)==0 ) {	// 日付
						sCommand[nLen-1] = 0;
						strcpy( sDate, &sCommand[3] );
					}
					if ( strncmp(sCommand,"KM",2)==0 ) {	// コミ
						sCommand[nLen-1] = 0;
						komi = atof(&sCommand[3] );
					}
					if ( strncmp(sCommand,"HA",2)==0 ) {	// 置石の数
						int n;
								
						sCommand[nLen-1] = 0;
						n = atoi( &sCommand[3] );
						nHandicap = n;
						if ( n == 13 ) n = 10;
						if ( n == 25 ) n = 11;
//						PRT("nHandicap=%d,n=%d\n",nHandicap,n);
						if ( n >= 2 ) SetHandicapStones();
					}

					nLen = 0;
					continue;
				}
				// 移動手以外は無視
				if ( sCommand[0] == 'V' || sCommand[0] == 'N' ) {	// V[B+5]は評価値？
					PRT("%s",sCommand);
					nLen = 0;
					continue;
				}
						
				if ( sCommand[0] == 'T' && sCommand[1] =='[' ) {	// 消費時間
					sCommand[nLen-1] = 0;
					kifu[tesuu][2] = atoi( sCommand+2 );
					nLen = 0;
					continue;
				}
						
				if ( sCommand[0] != 'B' && sCommand[0] != 'W' ) {	// 移動手以外は無視
					sCommand[nLen] = 0;
					PRT("%s ---> コマンドエラー！！！\n",sCommand);
//					nLen = 0;
					break;
				}
//				if ( tesuu == 0 && sCommand[0] == 'W' ) fWhiteFirst = 1;	// 白から打つ
				c = sCommand[2];
				if ( c >= 'A' && c<= 'Z' ) c += 32;	// ugf対応
				x = c - 'a' + 1;
				c = sCommand[3];
				if ( c >= 'A' && c<= 'Z' ) { c += 32; c = 'a'-2 + 'u' - c; }
				y = c - 'a' + 1;
				if ( x == 20 || y == 20 ) {
					x = 0;
					y = 0;
				} else if ( nLen != 5 || x < 1 || x > ban_size || y < 1 || y > ban_size ) {
					sCommand[nLen] = 0;
					PRT("%s:  移動場所エラー\n",sCommand);
//					nLen = 0;
//					break;
					PRT("不正な場所だが、パスとみなして続行\n");
					x = 0; y = 0;
				}
				z = y * 256 + x;
				tesuu++;	all_tesuu = tesuu;
				kifu[tesuu][0] = z;
				kifu[tesuu][1] = 2 - (sCommand[0] == 'B');	// 石の色
				kifu[tesuu][2] = 0;	// 思考時間、この後あれば設定される。
				sCommand[nLen] = 0;
//				PRT("%s, z=%04x: ",sCommand,z);
				nLen = 0;
			}

			if ( nLen >= 255 ) {
				PRT("データの内容が255文字以上。ちと長すぎ\n");
				break;
			}
		}
	}

//	for (i=0;i<2;i++) {	if ( strstr( sPlayerName[i], "Dai") != 0 ) sprintf(sPlayerName[i],"GnuGo3.5.4");}
//	komi = 6;

	return 0;
}

// SGFのバッファに書き出す
void SGF_OUT(char *fmt, ...)
{
	va_list ap;
	int n;
	char text[TMP_BUF_LEN];

	va_start(ap, fmt);
	vsprintf( text , fmt, ap );
	va_end(ap);

	n = strlen(text);
	if ( nSgfBufNum + n >= SGF_BUF_MAX ) { PRT("SgfBuf over\n"); return; }

	strncpy(SgfBuf+nSgfBufNum,text,n);
	nSgfBufNum += n;
	SgfBuf[nSgfBufNum] = 0;
	nSgfBufSize = nSgfBufNum;	// 一応代入
}

// SGFを書き出す
void SaveSGF(void)
{
	int i,j,x,y,z,m0,m1,s0,s1;
	int fSgfTime,fAdd;
	time_t timer;
	struct tm *tb;

	nSgfBufNum = 0;

	// 時刻の取得
	timer = time(NULL);
	tb = localtime(&timer);   /* 日付／時刻を構造体に変換する */

	SGF_OUT("(;\r\n"); 
	SGF_OUT("GM[1]SZ[%d]\r\n",ban_size); 
	SGF_OUT("PB[%s]\r\n",sPlayerName[0]); 
	SGF_OUT("PW[%s]\r\n",sPlayerName[1]); 
	if ( sDate[0] ) SGF_OUT("DT[%s]\r\n",sDate);
	else            SGF_OUT("DT[%4d-%02d-%02d]\r\n",tb->tm_year+1900,tb->tm_mon+1,tb->tm_mday); 
//	SGF_OUT("DT[%4d-%02d-%02d %02d:%02d:%02d]\r\n",year, month, day, hour, minute, second); 
	double score = LatestScore - komi;
	if ( score > 0 ) SGF_OUT("RE[B+%.1f]\r\n", score);
	else             SGF_OUT("RE[W+%.1f]\r\n",-score);

	if ( nHandicap ) SGF_OUT("HA[%d]",nHandicap); 
	SGF_OUT("KM[%.1f]TM[]",komi); 
	if ( fChineseRule ) SGF_OUT("RU[Chinese]"); 
	else                SGF_OUT("RU[Japanese]"); 
	SGF_OUT("PC[%s]",sPlace); 
	SGF_OUT("EV[%s]",sEvent); 
	SGF_OUT("GN[%s]",sRound); 
	m0 = total_time[0] / 60;		
	s0 = total_time[0] -(m0 * 60);
	m1 = total_time[1] / 60;
	s1 = total_time[1] -(m1 * 60);
	SGF_OUT("AP[%s]\r\n",sVersion);
	fSgfTime = 0;
	for ( i=1;i<tesuu+1; i++ ) if ( kifu[i][2] ) fSgfTime = 1;
	if ( fSgfTime ) {
		SGF_OUT("C[Time %d:%02d,%d:%02d]\r\n",m0,s0,m1,s1); 
		SGF_OUT("C[%s]\r\n",sYourCPU);
		SGF_OUT("C['T' means thinking time per a move.]\r\n");
	}
//	SGF_OUT("WS[0]",sEvent);
//	SGF_OUT("BS[7]",sEvent);
//	SGF_OUT("NS[1]",sEvent);

	// 盤面だけを書き込む
	fAdd = 0;
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = (y+1)*256+x+1;
		if ( ban_start[z] != 1 ) continue;
		if ( fAdd == 0 ) { SGF_OUT("AB"); fAdd = 1; }
		SGF_OUT("[%c%c]",x+'a', y+'a' );
	}
	if ( fAdd ) SGF_OUT("\r\n");
	fAdd = 0;
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = (y+1)*256+x+1;
		if ( ban_start[z] != 2 ) continue;
		if ( fAdd == 0 ) { SGF_OUT("AW"); fAdd = 1; }
		SGF_OUT("[%c%c]",x+'a', y+'a' );
	}
	if ( fAdd ) SGF_OUT("\r\n");
	// 無理矢理2回パス
//	SGF_OUT("\nB[tt]W[tt]\n"); WriteFile( hFile,(void *)lpSend, strlen(lpSend), &RetByte, NULL );


	j = 0;
	for ( i=1;i<tesuu+1; i++ ) {
		z = kifu[i][0];
		if ( z != 0 ) {
			x = z & 0x00ff;
			y = (z & 0xff00) >> 8;
		} else {
			x = 20;
			y = 20;
		}
		if ( kifu[i][1] == 1 ) SGF_OUT(";B[%c%c]",x+'a'-1, y+'a'-1 );
		else                   SGF_OUT(";W[%c%c]",x+'a'-1, y+'a'-1 );
		// 棋譜に消費時間を保存。独自拡張。
		if ( fSgfTime ) { 
			SGF_OUT("T[%2d]",kifu[i][2]);
		}

		j++;
		if ( j==10 - 4*fSgfTime ) {	// 10手ごとに改行する。
			SGF_OUT("\r\n");
			j = 0;
		}
	}
	SGF_OUT(")\r\n");
}

// ig1ファイルを読む
int ReadIG1(void)
{
	int i,x=0,y,yl,yh=0,z, xFlag,yFlag=0, nLen;
	int RetByte;		// 読み込まれたバイト数
	char c;
	char  lpLine[256];	// １分を読む
	
	nSgfBufNum = 0;
	
	for ( i=0;i<255;i++) {
		sPlayerName[0][i] = 0;
		sPlayerName[1][i] = 0;
	}

	// 一行をロードする
	// "INT" が見つかるまでロード
	for ( ;; ) {
		nLen = ReadOneLine( lpLine );
		if ( nLen == 0 ) {
			PRT("EOF - 読み込み失敗。MOV が見つからない\n");
			break;
		}
		lpLine[nLen] = 0;
		PRT(lpLine);
		if ( lpLine[0] == 'M' && lpLine[1] == 'O' && lpLine[2] == 'V' ) break;
		if ( strstr( lpLine, "PLAYER_BLACK" ) ) strncpy( sPlayerName[0], &lpLine[13], nLen-13-1);
		if ( strstr( lpLine, "PLAYER_WHITE" ) ) strncpy( sPlayerName[1], &lpLine[13], nLen-13-1);
		if ( strstr( lpLine, "黒：" ) ) strncpy( sPlayerName[0], &lpLine[4], nLen-4-1);
		if ( strstr( lpLine, "白：" ) ) strncpy( sPlayerName[1], &lpLine[4], nLen-4-1);
	}

	if ( nLen ) {	
		tesuu = 0;
		xFlag = 1;
		for ( ;; ) {
			c = GetSgfBuf( &RetByte );
			if ( RetByte == 0 ) break;	// EOF;
			if ( c == '\r' || c == '\n' ) continue;	// 改行コードはパス
			if ( c <= 0x20 ) continue;	// 空白以下のコード（TAB含む）は区切り
			if ( c == '-' ) break;
			if ( xFlag == 1 && 'A' <= c && c <= 'T'	) {
				x = c  - 'A' - (c > 'I') + 1;	// 'I'を超えたら-1	
				xFlag = 0;
				yFlag = 1;
			}
			else if ( yFlag == 1 && '0'	<= c && c <= '9' ) {
				yh = c - '0';
				yFlag = 2;
			}
			else if ( yFlag == 2 && '0'	<= c && c <= '9' ) {
				yl = c - '0';
				yFlag = 1;
				xFlag = 1;
				y = 10*yh + yl;
				if ( x==1 && y==0 ) z = 0;
				else z = ( y<<8 ) + x;
				tesuu++;	all_tesuu = tesuu;
				kifu[tesuu][0] = z;
				kifu[tesuu][1] = 2 - ( tesuu & 1 );	// 先手が黒
			}
		}
	}
	return 0;
}

// ig1ファイルを書き出す
void SaveIG1(void)
{
	int i,j,x,y,z;
	time_t timer;
	struct tm *tb;

	nSgfBufNum = 0;

	// 時刻の取得
	timer = time(NULL);
	tb = localtime(&timer);   /* 日付／時刻を構造体に変換する */

	// \n だけだと 0x0a のみが入り好ましくないので 0x0d(\r)を追加 
	SGF_OUT("#COMMENT\r\n");
	SGF_OUT("#--- CgfGoBan 棋譜ファイル（GOTERMと互換性あり）---\r\n");
	SGF_OUT("#対局日 %4d-%02d-%02d\r\n",tb->tm_year+1900,tb->tm_mon+1,tb->tm_mday); 
	SGF_OUT("PLAYER_BLACK %s\r\n",sPlayerName[0]);
	SGF_OUT("PLAYER_WHITE %s\r\n",sPlayerName[1]);

	double score = LatestScore - komi;
	if ( score > 0 ) SGF_OUT("RESULT B+%.1f\r\n", score);
	else             SGF_OUT("RESULT W+%.1f\r\n",-score);
//	SGF_OUT("PLACE %s\r\n",sPlace);
//	SGF_OUT("EVENT %s\r\n",sEvent);
//	SGF_OUT("GAME_NAME Round %s\r\n",sRound);

	SGF_OUT("INT %d 0\r\n",ban_size);
	SGF_OUT("MOV\r\n");

	j = 0;
	for ( i=1;i<tesuu+1; i++ ) {
		z = kifu[i][0];
		if ( z != 0 ) {
			x = z & 0x00ff;
			y = (z & 0xff00) >> 8;
		} else {
			x = 1;
			y = 0;
		}
		SGF_OUT(" %c%02d",x+64+(x>8),y );
 		j++;
		if ( j==19 ) {
			SGF_OUT("\r\n");
			j = 0;
		}
	}
	SGF_OUT(" -\r\n");
}

// SGFをロードして盤面を構成する
void LoadSetSGF(int fSgf)
{
	int i;

	tesuu = all_tesuu = 0;
	nHandicap = 0;

	// 盤面を初期化 ---> 置き碁のため
	for (i=0;i<BANMAX;i++) {
		ban_start[i] = ban[i];	// 開始局面を設定。初期状態は全て０
		if ( ban_start[i] != 3 ) ban_start[i] = 0;
	}

	if ( fSgf ) ReadSGF();	// SGFを読み込む
	else        ReadIG1();	// ig1ファイルを読む

#ifdef CGF_E
	PRT("Move Numbers=%d\n",tesuu);
#else
	PRT("読み込んだ手数=%d\n",tesuu);
#endif
	// 盤面を初期盤面に
	for (i=0;i<BANMAX;i++) ban[i] = ban_start[i];
//	PRT("ban_size=%d\n",ban_size);
	reconstruct_ban();
	tesuu = all_tesuu;

	for (i=1; i<=tesuu; i++) {
		int z   = kifu[i][0];
		int col = kifu[i][1];
		if ( make_move(z,col) != MOVE_SUCCESS ) {
			PRT("棋譜読込エラー z=%x,col=%d,tesuu=%d \n",z,col,i);
			break;
		}
	}

	PaintUpdate();
}

/*** printf の代用 ***/
void PRT(char *fmt, ...)
{
	va_list ap;
	int i,len,x,y,nRet = 1;
//	char c,text[256],sOut[MAX_PATH];
	char c,text[TMP_BUF_LEN*16],sOut[MAX_PATH];
	static FILE *fp;
	RECT rc;
	HDC hDC;

	va_start(ap, fmt);
	vsprintf( text , fmt, ap );

	//// FILE書出用 ////
	if ( fFileWrite == 1 ) {
		strcpy(sOut,sCgfDir);
		strcat(sOut,"\\output");	// 吐き出すファイル名
		fp = fopen(sOut,"a");
		fprintf(fp,text);
		fclose(fp);
	}
	////////////////////

//	PRT_Scroll();

	len = (int)strlen(text);
	for (i=0;   i<len; i++) {
		c = text[i];
		if ( c == '\n' ) {nRet++;PRT_Scroll();}
		else {
			cPrintBuff[PMaxY-1][PRT_x] = c;
			if ( PRT_x == PRetX - 1 ) {nRet++;PRT_Scroll();}	// 一列は100行
			else PRT_x++;
		}
	}
	if ( hPrint != NULL && IsWindow(hPrint) ) {
		if ( nRet == 1 ) {	// スクロールしない場合を高速描画
//			rc.left  = (PRT_x-len)*nWidth;
//			rc.right =  PRT_x     *nWidth;
			hDC = GetDC(hPrint);
			x = PRT_x - len;
			y = PMaxY -1;
			TextOut(hDC,x*nWidth,(y-PRT_y)*(nHeight),&cPrintBuff[y][x],strlen(&cPrintBuff[y][x]));
			ReleaseDC(hPrint,hDC);
		} else {
			ScrollWindow( hPrint, 0, -nHeight*(nRet-1), NULL, NULL );
			GetClientRect( hPrint, &rc);
			rc.top    = (PMaxY-PRT_y - nRet)*nHeight;
			rc.bottom = (PMaxY-PRT_y       )*nHeight;
			InvalidateRect(hPrint, &rc, TRUE);
			UpdateWindow(hPrint);
		}
	}
//	MessageBox( ghWindow, text, "Printf Debug", MB_OK);
	va_end(ap);
}

void PRT_Scroll(void)
{
	int i,j;

	for (i=1;i<PMaxY;i++) {
		for (j=0;j<PRetX;j++) cPrintBuff[i-1][j] = cPrintBuff[i][j];
	}
	for (i=0; i<PRetX; i++) cPrintBuff[PMaxY-1][i] = 0;
	PRT_x = 0;
}

// PRTの内容をクリップボードにコピー
void PRT_to_clipboard(void)
{
	int i;
	HGLOBAL hMem;
	LPSTR lpStr;
	char sTmp[TMP_BUF_LEN];

	hMem = GlobalAlloc(GHND, PMaxY*PMaxX);
	lpStr = (LPSTR)GlobalLock(hMem);

	lstrcpy( lpStr, "");

	for (i=0;i<PMaxY;i++) {
		if ( strlen(cPrintBuff[i]) == 0 ) continue;
		sprintf(sTmp,"%s\n",cPrintBuff[i]);
		lstrcat(lpStr,sTmp);
	}

	GlobalUnlock(hMem);
	if ( OpenClipboard(ghWindow) ) {
		EmptyClipboard();
		SetClipboardData( CF_TEXT, hMem );
		CloseClipboard();
//		PRT("クリップボードにPRTを出力しました\n");
	}

}


/*** 囲碁盤の初期座標値を取得（囲碁盤自体の縮小に備える）***/
void GetScreenParam( HWND hWnd,int *dx, int *sx,int *sy, int *db )
{
	RECT rc;
	
	GetClientRect( hWnd, &rc );

	*sx = 22;			// 囲碁盤の左端ｘ座標	 
	*sy = 20;

	*dx = 2*(rc.bottom - *sy -15 ) / ( 2*ban_size+1);	// -15 は気分
		
	if ( *dx <= 10 ) *dx = 10;	// 最低は10
	if ( *dx & 1 ) *dx -= 1;	// 偶数に
//	if ( !(*dx & 2) ) *dx -= 2;	// 18 + 4*x の倍数に

	*db = (*dx*3) / 4;	// 囲碁盤の端と線の間隔
	if ( !(*db & 1) ) *db += 1;	// 奇数に
//	if ( iBanSize==BAN_19 ) *dx = 26;
//	if ( iBanSize==BAN_13 ) *dx = 38;
//	if ( iBanSize==BAN_9  ) *dx = 50;
//	*dx = 26;		// 升目の間隔 (640*480..20, 800*600..25, 1280*780..34)
					// 18 <= dx <= 36 (24 +- 4 の倍数は暗い盤で枠がにじむ)
}

/*** WM_PAINT で画面を再描画する関数 ***/

char *info_str[] = {
#ifdef CGF_E
	"B", 
	"W",
	"Hama: %d",
	"N= %3d",
	"Black Turn",
	"White Turn",
	"Draw",
	"%s %.1f Win",
	"(Chinese)",
	"Black", 
	"White",
#else
	"黒", 
	"白",
	"揚浜：%d",
	"第%3d手",
	"黒の番です",
	"白の番です",
	"持碁",
	"%sの %.1f 目勝",
	"(中国)",
	"黒", 
	"白",
#endif
};

#ifdef CGF_E
char FontName[] = "FixedSys";	// "MS UI Gothic" "MS Sans Serif" "Courier New" "BatangChe"(win98では見にくい)
#else
char FontName[] = "標準明朝";
#endif

// 盤上に数値を書き込む
void DrawValueNumber(HDC hDC,int size, int col, int xx, int yy, char *sNum)
{
	TEXTMETRIC tm;
	HFONT hFont,hFontOld;
	int n = strlen(sNum);

	hFont = CreateFont(					/* フォントの作成 */
		size,0,0,0,FW_NORMAL,
		0,0,0,	DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
           DEFAULT_PITCH, FontName);
	hFontOld = (HFONT)SelectObject(hDC,hFont);
	GetTextMetrics(hDC, &tm);
	if ( col==2 ) SetTextColor( hDC, rgbBlack );	// 黒で
	else          SetTextColor( hDC, rgbWhite );	// 白で
	TextOut( hDC, xx-tm.tmAveCharWidth*n/2 - (n==3)*1, yy-size/2, sNum ,n);
	SelectObject( hDC, hFontOld );
	DeleteObject(hFont);
}

// 実際に描画を行う
void PaintScreen( HWND hWnd, HDC hDC )
{
	HBRUSH hBrush,hBrushWhite,hBrushBlack;
	HPEN hPen,hPenOld;
	HFONT hFont,hFontOld;
	RECT rRect;
	
	int dx,sx,sy,db;
	int x,y,z,xx,yy,i,j,k,n,size;
	char sNum[TMP_BUF_LEN];

	GetScreenParam(	hWnd,&dx,&sx,&sy,&db );

	hBrushBlack = CreateSolidBrush( rgbBlack );	// 黒石
	hBrushWhite = CreateSolidBrush( rgbWhite );	// 白石

	SetTextColor( hDC, rgbText );	//　文字色
//	SetBkColor( hDC, RGB(0,128,0) );		//　背景を濃い緑に

	// 背景をまず描画する。
	RECT rcFull;
	GetClientRect( hWnd, &rcFull );	// 現在の画面の大きさを取得。グラフの大きさをこのサイズに合わせる
	hBrush = CreateSolidBrush(rgbBlack);
	FillRect(hDC, &rcFull, hBrush );
	DeleteObject(hBrush);

	/* 盤面全体の表示 */
	SetRect( &rRect , sx, sy, sx+dx*(ban_size-1)+db*2, sy+dx*(ban_size-1)+db*2 );
	n = ID_NowBanColor - ID_BAN_1;
//	PRT("n=%d\n",n);
	if ( n >= RGB_BOARD_MAX || n < 0 ) { PRT("色がない n=%d\n",n); debug(); }
	hBrush = CreateSolidBrush( rgbBoard[n] );
	FillRect( hDC, &rRect, hBrush );
	DeleteObject(hBrush);
			
   	hPen = CreatePen(PS_SOLID, 1, rgbBlack );
	hPenOld = (HPEN)SelectObject( hDC, hPen );
	for (x=1;x<ban_size-1;x++) {
		MoveToEx( hDC, x*dx+sx+db , sy+db, NULL );
        LineTo( hDC, x*dx+sx+db , sy+db+dx*(ban_size-1) );
		MoveToEx( hDC, sx+db      , x*dx+sy+db, NULL );
        LineTo( hDC, sx+db+dx*(ban_size-1) , x*dx+sy+db );
	}
	SelectObject( hDC, hPenOld );
	DeleteObject(hPen);

   	hPen = CreatePen(PS_SOLID, 2, rgbBlack );
	SelectObject( hDC, hPen );
	MoveToEx( hDC, sx+db , sy+db, NULL );
	LineTo( hDC, sx+db , sy+db+dx*(ban_size-1) );
	LineTo( hDC, sx+db+dx*(ban_size-1), sy+db+dx*(ban_size-1) );
	LineTo( hDC, sx+db+dx*(ban_size-1), sy+db );
	LineTo( hDC, sx+db, sy+db );
	SelectObject( hDC, hPenOld );
	DeleteObject(hPen);

	/* 星の点 */
	SelectObject( hDC, hBrushBlack );
	if ( ban_size == BAN_19 ) {
    	for ( i=0;i<3;i++) {
    		x =  sx+db+dx*3 + dx*6*i;
    		for ( j=0;j<3;j++) {
		    	y =  sy+db+dx*3 + dx*6*j;
		    	Ellipse( hDC, x-2,y-2, x+3,y+3);
			}
		}
	} else if (ban_size == BAN_13 ) {
   		x =  sx+db+dx*6;
    	y =  sy+db+dx*6;
    	Ellipse( hDC, x-2,y-2, x+3,y+3);
    	for ( i=0;i<2;i++) {
    		x =  sx+db+dx*3 + dx*6*i;
    		for ( j=0;j<2;j++) {
		    	y =  sy+db+dx*3 + dx*6*j;
		    	Ellipse( hDC, x-2,y-2, x+3,y+3);
			}
		}
	} else {
   		if ( ban_size & 1 ) {
			n = ban_size / 2;
			x =  sx+db+dx*n;
    		y =  sy+db+dx*n;
    		Ellipse( hDC, x-2,y-2, x+3,y+3);
		}
	}

	SetBkMode( hDC, TRANSPARENT);	// TRANSPARENT ... 重ねる, OPAQUE...塗りつぶし

	// 題字「CGF碁盤」の表示
	x = sx+db+dx*(ban_size)+8;
	y = 15;

#ifdef CGF_E
	hFont = CreateFont( 36,0, 0, 0, FW_BOLD, 0, 0, 0,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT, "Comic Sans MS");
//		DEFAULT_PITCH | FF_SCRIPT, "Gungsuh");
	// "Arial","Tahoma","Times New Roman","MS UI Gothic","Mattise ITC","Lucida Sans","Lucida Handwriting"
	// Arial Black,Arial Narrow,Batang,Century,Comic Sans MS,Georgia,Gulim,Gungsuh,GungsuhChe,Impact
	x += 5;
	y += 5;
#else
	hFont = CreateFont( 36,0, 0, 0, FW_BOLD, 0, 0, 0,
		SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT, "標準ゴシック");
	y += 10;
#endif
	hFontOld = (HFONT)SelectObject( hDC, hFont );
	SetTextColor( hDC, rgbBlack );		// 影を黒で
	n = strlen(szTitleName);
	TextOut( hDC, x+4, y+4, szTitleName,n );
	SetTextColor( hDC, rgbWhite );	//　文字を白に
	TextOut( hDC, x-1, y-1, szTitleName,n );
	TextOut( hDC, x+1, y-1, szTitleName,n );
	TextOut( hDC, x-1, y+1, szTitleName,n );
	TextOut( hDC, x+1, y+1, szTitleName,n );
	SetTextColor( hDC, RGB(128,0,128) );
	TextOut( hDC,   x,   y, szTitleName,n );
	x += 7;
#ifdef CGF_E
	x -= 5;
#endif
   	hPen = CreatePen(PS_SOLID, 2, RGB( 255,255,0 ) );
	hPenOld = (HPEN)SelectObject( hDC, hPen );
	MoveToEx( hDC, x -10, 70, NULL );
	LineTo(   hDC, x+125, 70 );
	MoveToEx( hDC, x -10, 15, NULL );
	LineTo(   hDC, x+125, 15 );
	SelectObject( hDC, hPenOld );
	DeleteObject(hPen);
	SelectObject( hDC, hFontOld );
	DeleteObject(hFont);


	/* 数字 */
	SetTextColor( hDC, rgbText );
	if ( dx > 16 ) {
		k = 18;
		n = 5;
		xx = 0;
		yy = 0;
	} else {
		k = 12;
		n = 2;
		xx = 4;
		yy = 4;
	}
	
	hFont = CreateFont( k,0, 0, 0, FW_NORMAL, 0, 0, 0,
		SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT, FontName);
	hFontOld = (HFONT)SelectObject( hDC, hFont );
	for (i=0;i<ban_size;i++) {
		if ( fATView ) {
			// チェス式座標系
			sprintf( sNum,"%d",ban_size-i);	
			TextOut( hDC, xx + (ban_size-i<=9)*n , sy+db-8 + i*dx, sNum ,strlen(sNum) );
			sprintf( sNum,"%c",i+'A'+(i>7) );	// A-T 表記
			TextOut( hDC, sx+db-4 + i*dx, 0, sNum ,strlen(sNum) );
		} else {
#ifdef CGF_DEV
			sprintf( sNum,"%x",i+1);
			TextOut( hDC, xx + (i<15)*n , yy + sy+db-8 + i*dx, sNum ,strlen(sNum) );
			sprintf( sNum,"%x",i+1 );
			TextOut( hDC, sx+db-4 + i*dx - (i>14)*n, yy, sNum ,strlen(sNum) );
#else
			sprintf( sNum,"%d",i+1);
			TextOut( hDC, xx + (i<9)*n , sy+db-8 + i*dx, sNum ,strlen(sNum) );
			sprintf( sNum,"%d",i+1 );
			TextOut( hDC, sx+db-4 + i*dx - (i>8)*n, yy, sNum ,strlen(sNum) );
#endif
		}
	}
	SelectObject( hDC, hFontOld );
	DeleteObject(hFont);

	/* 盤面にある石を表示 */
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + (x+1);
	   	xx = sx+db+dx*x;
	   	yy = sy+db+dx*y;
		k = ban[z];
		if ( k == 0 ) continue;
		if ( k == 1 ) SelectObject( hDC, hBrushBlack );		// 黒石
		else          SelectObject( hDC, hBrushWhite );		// 白石

//	   	Ellipse( hDC, xx-dx/2,yy-dx/2, xx+dx/2+1,yy+dx/2+1);		// ディスプレイドライバによる？
	   	Ellipse( hDC, xx-dx/2,yy-dx/2, xx+dx/2,  yy+dx/2  );		// Windows98 ViperV550 ではこちら。
//		Chord( hDC,  xx-dx/2,yy-dx/2, xx+dx/2,yy+dx/2, xx-dx/2,yy-dx/2, xx-dx/2,yy-dx/2 );	// Ellipse は動作がおかしい（はみ出る）Win98 ViperV550では描画されないことあり。

		if ( kifu[tesuu][0] == z ) {	// 直前に打たれた石に手数を表示
			sprintf( sNum,"%d",tesuu);
			n = strlen(sNum);
			if ( n==1 ) size = dx*3/4;
			else if ( n==2 ) size = dx*7/10;
			else size = dx*7/12;
			DrawValueNumber(hDC, size, k, xx, yy, sNum);
		}
	}

	
	// 死活を表示
	if ( endgame_board_type == ENDGAME_STATUS ) {
		for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
			z = ((y+1)<<8) + (x+1);
	   		xx = sx+db+dx*x;
	   		yy = sy+db+dx*y;
			
			k = ban[z];
			n = endgame_board[z];
			if ( k == 0 ) {
				// 確定地を表示
				hBrush = NULL;
				if ( n == GTP_WHITE_TERRITORY ) hBrush = hBrushWhite;
				if ( n == GTP_BLACK_TERRITORY ) hBrush = hBrushBlack;
				if ( hBrush == NULL ) continue;
				SelectObject( hDC, hBrush );
				Rectangle( hDC, xx-dx/4,yy-dx/4, xx+dx/4,yy+dx/4);
				continue;
			}
			if ( n != GTP_DEAD ) continue;

		   	// 死んでいる石に×をつける
			if ( k == 1 ) hPen = CreatePen(PS_SOLID, 2, rgbWhite );
			else          hPen = CreatePen(PS_SOLID, 2, rgbBlack );
			hPenOld = (HPEN)SelectObject( hDC, hPen );
			MoveToEx( hDC, xx-dx/6,yy-dx/6, NULL );
			LineTo(   hDC, xx+dx/6,yy+dx/6 );
			MoveToEx( hDC, xx-dx/6,yy+dx/6 , NULL );
			LineTo(   hDC, xx+dx/6,yy-dx/6 );
			SelectObject( hDC, hPenOld );
			DeleteObject(hPen);
		}
	}
	// 数値を表示
	if ( endgame_board_type == ENDGAME_DRAW_NUMBER ) {
		for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
			z = ((y+1)<<8) + (x+1);
	   		xx = sx+db+dx*x;
	   		yy = sy+db+dx*y;
			k = ban[z];
			n = endgame_board[z];
			if ( n == 0 ) continue;

			sprintf( sNum,"%3d",n);
			n = strlen(sNum);
			size = dx*7/12;
			if ( n == 4 ) size = dx*7/16; 
			if ( n >= 5 ) size = dx*7/20; 
			DrawValueNumber(hDC, size, k, xx, yy, sNum);
		}
	}
	// 図形を表示
	if ( endgame_board_type == ENDGAME_DRAW_FIGURE ) {
		for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
			int m,d;
			z = ((y+1)<<8) + (x+1);
	   		xx = sx+db+dx*x;
	   		yy = sy+db+dx*y;
//			k = ban[z];
			n = endgame_board[z];
			
			m = n & ~(FIGURE_BLACK | FIGURE_WHITE);	// 色指定を消す
			d = 2;	// ペンの太さ
			if ( m == FIGURE_TRIANGLE || m == FIGURE_SQUARE	|| m == FIGURE_CIRCLE ) d = 1;
			if ( n & FIGURE_WHITE ) hPen = CreatePen(PS_SOLID, d, rgbWhite );
			else                    hPen = CreatePen(PS_SOLID, d, rgbBlack );
			hPenOld = (HPEN)SelectObject( hDC, hPen );

			if ( n & FIGURE_WHITE ) hBrush = hBrushWhite;
			else                    hBrush = hBrushBlack;
			SelectObject( hDC, hBrush );


			if ( m == FIGURE_TRIANGLE ) {	// 三角形
				int x1 = dx /  3;
				int y1 = dx /  3;
				int y2 = dx /  5;
				MoveToEx( hDC, xx   ,yy-y1, NULL );
				LineTo(   hDC, xx+x1,yy+y2 );
				LineTo(   hDC, xx-x1,yy+y2 );
				LineTo(   hDC, xx   ,yy-y1 );
			}
			if ( m == FIGURE_SQUARE ) {		// 四角
				int x1 = dx / 4;
				Rectangle( hDC, xx-x1,yy-x1, xx+x1,yy+x1);
			}
			if ( m == FIGURE_CIRCLE ) {		// 円
				int x1 = dx / 5;
				Chord( hDC, xx-x1,yy-x1, xx+x1,yy+x1, xx-x1,yy-x1, xx-x1,yy-x1 );
			}
			if ( m == FIGURE_CROSS ) {		// ×
				int x1 = dx / 6;
				MoveToEx( hDC, xx-x1,yy-x1, NULL );
				LineTo(   hDC, xx+x1,yy+x1 );
				MoveToEx( hDC, xx-x1,yy+x1, NULL );
				LineTo(   hDC, xx+x1,yy-x1 );
			}
			if ( m == FIGURE_QUESTION ) {	// "？"の記号	
				sprintf( sNum,"?" );
				size = dx*10/12;
				if ( n & FIGURE_BLACK ) k = 2;
				else                    k = 1;
				DrawValueNumber(hDC, size, k, xx, yy, sNum);
			}
			if ( m == FIGURE_HORIZON ) {	// 横線
				int x1 = dx * 10 / 22;
				MoveToEx( hDC, xx-x1,yy, NULL );
				LineTo(   hDC, xx+x1,yy );
			}
			if ( m == FIGURE_VERTICAL ) {	// 縦線
				int x1 = dx * 10 / 22;
				MoveToEx( hDC, xx,yy-x1, NULL );
				LineTo(   hDC, xx,yy+x1 );
			}
			if ( m == FIGURE_LINE_LEFTUP ) {	// 斜め、左上から右下
				int x1 = dx * 10 / 22;
				MoveToEx( hDC, xx-x1,yy-x1, NULL );
				LineTo(   hDC, xx+x1,yy+x1 );
			}
			if ( m == FIGURE_LINE_RIGHTUP ) {	// 斜め、左下から右上
				int x1 = dx * 10 / 22;
				MoveToEx( hDC, xx-x1,yy+x1, NULL );
				LineTo(   hDC, xx+x1,yy-x1 );
			}

			SelectObject( hDC, hPenOld );
			DeleteObject(hPen);
		}
	}



	SelectObject( hDC, hBrushBlack );		// 黒石
	xx = sx+db+dx*(ban_size)+ban_size;
   	yy =  135;
   	hPen = CreatePen(PS_SOLID, 1, rgbWhite );
	hPenOld = (HPEN)SelectObject( hDC, hPen );
#ifdef CGF_E
	n = 12;
#else
	n = 13;
#endif
	Chord( hDC,  xx-n,yy-n, xx+n,yy+n, xx-n,yy-n,xx-n,yy-n );
	SelectObject( hDC, hPenOld );
	DeleteObject(hPen);
    SelectObject( hDC, hBrushWhite );		// 白石
   	yy =  225;
	Chord( hDC,  xx-n,yy-n, xx+n,yy+n, xx-n,yy-n,xx-n,yy-n );	// Ellipse は動作がおかしい（はみ出る）
// 	Ellipse( hDC, xx-n,yy-n, xx+n,yy+n);


	/* その他の画面情報 */
	hFont = CreateFont( 16,0, 0, 0, FW_NORMAL, 0, 0, 0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT, FontName);
	hFontOld = (HFONT)SelectObject( hDC, hFont );
	SetTextColor( hDC, rgbText );	// 文字色
#ifdef CGF_E
	n = 3;
#else
	n = 0;
#endif
	sprintf( sNum,info_str[0]);
	TextOut( hDC, xx-8+n,128, sNum ,strlen(sNum) );
	sprintf( sNum,"%s",sPlayerName[0]);
	TextOut( hDC, xx+20,128, sNum ,strlen(sNum) );

	SetTextColor( hDC, rgbBlack );	// 文字を黒に
	sprintf( sNum,info_str[1]);
	TextOut( hDC, xx-8+n,218, sNum ,strlen(sNum) );
	SetTextColor( hDC, rgbWhite );	// 文字を白に
	sprintf( sNum,"%s",sPlayerName[1]);
	TextOut( hDC, xx+20,218, sNum ,strlen(sNum) );

	sprintf( sNum,info_str[2],hama[0]);
	TextOut( hDC, xx-16,152, sNum ,strlen(sNum) );
	sprintf( sNum,info_str[2],hama[1]);
	TextOut( hDC, xx-16,242, sNum ,strlen(sNum) );
	
	sprintf( sNum,info_str[3],tesuu);
	TextOut( hDC, xx-16,300, sNum ,strlen(sNum) );
	k = kifu[tesuu][0];
	if ( k==0 ) {
		sprintf( sNum,"PASS");
	} else {
		if ( fATView ) {
			sprintf( sNum,"%c -%2d",'A'+(k&0xff)+((k&0xff)>8)-1,ban_size-(k>>8)+1);
		} else {
			sprintf( sNum,"(%2d,%2d)",k&0xff,k>>8);
		}
	}
	TextOut( hDC, xx-16+8*8,300, sNum ,strlen(sNum) );

	if ( IsBlackTurn() ) sprintf( sNum,info_str[4]);
	else                 sprintf( sNum,info_str[5]);
	if ( fNowPlaying ) TextOut( hDC, xx-16,340, sNum ,strlen(sNum) );

	if ( endgame_board_type == ENDGAME_STATUS ) {
		endgame_score_str(sNum,0);
		TextOut( hDC, xx-16,390, sNum ,strlen(sNum) );

		sprintf( sNum,"Komi=%.1f",komi);
		TextOut( hDC, xx-16,430, sNum ,strlen(sNum) );
		if ( fChineseRule ) {
			// 中国ルールでカウントしてみる。
			// アゲハマは無視。盤上に残っている石の数と地の合計。
			endgame_score_str(sNum,1);
			TextOut( hDC, xx-16,450, sNum ,strlen(sNum) );
		}
	}

	// 思考時間の表示
	count_total_time();

	for (i=0;i<2;i++) {
		int m,s,mm,ss;

		m = total_time[i] / 60;
		s = total_time[i] -(m * 60);
		     if ( tesuu == 0    ) n = 0;
		else if ( (tesuu+i) & 1 ) n = tesuu;
		else                      n = tesuu - 1;
		mm = kifu[n][2] / 60;
		ss = kifu[n][2] - (mm * 60);
		sprintf( sNum,"%02d:%02d / %02d:%02d",mm,ss,m,s);
		int flag = i;
		if ( tesuu > 0 && kifu[1][1]==2 ) flag = 1 - i;	// 白から打っている場合は表示を逆に。
		TextOut( hDC, xx-16,174+90*flag, sNum ,strlen(sNum) );
	}


	SelectObject( hDC, hFontOld );
	DeleteObject(hFont);

	DeleteObject(hBrushWhite);
	DeleteObject(hBrushBlack);
}

// 終局処理をした後で、地の数のみを数える。(黒地 － 白地)
int count_endgame_score(void)
{
	int x,y,z,col,n,sum;
	
	sum = hama[0] - hama[1];
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + (x+1);
		col = ban[z];
		n   = endgame_board[z];
		if ( col == 0 ) {	// 空点
			if ( n == GTP_BLACK_TERRITORY ) sum++;	// 黒地
			if ( n == GTP_WHITE_TERRITORY ) sum--;
		} else {
			if ( n == GTP_DEAD ) {
				if ( col == 1 ) sum -= 2;
				if ( col == 2 ) sum += 2;
			}
		}
	}
	return sum;
}

// 中国ルールで計算（アゲハマは無視し、死石除去後の自分の地と自分の石、の合計を数える。ダメに打つ場合も1目となる）
int count_endgame_score_chinese(void)
{
	int x,y,z,col,n,sum;

	sum = 0;
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + (x+1);
		col = ban[z];
		n   = endgame_board[z];
		if ( col == 0 ) {	// 空点
			if ( n == GTP_BLACK_TERRITORY ) sum++;	// 黒地
			if ( n == GTP_WHITE_TERRITORY ) sum--;
		} else {
			if ( n == GTP_DEAD ) {
				if ( col == 1 ) sum--;
				if ( col == 2 ) sum++;
			} else {
				if ( col == 1 ) sum++;
				if ( col == 2 ) sum--;
			}
		}
	}
	return sum;
}

// 地を数えて結果を文字列で返す
void endgame_score_str(char *str, int flag)
{
	double score;
	
	if ( flag == 0 ) score = count_endgame_score();
	else             score = count_endgame_score_chinese();
	LatestScore = (int)score;
	score -= komi;
	if ( score == 0.0 ) sprintf(str,info_str[6]);
	else {
		char sTmp[TMP_BUF_LEN];
		if ( score > 0 ) {
			sprintf(sTmp,info_str[9]);
		} else {
			sprintf(sTmp,info_str[10]);
			score = -score;
		}
		sprintf(str,info_str[7],sTmp,score);
		if ( flag ) strcat(str,info_str[8]);
//		PRT("score=%.3f\n",score);
	}
}

// 裏画面に書いて一気に転送する。BeginPaintはこっちで行う。
void PaintBackScreen( HWND hWnd )
{
	PAINTSTRUCT ps;
	HDC hDC;
	HDC memDC;
	HBITMAP bmp,old_bmp;
	int maxX,maxY;
	RECT rc;

	hDC = BeginPaint(hWnd, &ps);

	// 仮想最大画面サイズを取得(Desktopのサイズ）SM_CXSCREEN だとマルチディスプレイに未対応。
	GetClientRect(hWnd,&rc);
	maxX = rc.right;
	maxY = rc.bottom;
//	maxX = GetSystemMetrics(SM_CXVIRTUALSCREEN);	// コンパイルが通らない。VC6ではだめ。VC7(.net)ならOK。
//	maxY = GetSystemMetrics(SM_CYVIRTUALSCREEN);
//	PRT("maxX,Y=(%d,%d)\n",maxX,maxY);

	// 仮想画面のビットマップを作成
	memDC   = CreateCompatibleDC(hDC);
	bmp     = CreateCompatibleBitmap(hDC,maxX,maxY);
	old_bmp = (HBITMAP)SelectObject(memDC, bmp);

	PaintScreen( hWnd, memDC );		// 実際に描画を行う関数

	BitBlt(hDC,0,0,maxX,maxY,memDC,0,0, SRCCOPY);	// 裏画面を一気に描画

	// 後始末
	SelectObject(memDC,old_bmp);	// 関連付けられたオブジェクトを元に戻す
	DeleteObject(bmp);				// HDCに関連付けられたGDIオブジェクトは削除できないから
	DeleteDC(memDC);

	EndPaint(hWnd, &ps);
}


/*** マウスの左クリックの時の位置をxy座標に変換して返す ***/
/*** どこも指してないときは０を返す                     ***/
int ChangeXY(LONG lParam)
{
	int dx,sx,sy,db;
	int x,y;
	
	GetScreenParam( hWndDraw,&dx, &sx, &sy, &db );
	x = LOWORD(lParam);
	y = HIWORD(lParam);
	x -= sx + db - dx/2;	// 盤の左隅（のさらに石１つ分の左隅）に平行移動
	y -= sy + db - dx/2;
	if ( x < 0	|| x > dx*ban_size ) return 0;
	if ( y < 0  || y > dx*ban_size ) return 0;
	x = x / dx;
	y = y / dx;
	return ( ((y+1)<<8) + x+1);
}

// 任意の手数の局面に盤面を移動させる
// 前に進む場合は簡単、戻る場合は初手からやり直す。
void move_tesuu(int n)
{
	int i,z,col,loop;

//	PRT("現在の手数=%d, 全手数=%d, 希望の手数=%d ...in \n",tesuu,all_tesuu,n);

	if ( n == tesuu ) return;
	if ( endgame_board_type ) {
		endgame_board_type = 0;
		UpdateLifeDethButton(0);
		PaintUpdate();
	}

	if ( n < tesuu ) {	// 手を戻す。
		if ( n <= 0 ) n = 0;
		loop = n;
		// 盤面を初期盤面に
		for (i=0;i<BANMAX;i++) ban[i] = ban_start[i];
		reconstruct_ban();
	} else {			// 手を進める
		loop = n - tesuu;
		if ( loop + tesuu >= all_tesuu ) loop = all_tesuu - tesuu;	// 最終局面へ。
	}

//	PRT("loop=%d,手数=%d\n",loop,tesuu);
	for (i=0;i<loop;i++) {
		tesuu++;
		z   = kifu[tesuu][0];	// 位置
		col = kifu[tesuu][1];	// 石の色
//		PRT("手を進める z=%x, col=%d\n",z,col);
		if ( make_move(z,col) != MOVE_SUCCESS )	{
			PRT("棋譜再現中にエラー! tesuu=%d, z=%x,col=%d\n",tesuu,z,col);
			break;
		}
	}
//	PRT("現在の手数=%d, 全手数=%d ..end\n",tesuu, all_tesuu);
}

// 現在の手番の色を返す（現在手数と初手が黒番かどうかで判断する）
int GetTurnCol(void)
{
	return 1 + ((tesuu+(nHandicap!=0)) & 1);	// 石の色
}

// 黒の手番か？
int IsBlackTurn(void)
{
	if ( GetTurnCol() == 1 ) return 1;
	return 0;
}

// Gnugoと対局中か？
int IsGnugoBoard(void)
{
	int i;
	for (i=0;i<2;i++) {
		if ( turn_player[i] == PLAYER_GNUGO ) return 1;
	}
	return 0;
}


// 画面の全書き換え
void PaintUpdate(void)
{
	InvalidateRect(hWndDraw, NULL, FALSE);
	UpdateWindow(hWndDraw);
	return;
}

// 盤面色のメニューにチェックを
void MenuColorCheck(HWND hWnd)
{
	HMENU hMenu = GetMenu(hWnd);
	int i;
	
	for (i=0;i<RGB_BOARD_MAX;i++) {
		CheckMenuItem(hMenu, ID_BAN_1 + i, MF_UNCHECKED);
	}
	CheckMenuItem(hMenu, ID_NowBanColor, MF_CHECKED);
}


int ReplayMenu[] = {
	IDM_BACK1,
	IDM_BACK10,
	IDM_BACKFIRST,
	IDM_FORTH1,
	IDM_FORTH10,
	IDM_FORTHEND,

	IDM_PLAY_START,
	IDM_KIFU_OPEN,
	IDM_PASS,
	IDM_BREAK,
	-1
};

// メニューとツールバーのボタンを淡色表示に
void MenuEnableID(int id, int on)
{
	HMENU hMenu = GetMenu(ghWindow);

	EnableMenuItem(hMenu, id, MF_BYCOMMAND | (on ? MF_ENABLED : MF_GRAYED) );
	SendMessage(hWndToolBar, TB_SETSTATE, id, MAKELPARAM(on ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE, 0));
}

// 対局中は棋譜移動メニューを禁止する
void MenuEnablePlayMode(int fLifeDeath)
{
	int i,id,on;

	for (i=0; ;i++) {
		id = ReplayMenu[i];
		if ( id < 0 ) break;
		on = !fNowPlaying;
		if ( id==IDM_PASS || id==IDM_BREAK ) on = !on;
		if ( fLifeDeath ) on = 0;
		MenuEnableID(id, on);
	}
}

// 死活表示中は全てのメニューを禁止する
void UpdateLifeDethButton(int check_on)
{
	MenuEnablePlayMode(check_on);
	UINT iFlags = check_on ? (TBSTATE_PRESSED | TBSTATE_ENABLED) : TBSTATE_ENABLED;
	SendMessage(hWndToolBar, TB_SETSTATE, IDM_DEADSTONE, MAKELPARAM(iFlags, 0));
}


// ToolBarの設定
#define NUMIMAGES       17	// ボタンの数
#define IMAGEWIDTH      16
#define IMAGEHEIGHT     15
#define BUTTONWIDTH      0
#define BUTTONHEIGHT     0

#define IDTBB_PASS       0
#define IDTBB_BACKFIRST  1
#define IDTBB_BACK10     2
#define IDTBB_BACK1      3
#define IDTBB_FORTH1     4
#define IDTBB_FORTH10    5
#define IDTBB_FORTHEND   6
#define IDTBB_DEADSTONE  7
#define IDTBB_DLL_BREAK  8
#define IDTBB_SET_BOARD  9
#define IDTBB_UNDO      10
#define IDTBB_BAN_EDIT  11
#define IDTBB_NEWGAME   12
#define IDTBB_OPEN      13
#define IDTBB_SAVE      14
#define IDTBB_BREAK     15
#define IDTBB_RESTART   16

TBBUTTON tbButton[] =           // Array defining the toolbar buttons
{
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
#ifndef CGF_DEV
    {IDTBB_RESTART,  IDM_PLAY_START,TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
    {IDTBB_OPEN,     IDM_KIFU_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDTBB_SAVE,     IDM_KIFU_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
#endif
    {IDTBB_PASS,     IDM_PASS,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
//  {IDTBB_UNDO,     IDM_UNDO,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
//  {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},

    {IDTBB_BACKFIRST,IDM_BACKFIRST, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},

    {IDTBB_BACK10,   IDM_BACK10,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDTBB_BACK1,    IDM_BACK1,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDTBB_FORTH1,   IDM_FORTH1,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDTBB_FORTH10,  IDM_FORTH10,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDTBB_FORTHEND, IDM_FORTHEND,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},

    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
    {IDTBB_DEADSTONE,IDM_DEADSTONE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},

    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
#ifdef CGF_DEV
    {IDTBB_SET_BOARD,IDM_SET_BOARD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
#endif
    {IDTBB_BREAK,    IDM_BREAK,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
	{0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
	{0, 0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0},
	{IDTBB_DLL_BREAK,IDM_DLL_BREAK, TBSTATE_INDETERMINATE,  TBSTYLE_BUTTON, 0, 0},

};


BOOL CreateTBar(HWND hWnd)
{

	InitCommonControls();	// コモンコントロールのdllを初期化
	hWndToolBar = CreateToolbarEx(hWnd,
                                  WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
                                  IDM_TOOLBAR,
                                  NUMIMAGES,
                                  ghInstance,
                                  IDB_TOOLBAR,
                                  tbButton,
                                  sizeof(tbButton)/sizeof(TBBUTTON),
                                  BUTTONWIDTH,
                                  BUTTONHEIGHT,
                                  IMAGEWIDTH,
                                  IMAGEHEIGHT,
                                  sizeof(TBBUTTON));

    return (hWndToolBar != 0);
}


LRESULT SetTooltipText(HWND /*hWnd*/, LPARAM lParam)
{
    LPTOOLTIPTEXT lpToolTipText;
    static char   szBuffer[64];

    lpToolTipText = (LPTOOLTIPTEXT)lParam;
    if (lpToolTipText->hdr.code == TTN_NEEDTEXT)
    {
        LoadString(ghInstance,
                   lpToolTipText->hdr.idFrom,   // string ID == command ID
                   szBuffer,
                   sizeof(szBuffer));

        lpToolTipText->lpszText = szBuffer;
    }
    return 0;
}

// ツールバーボタンの状態を変更
void UpdateButton(UINT iID, UINT iFlags)
{
    int iCurrentFlags;

    iCurrentFlags = (int)SendMessage(hWndToolBar, 
                                     TB_GETSTATE, 
                                     iID, 0L);

    if (iCurrentFlags & TBSTATE_PRESSED)
        iFlags |= TBSTATE_PRESSED;

    SendMessage(hWndToolBar, 
                TB_SETSTATE, 
                iID, 
                MAKELPARAM(iFlags, 0));
    return;
}


// WinMain()のメッセージループで条件判定に使う
int IsDlgDoneMsg(MSG msg)
{
	if ( hAccel != NULL && TranslateAccelerator(hWndMain, hAccel, &msg)	!= 0 ) return 0;

//	if ( IsDlgMsg(hDlgPntFgr,      msg) == 0 ) return 0;
	return 1;
}

// レジストリの処理
static TCHAR CgfNamesKey[]     = TEXT("Software\\Yss\\cgfgoban");
static TCHAR WindowKeyName[]   = TEXT("WindowPos") ;
static TCHAR PrtWinKeyName[]   = TEXT("PrtWinPos") ;
static TCHAR SoundDropName[]   = TEXT("SoundDrop") ;
static TCHAR BoardColorName[]  = TEXT("BoardColor") ;
static TCHAR InfoWindowName[]  = TEXT("InfoWindow") ;
static TCHAR FileListName[]    = TEXT("Recent File List") ;
static TCHAR BoardSizeName[]   = TEXT("BoardSize") ;
static TCHAR GnugoDirName[]    = TEXT("GnugoDir") ;
static TCHAR GnugoLifeName[]   = TEXT("GnugoLifeDeath") ;
static TCHAR TurnPlayer0Name[] = TEXT("TurnPlayerBlack") ;
static TCHAR TurnPlayer1Name[] = TEXT("TurnPlayerWhite") ;
static TCHAR NameListName[]    = TEXT("Recent Name List") ;
static TCHAR NameListNumName[] = TEXT("NameList") ;
static TCHAR GtpPathName[]     = TEXT("Recent Gtp List") ;
static TCHAR GtpPathNumName[]  = TEXT("GtpList") ;
static TCHAR NngsIPName[]      = TEXT("Recent Nngs List") ;
static TCHAR NngsIPNumName[]   = TEXT("NngsIP") ;
static TCHAR NngsTimeMinutes[] = TEXT("NngsMinutes") ;
static TCHAR ATViewName[]      = TEXT("ATView") ;
static TCHAR AIRyuseiName[]    = TEXT("AIRyusei") ;
static TCHAR ShowConsoleName[] = TEXT("ShowConsole") ;

// レジストリから情報（画面サイズと位置）を読み込む	 flag == 1 ... main window, 2... print window
BOOL LoadMainWindowPlacement (HWND hWnd,int flag)	
{
	WINDOWPLACEMENT   WindowPlacement; 
	TCHAR             szWindowPlacement [TMP_BUF_LEN] ;
	HKEY              hKeyNames;
	DWORD             Size,Type,Status;

	if ( UseDefaultScreenSize ) return TRUE;	// 固定の画面サイズを使う

	Status = RegOpenKeyEx(HKEY_CURRENT_USER, CgfNamesKey, 0L, KEY_READ, &hKeyNames) ;
	if (Status == ERROR_SUCCESS) {
		// get the window placement data
		Size = sizeof(szWindowPlacement);

		// Main Window の位置を復元
		if ( flag==1 ) Status = RegQueryValueEx(hKeyNames, WindowKeyName, NULL, &Type, (LPBYTE)szWindowPlacement, &Size) ;
		else           Status = RegQueryValueEx(hKeyNames, PrtWinKeyName, NULL, &Type, (LPBYTE)szWindowPlacement, &Size) ;
		RegCloseKey (hKeyNames);

		if (Status == ERROR_SUCCESS) {
			sscanf (szWindowPlacement, "%d %d %d %d %d %d %d %d %d", &WindowPlacement.showCmd,
				&WindowPlacement.ptMinPosition.x, &WindowPlacement.ptMinPosition.y,
				&WindowPlacement.ptMaxPosition.x, &WindowPlacement.ptMaxPosition.y,
				&WindowPlacement.rcNormalPosition.left,  &WindowPlacement.rcNormalPosition.top,
				&WindowPlacement.rcNormalPosition.right, &WindowPlacement.rcNormalPosition.bottom );
   
   			WindowPlacement.showCmd = SW_SHOW;
			WindowPlacement.length  = sizeof(WINDOWPLACEMENT);
			WindowPlacement.flags   = WPF_SETMINPOSITION;

			if ( !SetWindowPlacement(hWnd, &WindowPlacement) ) return (FALSE);

			return (TRUE) ;
		} else return (FALSE);
	}

	// open registry failed, use input from startup info or Max as default
	PRT("レジストリのオープンに失敗！\n");
	ShowWindow (hWnd, SW_SHOW) ;
	return (FALSE) ;
}

// レジストリに情報（画面サイズと位置）を書き込む flag == 1 ... main window, 2... print window
BOOL SaveMainWindowPlacement(HWND hWnd,int flag)
{
	WINDOWPLACEMENT   WindowPlacement;
	TCHAR             ObjectType [2] ;
    TCHAR             szWindowPlacement [TMP_BUF_LEN] ;
    HKEY              hKeyNames = 0 ;
    DWORD             Size ;
    DWORD             Status ;
    DWORD             dwDisposition ;
 
	if ( hWnd == NULL ) return FALSE;	// 情報表示画面を閉じている
	ObjectType [0] = TEXT(' ') ;
	ObjectType [1] = TEXT('\0') ;

    WindowPlacement.length = sizeof(WINDOWPLACEMENT);
    if ( !GetWindowPlacement(hWnd, &WindowPlacement) ) return FALSE;
		sprintf(szWindowPlacement, "%d %d %d %d %d %d %d %d %d",
			WindowPlacement.showCmd, 
            WindowPlacement.ptMinPosition.x, WindowPlacement.ptMinPosition.y,
            WindowPlacement.ptMaxPosition.x, WindowPlacement.ptMaxPosition.y,
            WindowPlacement.rcNormalPosition.left,  WindowPlacement.rcNormalPosition.top,
            WindowPlacement.rcNormalPosition.right, WindowPlacement.rcNormalPosition.bottom) ;

	// try to create it first
	Status = RegCreateKeyEx(HKEY_CURRENT_USER, CgfNamesKey, 0L,
		ObjectType, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE, NULL, &hKeyNames, &dwDisposition) ;

	// if it has been created before, then open it
	if (dwDisposition == REG_OPENED_EXISTING_KEY) {
		Status = RegOpenKeyEx(HKEY_CURRENT_USER, CgfNamesKey, 0L, KEY_WRITE, &hKeyNames);
    }

	// we got the handle, now store the window placement data
	if (Status == ERROR_SUCCESS) {
		Size = (lstrlen (szWindowPlacement) + 1) * sizeof (TCHAR) ;
		if ( flag==1 ) Status = RegSetValueEx(hKeyNames, WindowKeyName, 0, REG_SZ, (LPBYTE)szWindowPlacement, Size) ;
		else           Status = RegSetValueEx(hKeyNames, PrtWinKeyName, 0, REG_SZ, (LPBYTE)szWindowPlacement, Size) ;

		RegCloseKey (hKeyNames);
	}

	return (Status == ERROR_SUCCESS) ;
}

DWORD SaveRegList(HKEY hKeyNames, TCHAR *SubNamesKey,int nList, char sList[][TMP_BUF_LEN])
{
	TCHAR             ObjectType [2] ;
    DWORD             Size ;
    DWORD             Status ;
    DWORD             dwDisposition ;

	// try to create it first
	Status = RegCreateKeyEx(HKEY_CURRENT_USER, SubNamesKey, 0L,
		ObjectType, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE, NULL, &hKeyNames, &dwDisposition) ;
	// if it has been created before, then open it
	if (dwDisposition == REG_OPENED_EXISTING_KEY) {
		Status = RegOpenKeyEx(HKEY_CURRENT_USER, SubNamesKey, 0L, KEY_WRITE, &hKeyNames);
    }
	if (Status == ERROR_SUCCESS) {
		int i;
		char sTmp[TMP_BUF_LEN];

		Size = sizeof(sList[0]);
//		PRT("size=%d\n",Size);
		for (i=0;i<nList;i++) {
			sprintf(sTmp,"File%d",i+1);
//			PRT("%s\n",sTmp);
			Status |= RegSetValueEx(hKeyNames, sTmp, 0, REG_SZ, (LPBYTE)sList[i], Size) ;
		}
		RegCloseKey (hKeyNames);
	}
	return Status;
}

// レジストリに情報（効果音、画面の色）を書き込む
BOOL SaveRegCgfInfo(void)
{
	TCHAR             ObjectType [2] ;
    HKEY              hKeyNames = 0 ;
    DWORD             Status ;
    DWORD             dwDisposition ;
	TCHAR   SubNamesKey[TMP_BUF_LEN] ;
 
	ObjectType [0] = TEXT(' ') ;
	ObjectType [1] = TEXT('\0') ;

	// try to create it first
	Status = RegCreateKeyEx(HKEY_CURRENT_USER, CgfNamesKey, 0L,
		ObjectType, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE, NULL, &hKeyNames, &dwDisposition) ;

	// if it has been created before, then open it
	if (dwDisposition == REG_OPENED_EXISTING_KEY) {
		Status = RegOpenKeyEx(HKEY_CURRENT_USER, CgfNamesKey, 0L, KEY_WRITE, &hKeyNames);
    }

//	fInfoWindow = (hPrint != NULL);
	// we got the handle, now store the window placement data
	if (Status == ERROR_SUCCESS) {
		Status  = 0;
		Status |= RegSetValueEx(hKeyNames, SoundDropName,  0, REG_DWORD, (LPBYTE)&SoundDrop,      sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, BoardColorName, 0, REG_DWORD, (LPBYTE)&ID_NowBanColor, sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, InfoWindowName, 0, REG_DWORD, (LPBYTE)&fInfoWindow,    sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, BoardSizeName,  0, REG_DWORD, (LPBYTE)&ban_size,       sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, GnugoLifeName,  0, REG_DWORD, (LPBYTE)&fGnugoLifeDeathView, sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, TurnPlayer0Name,0, REG_DWORD, (LPBYTE)&turn_player[0], sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, TurnPlayer1Name,0, REG_DWORD, (LPBYTE)&turn_player[1], sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, NameListNumName,0, REG_DWORD, (LPBYTE)&nNameList     , sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, GtpPathNumName, 0, REG_DWORD, (LPBYTE)&nGtpPath      , sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, NngsIPNumName,  0, REG_DWORD, (LPBYTE)&nNngsIP       , sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, NngsTimeMinutes,0, REG_DWORD, (LPBYTE)&nngs_minutes  , sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, ATViewName,     0, REG_DWORD, (LPBYTE)&fATView       , sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, AIRyuseiName,   0, REG_DWORD, (LPBYTE)&fAIRyusei     , sizeof(DWORD) ) ;
		Status |= RegSetValueEx(hKeyNames, ShowConsoleName,0, REG_DWORD, (LPBYTE)&fShowConsole  , sizeof(DWORD) ) ;

		if ( Status != ERROR_SUCCESS ) PRT("RegSetValueEx Err\n");
		RegCloseKey (hKeyNames);
	}

	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,FileListName);	// Yssの下にFile用の格納場所を作る
	SaveRegList(hKeyNames, SubNamesKey, nRecentFile, sRecentFile);
	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,NameListName);
	SaveRegList(hKeyNames, SubNamesKey, nNameList, sNameList);
	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,GtpPathName);
	SaveRegList(hKeyNames, SubNamesKey, nGtpPath, sGtpPath);
	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,NngsIPName);
	SaveRegList(hKeyNames, SubNamesKey, nNngsIP, sNngsIP);

	return (Status == ERROR_SUCCESS) ;
}

void ReadRegistoryInt(HKEY hKey, TCHAR *sName, int *pValue, int default_value)
{
	DWORD size = sizeof(int);
	DWORD Type;
	DWORD status = RegQueryValueEx(hKey, sName, NULL, &Type, (LPBYTE)pValue, &size) ;
	if ( status == ERROR_SUCCESS) return;
	PRT("load reg err. set default %s=%d\n",sName,default_value);
	*pValue = default_value;
}

void ReadRegistoryStr(HKEY hKey, TCHAR *sName, char *str, DWORD size, char *default_str)
{
//	DWORD size = sizeof(str);	// これだと現時点の文字列の長さを返してしまう。NULLだと0。ERROR_MORE_DATA になる。
	DWORD Type;
	DWORD status = RegQueryValueEx(hKey, sName, NULL, &Type, (LPBYTE)str, &size) ;
	if ( status == ERROR_SUCCESS) return;
	PRT("load reg err=%d size=%d,%s=%s, %s\n",status,size,sName,str,default_str);
	strcpy(str,default_str);
}

DWORD LoadRegList(TCHAR *SubNamesKey,int *pList, int nListMax, char sList[][TMP_BUF_LEN])
{
	HKEY hKeyNames;
	DWORD Size,Type,Status;
	Status = RegOpenKeyEx(HKEY_CURRENT_USER, SubNamesKey, 0L, KEY_READ, &hKeyNames) ;
	if (Status == ERROR_SUCCESS) {	// 履歴情報を取得
		int i;
		char sTmp[TMP_BUF_LEN];
		for (i=0;i<nListMax;i++) {
			sprintf(sTmp,"File%d",i+1);
			Size = sizeof(sList[0]);
			Status = RegQueryValueEx(hKeyNames, sTmp,  NULL, &Type, (LPBYTE)sList[i],  &Size) ;
//			PRT("%s,%s\n",sTmp,sRecentFile[i]);
			if (Status != ERROR_SUCCESS) break;
			(*pList) = i+1;
		}
		RegCloseKey (hKeyNames);
	}
	return Status;
}


// レジストリから情報を読み込む ---> 情報Windowはメインを殺す前に消されるので判定が難しい・・・。
BOOL LoadRegCgfInfo(void)	
{
	HKEY    hKeyNames;
	DWORD   Status;
	TCHAR   SubNamesKey[TMP_BUF_LEN] ;

	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,FileListName);	// Yssの下にFile用の格納場所を
	LoadRegList(SubNamesKey, &nRecentFile, RECENT_FILE_MAX, sRecentFile);
	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,NameListName);
	LoadRegList(SubNamesKey, &nNameList, NAME_LIST_MAX, sNameList);
	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,GtpPathName);
	LoadRegList(SubNamesKey, &nGtpPath, GTP_PATH_MAX, sGtpPath);
	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,NngsIPName);
	LoadRegList(SubNamesKey, &nNngsIP, NNGS_IP_MAX, sNngsIP);

	Status = RegOpenKeyEx(HKEY_CURRENT_USER, CgfNamesKey, 0L, KEY_READ, &hKeyNames) ;
	if (Status == ERROR_SUCCESS) {
		ReadRegistoryInt(hKeyNames, SoundDropName,   &SoundDrop,             1);
		ReadRegistoryInt(hKeyNames, BoardColorName,  &ID_NowBanColor, ID_BAN_1);
		ReadRegistoryInt(hKeyNames, InfoWindowName,  &fInfoWindow,           0);
		ReadRegistoryInt(hKeyNames, BoardSizeName,   &ban_size,         BAN_19);
		ReadRegistoryInt(hKeyNames, GnugoLifeName,   &fGnugoLifeDeathView,   1);
		ReadRegistoryInt(hKeyNames, TurnPlayer0Name, &turn_player[0], PLAYER_HUMAN);
		ReadRegistoryInt(hKeyNames, TurnPlayer1Name, &turn_player[1], PLAYER_DLL);
		ReadRegistoryInt(hKeyNames, NameListNumName, &nNameList,             0);
		ReadRegistoryInt(hKeyNames, GtpPathNumName,  &nGtpPath,              0);
		ReadRegistoryInt(hKeyNames, NngsIPNumName,   &nNngsIP,               0);
		ReadRegistoryInt(hKeyNames, NngsTimeMinutes, &nngs_minutes,         40);
		ReadRegistoryInt(hKeyNames, ATViewName,      &fATView,               0);
		ReadRegistoryInt(hKeyNames, AIRyuseiName,    &fAIRyusei,             1);
		ReadRegistoryInt(hKeyNames, ShowConsoleName, &fShowConsole,          1);

		if ( UseDefaultScreenSize ) fInfoWindow = 1;	// 常にデバグ画面は表示
		RegCloseKey (hKeyNames);
		return (TRUE);
	}

	// open registry failed, use input from startup info or Max as default
	PRT("レジストリのオープン(細かい情報)に失敗！\n");
	return (FALSE) ;
}

// レジストリからCPUの情報を読み込む
void LoadRegCpuInfo(void)
{
	HKEY  hKeyNames;
	DWORD Size,Type,Status;
	static TCHAR CpuFsbKey[] = TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");

	sprintf(sYourCPU,"");
	Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CpuFsbKey, 0L, KEY_READ, &hKeyNames);
	if (Status == ERROR_SUCCESS) {
		Size = sizeof(sYourCPU);
		Status = RegQueryValueEx(hKeyNames, "ProcessorNameString",  NULL, &Type, (LPBYTE)sYourCPU, &Size);
		RegCloseKey (hKeyNames);
		if (Status != ERROR_SUCCESS) PRT("CPU取得失敗\n");
	}
	PRT("%s\n",sYourCPU);
}



// 文字列配列の候補リストを更新。一番最近選んだものを先頭へ。
void UpdateStrList(char *sAdd, int *pListNum, const int nListMax, char sList[][TMP_BUF_LEN] )
{
	int i,j,nLen;
	char sTmp[TMP_BUF_LEN];

	nLen = strlen(sAdd);
	if ( nLen >= TMP_BUF_LEN ) return;
	for (i=0;i<(*pListNum);i++) {
		if ( strcmp(sList[i], sAdd) == 0 ) break;	// 既にリストにあるファイルを開いた
	}
	if ( i == (*pListNum) ) {	// 新規に追加。古いものを削除。
		for (i=nListMax-1;i>0;i--) strcpy(sList[i],sList[i-1]);	// スクロール
		strcpy(sList[0],sAdd);
		if ( (*pListNum) < nListMax ) (*pListNum)++;
	} else {					// 既に開いたファイルを先頭に。
		strcpy(sTmp,    sList[i]);
		for (j=i;j>0;j--) strcpy(sList[j],sList[j-1]);	// スクロール
		strcpy(sList[0],sTmp    );
	}
}

// ファイルリストを更新。一番最近開いたものを先頭へ。
void UpdateRecentFile(char *sAdd)
{
	UpdateStrList(sAdd, &nRecentFile, RECENT_FILE_MAX, sRecentFile );
}

// メニューを変更する
void UpdateRecentFileMenu(HWND hWnd)
{
	HMENU hMenu,hSubMenu;
	int i;
	char sTmp[TMP_BUF_LEN];

	hMenu    = GetMenu(hWnd);
	hSubMenu = GetSubMenu(hMenu, 0);

	for (i=0;i<RECENT_FILE_MAX;i++) {
		DeleteMenu(hSubMenu, IDM_RECENT_FILE + i, MF_BYCOMMAND );
	}

	for (i=0;i<nRecentFile;i++) {
		if ( i < 10 ) sprintf(sTmp,"&%d ",(i+1)%10);
		else          sprintf(sTmp,"  ");
		strcat(sTmp,sRecentFile[i]);
		InsertMenu(hSubMenu, RECENT_POS+i,MF_BYPOSITION,IDM_RECENT_FILE + i,sTmp);
//		AppendMenu(hSubMenu, MF_STRING, IDM_RECENT_FILE + i, sTmp);
	}
}

// ファイルリストから1つだけ削除。ファイルが消えたので。
void DeleteRecentFile(int n)
{
	int i;

	// 新規に追加。古いものを削除。
	for (i=n;i<nRecentFile-1;i++) strcpy(sRecentFile[i],sRecentFile[i+1]);	// スクロール
	nRecentFile--;
	if ( nRecentFile < 0 ) { PRT("DeleteRecent Err!\n"); debug(); }
}



HMODULE hLib = NULL;	// アマゾン思考用のDLLのハンドル

#define CGFTHINK_DLL "cgfthink.dll"

// DLLを初期化する。成功した場合は0を。エラーの場合はそれ以外を返す
int InitCgfGuiDLL(void)
{
	hLib = LoadLibrary(CGFTHINK_DLL);
	if ( hLib == NULL ) return 1;

	// 使う関数のアドレスを取得する。
	cgfgui_thinking = (int(*)(int[], int [][3], int, int, int, double, int, int[]))GetProcAddress(hLib,"cgfgui_thinking");
	if ( cgfgui_thinking == NULL ) {
		PRT("Fail GetProcAdreess 1\n");
		FreeLibrary(hLib);
		return 2;
	}
	cgfgui_thinking_init = (void(*)(int *))GetProcAddress(hLib,"cgfgui_thinking_init");
	if ( cgfgui_thinking_init == NULL ) {
		PRT("Fail GetProcAdreess 2\n");
		FreeLibrary(hLib);
		return 3;
	}
	cgfgui_thinking_close = (void(*)(void))GetProcAddress(hLib,"cgfgui_thinking_close");
	if ( cgfgui_thinking_close == NULL ) {
		PRT("Fail GetProcAdreess 3\n");
		FreeLibrary(hLib);
		return 4;
	}

	return 0;
}

// DLLを解放する。
void FreeCgfGuiDLL(void)
{
	if ( hLib == NULL ) return;
	cgfgui_thinking_close();
	FreeLibrary(hLib);
}



#define BOARD_SIZE_MAX ((19+2)*256)		// 19路盤を最大サイズとする

// 着手位置を返す
int ThinkingDLL(int endgame_type)
{
	// DLLに渡す変数
	static int dll_init_board[BOARD_SIZE_MAX];
	static int dll_kifu[KIFU_MAX][3];
	static int dll_tesuu;
	static int dll_black_turn;
	static int dll_board_size;
	static double dll_komi;
	static int dll_endgame_type;
	static int dll_endgame_board[BOARD_SIZE_MAX];
	// 
	int x,y,z,i,ret_z;
	clock_t ct1;

	// 現在の棋譜をセットする。
	for (i=0;i<BOARD_SIZE_MAX;i++) {
		dll_init_board[i]    = 3;	// 枠で埋める
		dll_endgame_board[i] = 3;
	}
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = (y+1) * 256 + (x+1);
		dll_init_board[z]    = ban_start[z];
		dll_endgame_board[z] = 0;
	}
	for (i=0;i<tesuu;i++) {
		dll_kifu[i][0] = kifu[i+1][0];	// CgfGui内部でkifu[0]は未使用なのでずらす。
		dll_kifu[i][1] = kifu[i+1][1];
		dll_kifu[i][2] = kifu[i+1][2];
	}
	dll_tesuu        = tesuu;
	dll_black_turn   = (GetTurnCol() == 1);
	dll_board_size   = ban_size;
	dll_komi         = komi;
	dll_endgame_type = endgame_type;

	// DLLの関数を呼び出す。
	ct1 = clock();
	thinking_utikiri_flag = 0;	// 中断ボタンを押された場合に1に。
	fPassWindows = 1;			// DLLを呼び出し中は中断ボタン以外を使用不能に
	MenuEnableID(IDM_DLL_BREAK, 1);
	ret_z = cgfgui_thinking(dll_init_board, dll_kifu, dll_tesuu, dll_black_turn, dll_board_size, dll_komi, dll_endgame_type, dll_endgame_board);
	fPassWindows = 0;
	MenuEnableID(IDM_DLL_BREAK, 0);
	PRT("位置=(%2d,%2d),時間=%2.1f(秒),手数=%d,終局=%d\n",ret_z&0xff,ret_z>>8,GetSpendTime(ct1),dll_tesuu,endgame_type);
	PRT("----------------------------------------------\n");

	// 終局、図形、数値表示用の盤面に値をセット
	if ( endgame_type != ENDGAME_MOVE ) {
		for (i=0;i<BOARD_SIZE_MAX;i++) endgame_board[i] = dll_endgame_board[i];
	}

	return ret_z;
}

/*
void TestDllThink(void)
{
	static int fFirst = 1;	// 最初の1度だけDLLを初期化
	if ( fFirst ) {	
		if ( InitCgfGuiDLL() == 0 ) return;	// DLLを初期化する。
		cgfgui_thinking_init(&thinking_utikiri_flag);
		fFirst = 0;
	}
	ThinkingTestDLL();
//	FreeCgfGuiDLL();	// DLLを解放する。
}
*/

