/* cgf_term.cpp */
/*
  このソースは、囲碁の通信プロトコル SGMP(Standard Go Modem Protocol
- Revision 1.0) 用です。通信ポートのオープン、クローズ、通信データの
受信、送信などを行っています。

  言語は VisualC++6.0 で書かれています。MFC はいっさい使っていません。
SDKのみです。またスレッドで動作していません。ある一定の時間（WM_TIMER）
で通信バッファをのぞきに行く形をとっています。

  FOST杯では通信は必須ではありませんが、手入力だと、持ち時間のハンデが
つきます。今後作成される方は、参考にしていただければ幸いです。

・注意！
送られてきたシーケンスビットが正しいかどうかの判定はしていません。

このソースに関しては、私（山下宏）は著作権を放棄します。
再配布、転載、コピー、改造は自由に行って構いません。

2005/07/14 山下 宏
yss@bd.mbn.or.jp
-----------------------------------------------------------------------

This is a sample of SGMP(Standard Go Modem Protocol - Revision 1.0).
Source is built by Visual C++6.0 (Japanese)

Program checks COMM BUFFER once the 20m-second.

This source doesn't check seaquence bit.

I don't insist any copyright about this source.
You can copy and change freely.

2005/07/14
Hiroshi Yamashita
yss@bd.mbn.or.jp
*/

//#define CGFCOM_9x9	// CGFCOM.DLLと9路で対戦する場合は定義する。CGFCOM.DLLのバグ対策

#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "cgf.h"
#include "cgf_wsk.h"

#define TIMER_ID 1		/* 通信受信用タイマーID    */
#define INTERVAL 20		/* 20ms毎にTIMERが呼ばれる */

BOOL NEAR OpenConnection( void );
BOOL NEAR CloseConnection( void );
BOOL CheckCommBuff(void);
BOOL CALLBACK CommProc( HWND ,UINT, WPARAM, LPARAM);
BOOL AnalyzeStr(void);
void ReturnOK(void);
void AnswerQuery(void);
void ReadMove(void);
void Delay(DWORD dWait);	// 時間待ちの関数

HANDLE hCommDev = INVALID_HANDLE_VALUE;
BOOL fCommConnect = 0;	// 接続状態を示す

static int nLen = 0;	// ０～３Byte。受信したバイト数
static BYTE StartByte;
static BYTE CheckSum;
static BYTE CommHI;		// コマンドの上位バイト
static BYTE CommLO;		//           下位バイト
static BOOL BitH = 0;	// 自分のシーケンスビット
static BOOL BitY = 0;	// 相手のシーケンスビット
static BOOL BitHH,BitYY;// 新しいシーケンスビット
static BOOL WaitOK = 0;	// 最初はニュートラル状態

static BYTE lpRead[4+1];	// 受信用文字列
static BYTE lpSend[4+1];	// 送信用文字列

static DWORD RetByte;	// 単なるダミーの変数

extern HWND ghWindow;					/* WINDOWSのハンドル  */
extern HINSTANCE ghInstance;			/* インスタンス       */

static BOOL fEndDlg;	// ダイアログを終了するフラグ




