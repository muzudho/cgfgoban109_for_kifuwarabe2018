// cgf_pipe.cpp
// GnuGoとGTPで対戦するため
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <io.h>

#include "cgf.h"
#include "cgf_pipe.h"

#define BUF_GTP_SIZE 2048
char BufGetGTP[BUF_GTP_SIZE];
int fGnugoPipe = 0;


// GTPのエンジン gnugo.exe が存在してるか
int IsGtpEngineExist(void)
{
	char str[MAX_PATH];
	
	strcpy(str,sGtpPath[0]);
	char *p = strstr(str," ");	// "Program Files"で失敗する
	if ( p != NULL ) *p = 0;

	struct _finddata_t c_file;
	long hFile = _findfirst( str, &c_file );
	if ( hFile == -1L ) return 0;
	return 1;
}


SECURITY_ATTRIBUTES sa;
HANDLE hGnuOutRead, hGnuOutWrite;	// GnuGoからの出力を読み取るためのパイプ
HANDLE hGnuInRead,  hGnuInWrite;	// Gnuへ入力を送るためのパイプ
HANDLE hGnuErrRead, hGnuErrWrite;	// Gnuのエラーを読み取るため
HANDLE hGnuOutReadDup,hGnuInWriteDup,hGnuErrReadDup;	// 親でコピーして使うため。

STARTUPINFO si;
PROCESS_INFORMATION pi;

