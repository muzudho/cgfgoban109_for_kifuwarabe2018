/* cgf_wsk.cpp */
// WinSockを使った通信対戦用。wsock32.lib をリンクする必要あり
#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
//#include <winsock.h>

#include "cgf.h"
#include "cgf_wsk.h"


// nngsに接続するテスト

char nngs_player_name[2][TMP_BUF_LEN] = {
//	"aya",
//	"katsunari",
	"test1",
	"test2",
//	"brown",
};

SOCKET Sock = INVALID_SOCKET;

char *stone_str[2] = { "B","W" };

#define NNGS_LOGIN_TOTYU -3	// login処理の途中
#define NNGS_LOGIN_END   -2	// login処理が終了した
#define NNGS_GAME_END    -1	// 対局終了
#define NNGS_UNKNOWN     -4	// 無意味なデータ。無視する。

int nngs_minutes = 40;		// 40分切れ負け
//int already_log_in = -1;

// 0...PASS, 1以上...手。-1以下...エラーやログイン処理
// nngsの内容を解析
//int fTurn;	// 黒番の時 0、白番の時 1
int nngsAnalysis(char *str, int fTurn)
{
	char tmp[TMP_BUF_LEN];

	// AI竜星戦nngs
	if ( fAIRyusei ) {
		if ( strstr(str,"Login:") ) {	// 改行コードで1行を判断してるので失敗する
			sprintf(tmp,"%s\n",nngs_player_name[fTurn]);	// ユーザ名でログイン
			NngsSend(tmp);	
			return NNGS_LOGIN_TOTYU;
		}
		
		if ( strstr(str,"No Name Go Server (NNGS) version") ) {
			NngsSend("set client TRUE\n");	// シンプルな通信モードに
			return NNGS_LOGIN_TOTYU;
		}

		if ( strstr(str,"Set client to be True") ) {
			/*if ( fTurn == 0 ) {
				sprintf(tmp,"match %s B %d %d 0\n",nngs_player_name[1-fTurn],ban_size,nngs_minutes);
				NngsSend(tmp);	// 黒番の場合に、試合を申し込む
			}*/
			//return NNGS_LOGIN_TOTYU;
		}	

		sprintf(tmp,"Match [%dx%d]",ban_size,ban_size);	// "Match [19x19]"
		if ( strstr(str,tmp) ) {
			char opponent_name[TMP_BUF_LEN];
			int my_color = 0;
			int c;
			for (c = 0; c < TMP_BUF_LEN; c++){
				opponent_name[c] = '\0';
			}
			int d = 0;
			int counter = 0;
			int nLen = strlen(str);
			for (c = 0; c < nLen; c++){
				if (str[c] == ' '){
					counter++;

				}
				if (counter == 8){
					if (str[c] != ' '){
						opponent_name[d] = str[c];
						d++;
					}
				}
				if ((counter == 10)){
					if (str[c] == 'B'){
						my_color = 1;
					}
					else if (str[c] == 'W'){
						my_color = 0;
					}
				}
			}
			if (my_color == 0){
				sprintf(tmp, "match %s B %d %d 0\n", opponent_name/*nngs_player_name[1-fTurn]*/, ban_size, nngs_minutes);
			}
			else{
				sprintf(tmp, "match %s W %d %d 0\n", opponent_name/*nngs_player_name[1-fTurn]*/, ban_size, nngs_minutes);
			}

			//sprintf(tmp,"match %s W %d %d 0\n",nngs_player_name[1-fTurn],ban_size,nngs_minutes);
			Sleep(1000);
			NngsSend(tmp);	// 白番の場合に、試合を受ける。
			Sleep(1000);
			return NNGS_LOGIN_TOTYU;
		}

		if (/* fTurn == 0 &&*/ strstr(str,"accepted.") ) {	// 白が応じたので初手を送れる。
			return NNGS_LOGIN_END;
		}

	} else {	// UEC杯のnngs
/*
		if ( strstr(str,"Login:") ) {	// 改行コードで1行を判断してるので失敗する
			sprintf(tmp,"%s\n",nngs_player_name[fTurn]);	// ユーザ名でログイン
			NngsSend(tmp);	
			return NNGS_LOGIN_TOTYU;
		}
*/
		if ( strstr(str,"No Name Go Server (NNGS) version") ) {
			NngsSend("set client TRUE\n");	// シンプルな通信モードに
			return NNGS_LOGIN_TOTYU;
		}
		if ( strstr(str,"Set client to be True") ) {
			if ( fTurn == 0 ) {
				sprintf(tmp,"match %s B %d %d 0\n",nngs_player_name[1-fTurn],ban_size,nngs_minutes);
				NngsSend(tmp);	// 黒番の場合に、試合を申し込む
			}
			return NNGS_LOGIN_TOTYU;
		}	

		sprintf(tmp,"Match [%dx%d]",ban_size,ban_size);	// "Match [19x19]"
		if ( strstr(str,tmp) ) {
			sprintf(tmp,"match %s W %d %d 0\n",nngs_player_name[1-fTurn],ban_size,nngs_minutes);
			NngsSend(tmp);	// 白番の場合に、試合を受ける。
			return NNGS_LOGIN_END;
		}

		if ( fTurn == 0 && strstr(str,"accepted.") ) {	// 白が応じたので初手を送れる。
			return NNGS_LOGIN_END;
		}	
	}


	if ( strstr(str,"Illegal") ) return NNGS_GAME_END;	// Errorの場合

	sprintf(tmp,"(%s): ",stone_str[1-fTurn]);
	char *p = strstr(str,tmp);
	if ( p != NULL ) {
		int x,y;

		p += 5;
		if ( strstr(p,"Pass") ) {
			PRT("相手 -> PASS!\n");
			return 0;
		} else {
			x = *p;
			y = atoi(p+1);
			PRT("相手 -> %c%d\n",x,y);
			x = x  - 'A' - (x > 'I') + 1;	// 'I'を超えたら -1	
			return (ban_size +1 - y)*256 + x;
		}
	}	

	// Passが連続した場合に来る(PASSした後の相手からのPASSは来ない）
	if ( strstr(str,"You can check your score") ) {
		sprintf(tmp,"done\n");	// 地を計算するモードを終えるために "done" を送る
								// この後、"resigns." 文字列が来る。これでサーバに正式な棋譜として保存される。
		NngsSend(tmp);
		return 0;	// PASSを返す
	}

	// 終局処理が終わった。"9 {Game 1: test2 vs test1 : Black resigns. W 10.5 B 6.0}"
	if ( strstr(str,"9 {Game") && strstr(str,"resigns.") ) {
		sprintf(tmp,"%s vs %s",nngs_player_name[1],nngs_player_name[0]);
		if ( strstr(str,tmp) ) return NNGS_GAME_END;	// game end
	}

	sprintf(tmp,"{%s has disconnected}",nngs_player_name[1-fTurn]);
	if ( strstr(str,tmp) ) return NNGS_GAME_END;	// 対戦相手が接続を切った。

	// どちらかの時間が切れた場合
	if ( strstr(str,"forfeits on time") &&
	     strstr(str,nngs_player_name[0]) && strstr(str,nngs_player_name[1]) ) {
		return NNGS_GAME_END;
	}
	return NNGS_UNKNOWN;	// 無意味なデータ
}