BOOL NEAR OpenConnection( void )
{            
	DCB dcb;	// 通信情報構造体
    COMMTIMEOUTS CommTimeOuts;	// 通信タイムアウトの構造体
	char szCommNumber[6];

	BitH = 0;	// 自分のシーケンスビット
	BitY = 0;	// 相手のシーケンスビット

	BitH = BitY = BitHH = BitYY = 0;	// 全てのシーケンスビットを初期化

	// 通信ポートのオープン
	sprintf( szCommNumber, "COM%d",nCommNumber);
	PRT("%sで通信を試みます。\n",szCommNumber);
	hCommDev = CreateFile( szCommNumber, GENERIC_READ | GENERIC_WRITE,
				         0,                    // exclusive access
				         NULL,                 // no security attrs
				         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hCommDev == INVALID_HANDLE_VALUE ) return(FALSE);	// 通信ポートオープン失敗

	// 通信タイムアウトの設定
	CommTimeOuts.ReadIntervalTimeout         = MAXDWORD;
    CommTimeOuts.ReadTotalTimeoutMultiplier  =    0;
    CommTimeOuts.ReadTotalTimeoutConstant    = 1000;
    CommTimeOuts.WriteTotalTimeoutMultiplier =    0;
    CommTimeOuts.WriteTotalTimeoutConstant   = 1000;
    if ( !SetCommTimeouts( hCommDev, &CommTimeOuts ) ) {
        CloseHandle(hCommDev);
        return (FALSE); // SetCommTimeouts error
    }

	GetCommState( hCommDev, &dcb );	// 通信情報の取得
	dcb.BaudRate = 2400;			// 2400ボーに設定
	dcb.ByteSize = 8;				// 8データビット
	dcb.Parity = NOPARITY;			// パリティなし
	dcb.StopBits = ONESTOPBIT;		// 1ストップビット
	SetCommState( hCommDev, &dcb );	// 通信情報の設定
	return ( TRUE ) ;
}

BOOL NEAR CloseConnection( void )
{
	if ( fUseNngs ) { close_nngs();	return TRUE; }
	return ( CloseHandle( hCommDev ) );
}

// 通信バッファから1byteづつ読み込む。
BOOL CheckCommBuff(void )
{
	BOOL	fReadStat;
	COMSTAT	ComStat;
	DWORD	dwError, dwLen;

	ClearCommError( hCommDev, &dwError, &ComStat );	// 受信データがあるかチェック
	dwLen = ComStat.cbInQue;	// そのバイト数

	if (dwLen > 0) {
		fReadStat = ReadFile( hCommDev, (LPSTR *)lpRead, 1, &dwLen, NULL ); // 1Byteだけ読みとる
//		PRT("data=%02x: nLen=%d, StartByte=%x, Sum=%x, ComH=%x, ComL=%x\n",lpRead[0], nLen,StartByte, CheckSum,CommHI,CommLO);
		if ( fReadStat == FALSE ) PRT("受信処理にてエラーが発生しました\n");
		else return (TRUE);
	}
	return (FALSE);
}


// 受信待ちをするダイアログボックス関数
// ダイアログを終了する時は、手が返ってきた場合と、エラー、キャンセル、の場合。
BOOL CALLBACK CommProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/ )
{
	switch(msg) {
		case WM_INITDIALOG:
			fEndDlg = FALSE;
			RetComm = 0;	// エラーで終了した場合
			SetTimer(hDlg, TIMER_ID, INTERVAL, NULL);		// タイマーをセット
			return TRUE;

		case WM_COMMAND:
			if ( wParam == IDCANCEL ) {
				KillTimer(hDlg, TIMER_ID);	// タイマー停止
				EndDialog(hDlg,FALSE);			// 中断で終了
			}
			return TRUE;
/*
		// WinSockを使って通信する場合
		case WM_USER_ASYNC_SELECT:
			OnServerAsyncSelect(hDlg, wParam, lParam);	// 非同期のSelectを受け取る
			return TRUE;
*/
		case WM_TIMER:
			if ( CheckCommBuff() ) {
				if ( AnalyzeStr() == FALSE ) {
					PRT("受信文字列解析でエラーが発生しました。\n");
					KillTimer(hDlg, TIMER_ID);	// タイマー停止
					EndDialog(hDlg,FALSE);		// エラーで終了
				}
			}
			if ( fEndDlg ) {
				KillTimer(hDlg, TIMER_ID);	// タイマー停止
				EndDialog(hDlg,TRUE);		// 正常終了
			}
			return TRUE;
	}
	return FALSE;
}