// パイプを作成
int open_gnugo_pipe()
{
	fGnugoPipe = 0;
//	if ( IsGtpEngineExist() == 0 ) return 0;

	// 匿名パイプを作成
//	SECURITY_ATTRIBUTES sa;
	sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle       = TRUE;
	sa.lpSecurityDescriptor = NULL;
//	HANDLE hGnuOutRead, hGnuOutWrite;	// GnuGoからの出力を読み取るためのパイプ
//	HANDLE hGnuInRead,  hGnuInWrite;	// Gnuへ入力を送るためのパイプ
//	HANDLE hGnuErrRead, hGnuErrWrite;	// Gnuのエラーを読み取るため
//	HANDLE hGnuOutReadDup,hGnuInWriteDup;	// 親でコピーして使うため。

	// GnuGoの入力(STDIN)用のパイプを作る。
	CreatePipe(&hGnuInRead, &hGnuInWrite, &sa, 0);
	DuplicateHandle(GetCurrentProcess(), hGnuInWrite, GetCurrentProcess(), &hGnuInWriteDup, 0,
			     FALSE,	/* 継承しない */ DUPLICATE_SAME_ACCESS);
	CloseHandle(hGnuInWrite);	// 以降はコピーを使うので閉じる。

	// GnuGoの出力(STDOUT)用のパイプを作る。
	CreatePipe(&hGnuOutRead, &hGnuOutWrite, &sa, 0);
	// ハンドルのコピーを作って、親ではこちらを利用する。子に継承されないように。
	DuplicateHandle(GetCurrentProcess(), hGnuOutRead, GetCurrentProcess(), &hGnuOutReadDup, 0,
			     FALSE,	/* 継承しない */ DUPLICATE_SAME_ACCESS);
	CloseHandle(hGnuOutRead);	// 以降はコピーを使うので閉じる。
#if 0	// コメントアウトするとMoGoが動くようになる。大量のstderrを吐き出してるから？
	// GnuGoのエラー(STDERR)用のパイプを作る。
	CreatePipe(&hGnuErrRead, &hGnuErrWrite, &sa, 0);
	DuplicateHandle(GetCurrentProcess(), hGnuErrRead, GetCurrentProcess(), &hGnuErrReadDup, 0,
			     FALSE,	/* 継承しない */ DUPLICATE_SAME_ACCESS);
	CloseHandle(hGnuErrRead);	// 以降はコピーを使うので閉じる。
#endif

	// プロセスに渡す情報の準備
//	STARTUPINFO si;
//	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb          = sizeof(STARTUPINFO);
	si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
//	si.wShowWindow = SW_SHOWDEFAULT;
	si.wShowWindow = SW_HIDE;	// コマンドプロンプトの画面を隠す場合。
	si.hStdInput   = hGnuInRead;	// GnuGoへの入力はこのハンドルに送られる。パイプで繋がってるのでhGnuInWriteDupから命令を送る。
	si.hStdOutput  = hGnuOutWrite;	// GnuGoの  出力はこのハンドルに送られる。パイプで繋がってるのでhGnuOutReadDupで読み取れる。
	si.hStdError   = hGnuErrWrite;	// GnuGoのエラー出力も送る場合。    　    パイプで繋がってるのでhGnuErrReadDupで読み取れる。

	// GnuGoを起動
//	CreateProcess(NULL, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --clock 2400 --level 12 --autolevel", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
//	CreateProcess(NULL, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --clock 300 --level 12 --autolevel", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
//	CreateProcess(NULL, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --level 12", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
//	CreateProcess(NULL, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	PRT("%s\n",sGtpPath[0]);
	int fRet = CreateProcess(NULL, sGtpPath[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	if ( fRet == 0 ) { PRT("Fail CreateProcess\n"); return 0; }
//	PRT("c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --level 12");

	// もう使わないハンドルを閉じる。
	CloseHandle(pi.hThread);
	CloseHandle(hGnuInRead);
	CloseHandle(hGnuOutWrite);
	CloseHandle(hGnuErrWrite);

	Sleep(500);	// 安全のために待つ。
//	WaitForInputIdle(pi.hProcess, INFINITE);	// GnuGoが入力を受け取れるようになるまで待つ ---> CgfGobanをCgfGobanから呼ぶと固まる。

	PRT("open gnugo pipe...\n");
	fGnugoPipe = 1;
	return 1;
//	test_gnugo_loop();
}

void close_gnugo_pipe()
{
    // 子プロセスのハンドルを閉じる。
	CloseHandle(pi.hProcess);
	// GnuGoとの通信で使っていたハンドルを閉じる。
	CloseHandle(hGnuInWriteDup);
	CloseHandle(hGnuOutReadDup);
	CloseHandle(hGnuErrReadDup);
	fGnugoPipe = 0;
}

// パイプからGnuGoの出力を読み出す
int ReadGTP() {
	int n = 0;
	char szOut[2],c;
	DWORD dwRead;
  	char buf[BUF_GTP_SIZE];

	for (;;) {
		if ( ReadFile(hGnuOutReadDup, szOut, 1, &dwRead, NULL) == 0 ) {
			PRT("ReadFile Err\n");
			return 0;
		}
		if ( dwRead == 0 ) continue;
		c = szOut[0];
		if ( c != '\n' && c < 32 ) continue;	// '\r' とかは無視
		if ( c == 9 ) c = 32;	// TABをSPACEに変換
		buf[n]=c;
		n++;
		if ( n >= BUF_GTP_SIZE ) { PRT("Over!\n"); debug(); }
//		PRT("%c(%x)",c,c);
		if ( c == '\n' ) break;	// コマンドが終了した
	}

	buf[n] = 0;
	PRT("ReadGTP %s",buf);
	if ( buf[0] == '#') return 1;	// コメントは無視
	if ( buf[0] == '\n' ) return 1;	// 改行だけの行は区切り。まだデータが来る
	strcpy(BufGetGTP,buf);
	return 2;
}

// コマンドを受信する。手の受信も兼ねる。Errorの場合は0を返す。
int ReadCommmand(int *z)
{
	int x=0,y=0,fRet;
	char move[BUF_GTP_SIZE];

	*z = 0;
	
	for (;;) {
		fRet = ReadGTP();	// パイプからGnuGoの出力を読み出す
		if ( fRet == 0 ) return 0;
		if ( fRet == 1 ) break;	// 正常終了
//		if ( BufGetGTP[0] == '\n' ) break;	// 完全終了
		if ( BufGetGTP[0] == '=' && strlen(BufGetGTP) > 2 ) {
			memset(move,0,sizeof(move));
			sscanf(BufGetGTP+2,"%s",move);	// id は無視
			PRT("move=%s\n",move);
			if      ( stricmp(move,"pass")   ==0 ) *z =  0;//GTP_MOVE_PASS;		// GnuGoは"PASS"と送ってくる。小文字では？
			else if ( stricmp(move,"resign") ==0 ) *z = -1;//GTP_MOVE_RESIGN;	// 投了の場合(nngsに送って大丈夫？相手が落ちるかも）SGMPは非対応。
			else {			   // "Q4" なら
				x = move[0];
				if ( x > 'I' ) x--;	// 'I'は存在しないので
				x =	x - 'A' + 1;
				y = atoi(move+1);
			*z = (ban_size+1-y)*256 + x;
			}
			PRT("x=%d,y=%d,z=%04x\n",x,y,*z);
		}
	}
	return 1;
}


void SendGTP(char *str)
{
	DWORD dwRead;
	WriteFile(hGnuInWriteDup, str, strlen(str), &dwRead, NULL);
	PRT("SendGTP %s",str);
}

// パイプからGnuGoのエラー出力を読み出す
int ReadGnuGoErr() {
	char szOut[BUF_GTP_SIZE];
	DWORD dwRead;

	// 同様にエラー出力の処理
	PeekNamedPipe(hGnuErrReadDup, NULL, 0, NULL, &dwRead, NULL);
	if (dwRead > 0) {
		if ( ReadFile(hGnuErrReadDup, szOut, BUF_GTP_SIZE-1, &dwRead, NULL) == 0 ) {
			PRT("ReadFile GnuGoErr Err\n");
			return FALSE;
		}
		szOut[dwRead] = 0;
		PRT("GnuGoStdErr->%s",szOut);
	}
	return TRUE;
}


void PaintUpdate(void);	// 画面の全書き換え

extern int fAutoSave;		// 棋譜の自動セーブ（通信対戦時）


// 棋譜をセットして手を進めて画面を更新。エラーの場合は0以外を返す。
int set_kifu_time(int z,int col,int t)
{
	int k;
	PRT("set_kifu_time z=(%2d,%2d)=%04x,col=%d,tesuu=%d,t=%d\n",z&0xff,z>>8,z,col,tesuu,t);
	if ( z < 0 ) return MOVE_RESIGN;

	k = make_move(z,col);
	if ( k != MOVE_SUCCESS ) {
		int x = z & 0xff;
		int y = z >> 8;
		PRT("相手がルール違反かな、と思うけどx,y,z=%d,%d,%04x\n",x,y,z);
		return k;
	}

	tesuu++;	all_tesuu = tesuu;
	kifu[tesuu][0] = z;		// 棋譜を記憶
	kifu[tesuu][1] = col;	// 石の色
	kifu[tesuu][2] = t;		// 思考時間

	if ( z==0 ) {
		if ( tesuu > 1 && kifu[tesuu-1][0] == 0  ) {
			PRT("２回連続パスですね\n");
			return MOVE_PASS_PASS;
		}
	}
	if ( tesuu > 2000 ) {
		PRT("手数が2000手を超えました\n");
		return MOVE_KIFU_OVER;
	}

	PaintUpdate();
	return MOVE_SUCCESS;
}

// 経過時間を秒で返す(double)
double GetSpendTime(clock_t start_time)
{
	return (double)(clock() - start_time) / CLOCKS_PER_SEC;
}

// 累計の思考時間を計算しなおすだけ。現在の手番の累計思考時間を返す
int count_total_time(void)
{
	int i,n;
	
	total_time[0] = 0;	// 先手の合計の思考時間。
	total_time[1] = 0;	// 後手
	for (i=1; i<=tesuu; i++) {
		if ( (i & 1) == 1 ) n = 0;	// 先手
		else                n = 1;
		total_time[n] += kifu[i][2];
	}
	return total_time[tesuu & 1];		// 思考時間。先手[0]、後手[1]の累計思考時間。
}


// 座標を'K15'のような文字列に変換
void change_z_str(char *str, int z)
{
	int x,y;

	if ( z <  0 ) sprintf(str,"resign");
	else if ( z == 0 ) sprintf(str,"pass");
	else {
		x =  z & 0x00ff;
		y = (z & 0xff00) >> 8;
		x = 'A' + x - 1;
		if ( x >= 'I' ) x++;
		y = ban_size - y + 1;
		sprintf(str,"%c%d",x,y);
	}
}

// GnuGoの初期化
int init_gnugo_gtp(char *sName)
{
	const int NAME_STR_MAX = 64;
	int z;
	char str[256];
	sprintf(str,"boardsize %d\n",ban_size);	// 19路盤に
	SendGTP(str);
	if ( ReadCommmand(&z) == 0 ) return 0;	// パイプから結果を読み出す
	SendGTP("clear_board\n");				// 盤面を初期化
	if ( ReadCommmand(&z) == 0 ) return 0;
	sprintf(str,"komi %.1f\n",komi);		// コミを設定する
	SendGTP(str);
	if ( ReadCommmand(&z) == 0 ) return 0;
//	SendGTP("list_commands\n");	if ( ReadCommmand(&z) == 0 ) return 0;
//	SendGTP("get_komi\n");		if ( ReadCommmand(&z) == 0 ) return 0;

	SendGTP("name\n");	
	if ( ReadCommmand(&z) == 0 ) return 0;
	PRT("name=%s\n",BufGetGTP+2);
	if ( strlen(BufGetGTP+2) > NAME_STR_MAX ) *(BufGetGTP+2 + NAME_STR_MAX) = 0;
	strcpy(sName,BufGetGTP+2);
	int n = strlen(sName);
	if ( n > 0 && sName[n-1] < ' ' ) sName[n-1] = 0;
	SendGTP("version\n");
	if ( ReadCommmand(&z) == 0 ) return 0;
	PRT("version=%s\n",BufGetGTP+2);
	if ( strlen(BufGetGTP+2) > NAME_STR_MAX ) *(BufGetGTP+2 + NAME_STR_MAX) = 0;	// MoGoのバージョンが長すぎる
	strcat(sName," ");
	strcat(sName,BufGetGTP+2);
	n = strlen(sName);
	if ( n > 0 && sName[n-1] < ' ' ) sName[n-1] = 0;
	return 1;
}

// Gnugoに打たせる
int PlayGnugo(int /*endgame_type*/)
{
	int z;
	int black_turn = (GetTurnCol() == 1);

	if ( black_turn ) SendGTP("genmove black\n");
	else              SendGTP("genmove white\n");
	if ( ReadCommmand(&z) == 0 ) {
		PRT("Gnugo GTP Err\n");
		return 0;
	}
	return z;
}

// Gnugoの盤面に石を置く
void PutStoneGnuGo(int z, int black_turn)
{
	int x;
	char str[BUF_GTP_SIZE],str2[BUF_GTP_SIZE];

	if ( z <  0 ) return;
	change_z_str(str, z);
	if ( black_turn ) sprintf(str2,"play black %s\n",str);
	else              sprintf(str2,"play white %s\n",str);
	SendGTP(str2);
	ReadCommmand(&x);	// OKの'='を受信
}

// Gnugo側の盤面も動かす
void UpdateGnugoBoard(int z)
{
	int black_turn = (GetTurnCol() == 1);

	PutStoneGnuGo(z, black_turn);
}

static const char *status_names[6] =
 {"alive", "dead", "seki", "white_territory", "black_territory", "dame"};

// Gnugoの死活判定で死活表示を行う
void FinalStatusGTP(int *p_endgame)
{
	char str[BUF_GTP_SIZE],str2[BUF_GTP_SIZE];
	int x,y,z,i,k;

	if ( fGnugoPipe==0 ) {
		if ( open_gnugo_pipe()==0 ) return;
		//Gnugo側の盤面を初期化して石を置く
		init_gnugo_gtp(str);
		for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
			z = ((y+1)<<8) + x+1;
			k = ban[z];
			if ( k==0 ) continue;
			PutStoneGnuGo(z, (k==1));
		}
	}

#if 1	// MoGoでは非対応なので
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + x+1 ;
		change_z_str(str, z);
		sprintf(str2,"final_status %s\n",str);
		SendGTP(str2);
		k = 0;
		if ( ReadCommmand(&k) == 0 ) break;	// パイプから結果を読み出す
		for (i=0;i<6;i++) {
			if ( strstr(BufGetGTP+2,status_names[i]) != NULL ) break;
		}
		*(p_endgame+z) = i;
	}
#else

	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + x+1;
		k = ban[z];
		int status = 0;	// 初期状態は全部活
		if ( k==0 ) status = 5;	// dame
		*(p_endgame+z) = status;
	}
	SendGTP("final_status_list dead\n");	// 死石のみ、goguiのscoreがこの仕様なので

	int n = 0;
  	char buf[BUF_GTP_SIZE];

	for (;;) {
		DWORD dwRead;
		char szOut[1],c;
		if ( ReadFile(hGnuOutReadDup, szOut, 1, &dwRead, NULL) == 0 ) {
			PRT("ReadFile Err\n");
			return;
		}
		if ( dwRead == 0 ) continue;
		c = szOut[0];
		if ( c != '\n' && c < 32 ) continue;	// '\r' とかは無視
		if ( c == 9 ) c = 32;	// TABをSPACEに変換
		buf[n]=c;
		n++;
		if ( n >= BUF_GTP_SIZE ) { PRT("Over!\n"); debug(); }
//		PRT("%c(%x)",c,c);
//		if ( c == '\n' ) break;	// コマンドが終了した
		if ( n > 2 && buf[n-1]=='\n' && buf[n-2]=='\n' && buf[n-3]=='\n' ) break;
	}

	buf[n] = 0;
	PRT("ReadGTP %s",buf);