// データ送信
void NngsSend(char *ptr)
{
	int n = strlen(ptr);
	int nByteSend = send( Sock, ptr, n, 0);
	if ( nByteSend == SOCKET_ERROR ) {
		PRT("send() Err=%d, %s\n",WSAGetLastError(), ptr);
		return;
	}
	PRT("--->%s",ptr);
}

// Loginする('\n'を待たずにすぐ送る)
void login_nngs(int fTurn)
{
	char tmp[256];
	sprintf(tmp,"%s\n",nngs_player_name[fTurn]);
	NngsSend(tmp);	// すぐにユーザ名でログイン
}

//char sNNGS_IP[TMP_BUF_LEN] = "nngs.computer-go.jp";	// nngs.computer-go.jp 9696	
//char sNNGS_IP[TMP_BUF_LEN] = "192.168.11.11";

int fUseNngs = 0;	// nngs経由で非同期で対戦する場合
extern HWND ghWindow;					/* WINDOWSのハンドル  */

// サーバと接続する。失敗すれば0を返す
int connect_nngs(void)
{
	WSADATA WsaData;
	SOCKADDR_IN Addr;
	unsigned short nPort = 9696;	// サーバのポート番号
	unsigned long addrIP;			// IPが変換される数値
	PHOSTENT host = NULL;			// ホスト情報
	int i;
//	char sIP[] = "192.168.0.1";		// GPW杯でのサーバのIP
//	char sIP[] = "202.250.144.34";	// nngs.computer-go.jp 9696

	// WinSockの初期化
	if ( WSAStartup(0x0101, &WsaData) != 0 ) {
		PRT("Error!: WSAStartup(): %d\n",WSAGetLastError());
		return 0;
	}

	// ソケットを作る
	Sock = socket(PF_INET, SOCK_STREAM, 0);
	if ( Sock == INVALID_SOCKET ) {
		PRT("Error: socket(): %d\n", WSAGetLastError());
		return 0;
	}
	
	// IPアドレスを数値に変換
	addrIP = inet_addr(sNngsIP[0]);
	if ( addrIP == INADDR_NONE ) {
		host = gethostbyname(sNngsIP[0]);		// ホスト名から情報を取得してIPを調べる
		if ( host == NULL ) {
			PRT("Error: gethostbyname(): \n");
			return 0;
		}
		addrIP = *((unsigned long *)((host->h_addr_list)[0]));
	} else {
//		host = gethostbyaddr((const char *)&addrIP, 4, AF_INET);	// Localな環境では失敗する
	}

	if ( host ) {	// IP情報などを表示
		PRT("公式名 = %s\n",host->h_name);
		for (i=0; host->h_aliases[i]; i++) PRT("別名 = %s\n",host->h_aliases[i]);
		for (i=0; host->h_addr_list[i]; i++) {
			IN_ADDR *ip;
			ip = (IN_ADDR *)host->h_addr_list[i];
			PRT("IP = %s\n",inet_ntoa(*ip));
		}
	} else PRT("host = NULL,sIP=%s\n",sNngsIP[0]);

	// 非同期で通信する場合
	// 接続、送信、受信、切断のイベントをウィンドウメッセージで通知されるように
	if ( fUseNngs && WSAAsyncSelect(Sock, ghWindow, WM_USER_ASYNC_SELECT, FD_CONNECT | FD_WRITE | FD_READ | FD_CLOSE) == SOCKET_ERROR ) {
		PRT("ソケットイベント通知設定に失敗\n");
		return 0;
	}

	// サーバーのIPアドレスとポート番号の指定
	memset(&Addr, 0, sizeof(Addr));
	Addr.sin_family      = AF_INET;
	Addr.sin_port        = htons(nPort);
	Addr.sin_addr.s_addr = addrIP;

	// サーバーへ接続	 
	if ( connect( Sock, (LPSOCKADDR)&Addr, sizeof(Addr) ) == SOCKET_ERROR ) {
		if ( WSAGetLastError() != WSAEWOULDBLOCK ) {
			PRT("Error: connect(): \n");
			return 0;
		}
	}

//	close_nngs();	// 接続の切断
	return 1;
}