// 受信文字列の解析
BOOL AnalyzeStr(void)
{
	BYTE c;

	c = lpRead[0];

	if ( (c & 0xfc) == 0 ) {	// 0000 00hy スタートビットが来た場合
		nLen = 1;
		BitHH = ( c & 0x02) >> 1;
		BitYY = ( c & 0x01);
		StartByte = c;
		return(TRUE);
	}

	if ( (c & 0x80) == 0 ) {	// TEXT文字列と思われる
		PRT("Text=%x,%c\n",c,c);	// debug用の表示関数
		return(TRUE);
	}
	
	if ( nLen == 0 ) return(TRUE);	// 不要な文字

	if ( nLen == 1 ) {	// ２バイト目はチェックサム
//		CheckSum = c & 0x7f;
		CheckSum = c;
		nLen = 2;
		return(TRUE);
	}

	if ( nLen == 2 ) {	// ３バイト目はコマンドの上位バイト
		CommHI = c;
		nLen = 3;
		return(TRUE);
	}

	if ( nLen == 3 ) {	// ４バイト目
		nLen = 0;
		CommLO = c;							
		c = (StartByte + CommHI + CommLO) & 0x7f;
		if ( c != (CheckSum & 0x7f) ) {
			PRT("チェックサムエラー CheckSum=%x, Sum=%x\n",CheckSum,c);
			return(FALSE);	// チェックサムエラー
		}

		PRT("Read Data    <- %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);

		// シーケンスビットのチェックをする？
		// BitH ,BitY  現在のシーケンスビット
		// BitHH,BitYY 受信したシーケンスビット
		
		if ( CommHI == 0x87 && CommLO == 0xff ) {					// OK!
			fEndDlg = TRUE;		// OK が返ってきたらDAILOGを抜ける
			RetComm = 2;		// OK が来た
			PRT("OKを受信。\n");
		}
		else if ( CommHI == 0x90 && CommLO == 0x80 ) ReturnOK();	// DENY
		else if ( CommHI == 0xa0 && CommLO == 0x80 ) {				// NEWGAME
			fEndDlg = TRUE;		// DAILOGを抜ける
			RetComm = 3;		// NEWGAME が来た!
			PRT("NEWGAMEが来た！\n");
		}
		else if ( (CommHI & 0xf0) == 0xb0 ) AnswerQuery();			// QUERY // 全て０で答える 
		else if ( (CommHI & 0xf0) == 0xc0 ) {						// ANSWER
			fEndDlg = TRUE;		// DAILOGを抜ける
			RetComm = 4;		// ANSWERが帰ってきた
			PRT("ANSWERが戻ってきた。\n");
		}
		else if ( (CommHI & 0xf0) == 0xd0 ) ReadMove();				// Move
		else if ( (CommHI & 0xf0) == 0xe0 ) return(FALSE);			// Back Move
		else if ( (CommHI & 0xf0) == 0xf0 ) return(FALSE);			// 拡張コマンドは使わない
		return(TRUE);
	}
	PRT("4バイト目を超えた\n");
	return(FALSE);
}

// 4バイトを送信
void Send4Byte(unsigned char ptr[])
{
	Delay(100);	// 少しだけ待つ。
	WriteFile( hCommDev, (LPSTR *)ptr, 4, &RetByte, NULL );
}

// OK を返す
void ReturnOK(void)
{

//	StartByte = (BitHH << 1) + BitYY;
	StartByte = (BitYY << 1) + BitHH;	// 自分と相手のビットを交換
	CommHI = 0x87;
	CommLO = 0xff;
	CheckSum = StartByte + ( CommHI & 0x7f ) + ( CommLO & 0x7f );
	CheckSum |= 0x80;

	lpSend[0] = StartByte;
	lpSend[1] = CheckSum;
	lpSend[2] = CommHI;
	lpSend[3] = CommLO;
	Delay(100);						/* 一定時間(100ms)待つ */
	PRT("Send Data    -> %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);
	PRT("ReturnOK()\n");
	Send4Byte(lpSend);
//	WriteFile( hCommDev,(void *)lpSend, 4, &RetByte, NULL );
}

// 
void AnswerQuery(void)
{
	PRT("Query(%d)を受信\n",CommLO & 0x7f);

	BitH = ( BitH == 0 );	// 論理演算	0 -> 1, 1 -> 0
	StartByte = (BitYY << 1) + BitH;

	if ( CommHI == 0xb0 && CommLO == 0x8b ) {	// あなたは何色？
		CommHI = 0xc0;
		CommLO = 0x80 + UN_COL(dll_col);	// DLLが黒の時２
	} else if ( CommHI == 0xb0 && CommLO == 0x88 ) {	// handicapは？
		CommHI = 0xc0;
		CommLO = 0x81;	// 互い先に決まってるだろベイベー
	} else if ( CommHI == 0xb0 && CommLO == 0x87 ) {	// ルールは
		CommHI = 0xc0;
		CommLO = 0x81;	// 日本ルール
//		CommLO = 0x82;	// Ing or Chinese
	} else if ( CommHI == 0xb0 && CommLO == 0x89 ) {	// 盤のサイズは？
		CommHI = 0xc0;
//		CommLO = 0x93;	// 19*19
//		CommLO = 0x89;	// 9*9
		CommLO = 0x80 + ban_size;
	} else {
		CommHI = 0xc0;
		CommLO = 0x80;	// 後はみんな当然０（unknown）
	}
	CheckSum = StartByte + ( CommHI & 0x7f ) + ( CommLO & 0x7f );
	CheckSum |= 0x80;

	lpSend[0] = StartByte;
	lpSend[1] = CheckSum;
	lpSend[2] = CommHI;
	lpSend[3] = CommLO;
	Delay(500);						/* 一定時間(500ms)待つ */
	PRT("Send Data    -> %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);
	PRT("Answer(%d)\n",CommLO & 0x7f);
	Send4Byte(lpSend);
//	WriteFile(hCommDev, lpSend, 4, &RetByte, NULL);
}

// 手を受け取る
void ReadMove(void)
{
	int x,y,z;

//	if ( CommHI & 0x04 ) PRT("白石を置いたよ\n");
//	else PRT("黒石を置いたよ\n");
	z = CommLO & 0x7f;
	z += (CommHI & 0x03) << 7;
	PRT("位置(1-361)=%d ",z);
	if ( z != 0 ) {
		z = z - 1;
#ifdef CGFCOM_9x9	// CGFCOM.DLL用
		y = z / 19;
		x = z - y*19;
		y = 19 - y;
#else
		y = z / ban_size;
		x = z - y*ban_size;
		y = ban_size - y;
#endif
		y = y << 8;
		z = y + x + 1;
		// z が１９の倍数の時バグル
/*		y = z / 19;
		x = z - y*19;
		y = 19 - y;
		y = y << 8;
		z = y + x;
*/
	}
	PRT("(内部変換後=%x)\n",z);
	ReturnOK();
	RetZ = z;
	RetComm = 1;	// 手が来た場合は１
	fEndDlg = TRUE;
}

// 手 ｚ を送る
void SendMove(int z)
{
	int x,y;

	if ( fUseNngs ) {
		char str[TMP_BUF_LEN],str2[TMP_BUF_LEN];
		change_z_str(str,z);
		sprintf(str2,"%s\n",str);
		NngsSend(str2);
		return;
	}

	BitH = ( BitH == 0 );	// 論理演算	0 -> 1, 1 -> 0
	StartByte = (BitYY << 1) + BitH;

	if ( z != 0 ) {
		x = z & 0x00ff;
		y = z >> 8;
#ifdef CGFCOM_9x9	// CGFCOM.DLL用
		y = 19 - y;
		z = y*19 + x;
#else
		y = ban_size - y;
		z = y*ban_size + x;
#endif
	}
	CommHI = (z & 0x180) >> 7;
	CommHI |= 0xd0;
	if ( dll_col == 2 ) CommHI |= 0x04;	// 自分が白なら白番を設定
	CommLO = (z & 0x7f) | 0x80;

	CheckSum = StartByte + ( CommHI & 0x7f ) + ( CommLO & 0x7f );
	CheckSum |= 0x80;

	lpSend[0] = StartByte;
	lpSend[1] = CheckSum;
	lpSend[2] = CommHI;
	lpSend[3] = CommLO;
	PRT("Send My Move -> %02x:%02x:%02x:%02x\n",lpSend[0],lpSend[1],lpSend[2],lpSend[3]);
	Send4Byte(lpSend);
//	WriteFile( hCommDev,(void *)lpSend, 4, &RetByte, NULL );
}

// NewGameを送る
int SendNewGame(void)
{
	BitH = BitY = BitHH = BitYY = 0;	// 念のためここでもシーケンスビットを初期化

	BitH = ( BitH == 0 );	// 論理演算	0 -> 1, 1 -> 0
	StartByte = (BitYY << 1) + BitH;

	CommHI = 0xa0;
	CommLO = 0x80;

	CheckSum = StartByte + ( CommHI & 0x7f ) + ( CommLO & 0x7f );
	CheckSum |= 0x80;

	lpSend[0] = StartByte;
	lpSend[1] = CheckSum;
	lpSend[2] = CommHI;
	lpSend[3] = CommLO;
	PRT("Send Data    -> %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);
	PRT("NewGame を送信\n");
	Send4Byte(lpSend);
//	WriteFile( hCommDev,(void *)lpSend, 4, &RetByte, NULL );

	// 受信文字列、OK を待つ
	if ( DialogBox(ghInstance, "COMM_DIALOG", ghWindow, (DLGPROC)CommProc) == FALSE ) {
		PRT("NewGameの中断処理\n");
		return(FALSE);
	}
	if ( RetComm != 2 ) {
		PRT("NEWGAME を送ったのに OK が帰ってこないよ。\n");
		return(FALSE);
	}
	return( TRUE );
}

// QUERY 質問を送る。
int SendQuery(int qqq)
{
	BitH = ( BitH == 0 );	// 論理演算	0 -> 1, 1 -> 0
	StartByte = (BitYY << 1) + BitH;

	CommHI = 0xb0;
	CommLO = 0x80 + qqq;

	CheckSum = StartByte + ( CommHI & 0x7f ) + ( CommLO & 0x7f );
	CheckSum |= 0x80;

	lpSend[0] = StartByte;
	lpSend[1] = CheckSum;
	lpSend[2] = CommHI;
	lpSend[3] = CommLO;
	Delay(500);						/* 一定時間(500ms)待つ */
	PRT("Send Data    -> %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);
	PRT("Send Query = %d \n",qqq);
	Send4Byte(lpSend);
//	WriteFile( hCommDev,(void *)lpSend, 4, &RetByte, NULL );
//	PRT("Send Query End\n");

	// 受信文字列、ANSWER を待つ
	if ( DialogBox(ghInstance, "COMM_DIALOG", ghWindow, (DLGPROC)CommProc) == FALSE ) {
		PRT("QUERYの中断処理\n");
		return(FALSE);
	}
	if ( RetComm != 4 ) {
		PRT("QUERY を送ったのに ANSWER が返ってこない・・・。\n");
		return(FALSE);
	}
//	ReturnOK();		// 取りあえずANSWERにはOKを返しましょう
	return( CommLO & 0x7f );
}

// Delay 処理  dWait (ms) だけ待つ。500 で 500ms = 0.5秒
void Delay(DWORD dWait)
{
//	DWORD dTicks;
//	MSG msg;

	if ( dWait > 500 ) PRT("Wait...%dms: ",dWait);
	Sleep(dWait);	// Sleep関数のほうがスマート？。CPU時間はまったく食わない。
/*
	dTicks = GetTickCount();	// システムがスタートしてからのミリ秒を得る

	// 指定時間を経過するまでメッセージループ	 ---> 本体を終了されるとおかしくなる
	while (dWait > (GetTickCount() - dTicks)) {	
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
*/
}