//	strcpy(BufGetGTP,buf);
	return;
/*

	for (;;) {
		if ( ReadCommmand(&z)==0 ) break;
	}
	PRT("%s",BufGetGTP);
*/
//		if ( (c == '\n' && n==1) || (c=='\n' && n>1 && buf[n-2]=='\n') ) break;	// コマンドが終了した
//	PRT("%s",buf);
//	for (;;) {
//		int fRet = ReadGTP();	// パイプからGnuGoの出力を読み出す
//		if ( fRet == 2 ) break;
//	}

//	if ( BufGetGTP[0] == '=' && strlen(BufGetGTP) > 2 ) {
#endif
}





#if 0
void test_gnugo_loop(void)
{
	int fRet = 0;
	int i,x,z;
	clock_t ct1;

	fAutoSave = 1;		// 棋譜の自動セーブ（通信対戦時）

	for (;;) {

		// GnuGoの初期化
		if ( init_gnugo_gtp() == 0 ) return;

//		PRT("\\r=%d,\\n=%d\n",'\r','\n');
		i = 0;
		if ( dll_col == 1 ) goto cgf_first;

		for (i=0;i<1000;i++) {
			char str[BUF_GTP_SIZE],str2[BUF_GTP_SIZE];

			// パイプからGnuGoのエラー出力を読み出す
//			if ( ReadGnuGoErr() == FALSE ) break;	// うまく読み取れない？

			ct1 = clock();	// 探索開始時間。1000分の１秒単位で測定。

			if ( tesuu&1 ) SendGTP("genmove white\n");
			else           SendGTP("genmove black\n");
			if ( ReadCommmand(&z) == 0 ) break;
		
			fRet = set_kifu_time(z,GetTurnCol(),(int)GetSpendTime(ct1));
			if ( fRet ) break;
cgf_first:

			ct1 = clock();	// 探索開始時間。1000分の１秒単位で測定。
#if 0
for (;;) {
	for (i=0;i<300;i++) {
		x = (rand() % 17) + 2;	// 2-18。盤の端には打たない。
		int y = (rand() % 17) + 2;	
		z = y * 256 + x;
		if ( ban[z] != 0 ) continue;
		if ( ban[z+1] && ban[z-1] && ban[z+0x100] && ban[z-0x100] ) continue;
		break;
	}
	if ( i==300 ) z = 0;
	break;
}
#else
			z = ThinkingDLL(0);	// 残り時間判定付きの思考ルーチン呼び出し
#endif

			if ( thinking_utikiri_flag ) break;

			change_z_str(str, z);
			if ( tesuu & 1 ) sprintf(str2,"play white %s\n",str);
			else             sprintf(str2,"play black %s\n",str);

			SendGTP(str2);
			if ( ReadCommmand(&x) == 0 ) break;	// OKの'='を受信

			fRet = set_kifu_time(z,GetTurnCol(),(int)GetSpendTime(ct1));
			if ( fRet ) break;
		}

		if ( thinking_utikiri_flag ) break;
		if ( fRet != 2 ) break;
		break;
	}
}
#endif