// 接続の切断
void close_nngs(void)
{
	if ( Sock != INVALID_SOCKET ) closesocket(Sock);
	Sock = INVALID_SOCKET;
	WSACleanup();
}



HWND hNngsWaitDlg = NULL;
#define IDM_NNGS_MES 1000

// 非同期のSelectを受け取る。nngs用
void OnNngsSelect(HWND /*hWnd*/, WPARAM wParam, LPARAM lParam)
{
	static char buf[TMP_BUF_LEN];
	static int buf_size = 0;

	if ( WSAGETASYNCERROR(lParam) != 0 ) {
		PRT("ソケットイベントの通知でエラー=%d\n",WSAGETSELECTERROR(lParam));
//		WSAECONNABORTED  (10053)
		closesocket(Sock);		// ソケットを破棄
		Sock = INVALID_SOCKET;
		return;
	}
	if ( Sock != (SOCKET)wParam) return;	// 処理すべきソケットか判定

	switch (WSAGETSELECTEVENT(lParam)) {
		case FD_CONNECT:	// 接続に成功	
			PRT("サーバに接続\n");
			buf_size = 0;
			break;

		case FD_CLOSE:		// 接続を切られた
			closesocket(Sock);
			Sock = INVALID_SOCKET;
			PRT("サーバから切断された\n");
			break;

		case FD_WRITE:		// 送信バッファに空きができた
			PRT("FD_WRITE\n");
			break;

		case FD_READ:		// データを受信する
//			PRT("FD_READ size=%d\n",buf_size);
			int nRecv = recv( Sock, buf + buf_size, 1, 0);
			if ( nRecv == SOCKET_ERROR ) {		// 多分ブロックしただけ？なので何もしない
				PRT("Error=%d recv()\n",WSAGetLastError());	
				break;
			}
			buf_size += nRecv;
			if ( buf_size >= TMP_BUF_LEN ) {
				PRT("buf over! size=%d\n",buf_size);
				buf_size = 0;
				break;
			}
			if ( buf_size <= 0 ) break;
			buf[buf_size] = 0;

			if ( strcmp(buf,"Login: ") == 0 ) {
				PRT("<---%s",buf);
				buf_size = 0;
				login_nngs(dll_col-1);	// loginする
				break;
			}

			if ( buf[buf_size-1] == '\n' ) {	// 1行受け取った
				buf_size = 0;
				PRT("<---%s",buf);
				if ( fPassWindows ) { PRT("思考中はnngsからの情報は無視\n"); break; }
				// 構文解析ルーチンへ飛ぶ
				int ret = nngsAnalysis(buf,dll_col-1);	// nngsの内容を解析
//				PRT("ret=%d\n",ret);
				if ( ret == NNGS_LOGIN_TOTYU ) break;
				if ( ret == NNGS_UNKNOWN     ) break;
				RetZ = ret;
				if ( hNngsWaitDlg == NULL ) {
					PRT("nngs待機ダイアログが出ていない\n");
					break;
				}
				PostMessage(hNngsWaitDlg,WM_COMMAND,IDM_NNGS_MES,0);	// 通信待ちダイアログを終了させる。
			}
			break;
	}
}


// nngsからの情報を待つ
BOOL CALLBACK NngsWaitProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			hNngsWaitDlg = hDlg;
			if ( lParam == 0 ) {	// 最初は接続してログインする
				if ( connect_nngs() == 0 ) EndDialog(hDlg,FALSE);	// ログイン失敗
			}
			return TRUE;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ) {
				case IDCANCEL:
					EndDialog(hDlg,FALSE);			// 中断で終了
					break;
				case IDM_NNGS_MES:
					EndDialog(hDlg,TRUE);			// 何らかのメッセージが来たので終了
					break;
			}
			return TRUE;

		case WM_DESTROY:
			hNngsWaitDlg = NULL;
			break;
	}
	return FALSE;
}
