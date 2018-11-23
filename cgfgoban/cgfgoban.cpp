/* cgfgoban.cpp
 * Go Program GUI
 * written by Hiroshi Yamashita
 * 2005/07/14
 */
#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#include "cgf.h"
#include "cgf_rc.h"		   

extern HWND ghWindow;					/* WINDOWSのハンドル  */

int ban_size = BAN_19;

int tesuu = 0;			// 手数
int all_tesuu = 0;		// 最後までの手数
int kifu[KIFU_MAX][3];	// 棋譜	[0] 位置, [1] 色, [2] 時間

int kou_iti;			// 次に劫になる位置
int hama[2];			// 黒と白の揚浜（取り石の数）

double komi = 6.5;		// コミは6目半。

 /*** エラー処理 ***/
void debug(void)
{
	PRT("debug!!!\n");
	PRT("手数=%d\n",tesuu);
	hyouji();

#ifdef CGF_E
	MessageBox( ghWindow, "CgfGoBan's Internal Error!\nIf you push OK, Program will be terminated by force.", "Debug!", MB_OK);
#else
	MessageBox( ghWindow, "内部エラーで停止しました", "デバッグ！", MB_OK);
#endif
	exit(1);
}

void hyouji(void)
{
	int k,x,y,z;
	char *st[5] = { "＋","●","○","★","☆" };	// 埋まったのが黒。空洞が白
//	PRT("   ＡＢＣＤＥＦＧＨＪＫＬＭＮＯＰＱＲＳＴ\n");
//	PRT(" 1 2 3 4 5 6 7 8 9 a b c d e f10111213\n");
	PRT("   ");
	for (x=1;x<ban_size+1;x++) PRT("%2x",x);
	PRT("\n");
	for (y=0;y<ban_size;y++) {
//		PRT("%2d:",y+1);
		PRT("%2x:",y+1);
		for (x=0;x<ban_size;x++) {
			z = ((y+1)<<8) + x+1 ;
			k = ban[z];
			PRT("%s",st[k]);
		}
		PRT("\n");
	}
}




// クリップボードに盤面をコピー
void hyouji_clipboard(void)
{
	int k,x,y,z,zz,n;
	HGLOBAL hMem;
	LPSTR lpStr;
	static char *cXn[20] = {"","Ａ","Ｂ","Ｃ","Ｄ","Ｅ","Ｆ","Ｇ","Ｈ","Ｊ","Ｋ","Ｌ","Ｍ","Ｎ","Ｏ","Ｐ","Ｑ","Ｒ","Ｓ","Ｔ"};
	static char *cYn[20] = {"","１","２","３","４","５","６","７","８","９","10","11","12","13","14","15","16","17","18","19"};

static char *cSn[] = {
"┏","┯","┓",
"┠","┼","┨",
"┗","┷","┛",
"┌","┬","┐",
"├","┼","┤",
"└","┴","┘",
};

/*
※一二三四五六七八九
01┌┬┬┬┬┬┬┬┐
02├┼┼┼┼┼┼┼┤
03├┼┼●┼┼●┼┤
04├┼┼┼┼┼┼┼┤
05├┼☆┼○┼┼┼┤
06├┼┼┼┼┼┼┼┤
07├┼●┼┼┼┼┼┤
08├┼┼┼┼┼┼┼┤
09└┴┴┴┴┴┴┴┘
*/
	char sTmp[256];

	hMem = GlobalAlloc(GHND, 1024);
	lpStr = (LPSTR)GlobalLock(hMem);

	lstrcpy( lpStr, "");
	
	if ( tesuu & 1 ) lstrcat(lpStr,"黒");
	else lstrcat(lpStr,"白");
	sprintf( sTmp,"%d手",tesuu);
	lstrcat(lpStr,sTmp);

	zz = kifu[tesuu][0];
	if ( zz != 0 ) {
		x = zz & 0x00ff;
		y = (zz & 0xff00) >> 8;
		sprintf(sTmp," %s-%s ",cXn[x],cYn[y]);
	} else sprintf(sTmp," PASS ");
	lstrcat(lpStr,sTmp);
	sprintf(sTmp,"まで  ハマ 黒%d子  白%d子\r\n",hama[0],hama[1]);
	lstrcat(lpStr,sTmp);

	sprintf(sTmp,"  ");
	lstrcat(lpStr,sTmp);
	for (x=0;x<ban_size;x++) {
		sprintf(sTmp,"%s",cXn[x+1]);
		lstrcat(lpStr,sTmp);
	}
	sprintf(sTmp,"\r\n");
	lstrcat(lpStr,sTmp);

	for (y=0;y<ban_size;y++) {
		sprintf(sTmp,"%s",cYn[y+1]);
		lstrcat(lpStr,sTmp);
		for (x=0;x<ban_size;x++) {
			z = ((y+1)<<8) + x+1 ;
			k = ban[z];
#if 0
			if ( k==1 ) {
				if ( z == zz ) sprintf(sTmp,"◆");
				else sprintf(sTmp,"●");		// 埋まったのが黒
			} else if ( k==2 ) {
				if ( z == zz ) sprintf(sTmp,"◇");
				else sprintf(sTmp,"○");	// 空洞が白
			} else {
				if ( (x==0 && y==0) || (x==0 && y==ban_size-1) || (x==ban_size-1 && y==ban_size-1) || (x==ban_size-1 && y==0) ) sprintf(sTmp,"・");
				else if ( x==0 || x==ban_size-1 ) sprintf(sTmp,"｜");
				else if ( y==0 || y==ban_size-1 ) sprintf(sTmp,"－");
				else sprintf(sTmp,"＋");
			}
#else
			//┏┓┗┛┯┨┷┠━┃┼
			if ( k==1 ) {
				if ( z == zz ) sprintf(sTmp,"★");
				else sprintf(sTmp,"●");		// 埋まったのが黒
			} else if ( k==2 ) {
				if ( z == zz ) sprintf(sTmp,"☆");
				else sprintf(sTmp,"○");	// 空洞が白
			} else {
				if ( x==0 && y==0 ) n = 0;
				else if ( x==0 && y==ban_size-1 ) n = 6;
				else if ( x==ban_size-1 && y==ban_size-1 ) n = 8;
				else if ( x==ban_size-1 && y==0) n = 2;
				else if ( x==0          ) n = 3;
				else if ( x==ban_size-1 ) n = 5;
				else if ( y==0          ) n = 1;
				else if ( y==ban_size-1 ) n = 7;
				else n = 4;
//				sprintf(sTmp,"%s",cSn[n+0]);
				sprintf(sTmp,"%s",cSn[n+9]);
			}
#endif
			lstrcat(lpStr,sTmp);
		}
		sprintf(sTmp,"\r\n");
		lstrcat(lpStr,sTmp);
	}


	GlobalUnlock(hMem);
	if ( OpenClipboard(ghWindow) ) {
		EmptyClipboard();
		SetClipboardData( CF_TEXT, hMem );
		CloseClipboard();
		PRT("クリップボードに盤面(手数=%d)を出力しました。\n",tesuu);
	}
}


// SGFの棋譜をクリップへ
void SGF_clipout(void)
{
	HGLOBAL hMem;
	LPSTR lpStr;

	SaveSGF();	// SGFを書き出す

	hMem = GlobalAlloc(GHND, strlen(SgfBuf)+1);
	lpStr = (LPSTR)GlobalLock(hMem);
	strcpy(lpStr,SgfBuf);

	GlobalUnlock(hMem);
	if ( OpenClipboard(ghWindow) ) {
		EmptyClipboard();
		SetClipboardData( CF_TEXT, hMem );
		CloseClipboard();
//		PRT("クリップボードにSGFを出力しました。\n");
	}
}


#define SBC_NUM 31	// クリップボードから読み込む時の文字の種類
static char *sBC[SBC_NUM] = {
"●","◆","★",
"○","◇","☆","◯",	// 0x819b, 0x81fc,コードが違う。
"┏","┯","┓",
"┠","┼","┨",
"┗","┷","┛",
"・","－","＋",
"｜","－","＋",
"┌","┬","┐",
"├","╋","┤",
"└","┴","┘",
			
};

// クリップボードから盤面を読み込む
void clipboard_to_board(void)
{
	int i,j,k,y,z,flag=0,nLen,sgf_start=0;
	char c,*p;
	HGLOBAL hMem,hClipMem;
	LPSTR lpStr,lpClip;

	if ( OpenClipboard(ghWindow) == FALSE ) {
		PRT("クリップボードオープンの失敗です。\n");
		return;
	}

	hClipMem = GetClipboardData( CF_TEXT );
	hMem = GlobalAlloc( GHND, GlobalSize( hClipMem) );
	lpStr  = (LPSTR)GlobalLock(hMem);
	lpClip = (LPSTR)GlobalLock(hClipMem);
	lstrcpy( lpStr, lpClip );
	GlobalUnlock(hMem);
	GlobalUnlock(hClipMem);
	CloseClipboard();


	// 1文字づつ取り出して比較、検討。
	// 改行があれば停止
	nLen = strlen( lpStr );
	PRT("クリップボードから読み込み。バイト数=%d\n",nLen);

	// SGFファイルかどうかの判定をする。
	// 文字列の中に(;が連続して出現すれば、最初の(から読み込みを開始。改行文字列は無視する。
	for (i=0;i<nLen;i++) {
		c = *(lpStr+i);	// 1バイト読み取る。
		if ( c=='(' ) {
			sgf_start = i; 
			flag = 1;
			continue;
		}
		if ( c < 0x20 ) continue;	// 無視
		if ( c==';' && flag ) { PRT("SGF file!\n"); break; }
		flag = 0;
	}
	if ( i != nLen ) {
		// SGF用のバッファにコピー
		nSgfBufSize = 0;
		for (i=sgf_start;i<nLen;i++) {
			SgfBuf[i-sgf_start] = *(lpStr+i);
			nSgfBufSize++;
			if ( nSgfBufSize >= SGF_BUF_MAX ) break;
		}
		LoadSetSGF(1);	// SGFをロードして盤面を構成する
		return;
	}
	

	// 盤面。解析する
//	x = 0;
	y = 0;
//	flag = 0;	// 0...Ｙ座標を検索中。1...19個の石を読み取り中。2...改行まで読み込み中。

	for (i=0;i<nLen-1;i++) {
		c = *(lpStr+i);	// 1バイト読み取る。
		if ( c=='\n' || c=='\r' ) continue;		// 1行終了
//		if ( flag == 0 && x >= 4 ) continue;	// 4文字以内にＹ座標がなければ改行まで無視する。
//		if ( flag == 2 ) continue;				// 改行を待つだけ

		// 19回連続して盤面テキストが出現すればそこを盤面とする。
		for (j=0;j<19;j++) {
			p = lpStr + i + j*2;
			if ( i+j*2 >= nLen ) break;
			for (k=0;k<SBC_NUM;k++) if ( strncmp(p,sBC[k],2) == 0 ) break;
			if ( k == SBC_NUM ) break;	// 盤面テキストに不一致
//			PRT("i=%d,y=%d,j=%d,k=%d\n",i,y,j,k);
		}
		if ( j==19 && y < 19 ) {	// 新しい行を発見！
			y++;
			for (j=0;j<19;j++) {
				p = lpStr + i + j*2;
				z = y*256 + (j+1);
				ban[z] = 0;
				for (k=0;k<3;k++) if ( strncmp(p,sBC[k],2) == 0 ) break;
				if ( k!=3 ) ban[z] = 1;
				for (k=3;k<7;k++) if ( strncmp(p,sBC[k],2) == 0 ) break;
				if ( k!=7 ) ban[z] = 2;
			}
			i+= 19*2 -1;
//			PRT("i=%d,flag=%d,y=%d,j=%d\n",i,flag,y,j);
		}
	}

	GlobalFree(hMem);

	PostMessage(ghWindow,WM_COMMAND,IDM_SET_BOARD,0L);	// 画面の再構成

/*
 ０ 盤の配列を用意し、初期化しておく。

 １ 先頭の半角空白、全角空白をカット。

 ２ 次の２文字を配列と比較してＹ座標を返す
    '01'  ' 1'  '１'  'ａ' なら１行目
    '02'  ' 2'  '２'  'ｂ' なら２行目
    ・・・
    ただし、'ｉ' がある（琵琶子のように）。

 ３ 次から２文字ずつ見ていく。１９回かまたは文字列がなくなるまで。
    '●'  '◆' なら黒石
    '○'  '◇' なら白石
    盤に石を上書きしていく。

 ゴミは無視する。足りなければ上書きされないまで。

 Ｙ座標のカウンタをもっておいて、そのカウンタより小さいときは無視する。
*/

}



/*** 盤面の石情報のみから連構造体の再構築 ***/
void reconstruct_ban(void)
{
	int k,x,y,z;
	
	/*** それぞれの初期盤面状態を、ban_19[]に一端保存 ***/
	for (y=0;y<ban_size+2;y++) for (x=0;x<ban_size+2;x++) {
		z = y*256 + x;
		k = ban[z];
		if      ( ban_size == BAN_19 ) ban_19[y][x] = k;
		else if ( ban_size == BAN_13 ) ban_13[y][x] = k;
		else                           ban_9 [y][x] = k;
	}
	init_ban();	// 初期設定を呼ぶ
}

// 初期盤面の値をコピーするだけ
int init_ban_sub(int x,int y)
{
	int k;
	
	if      ( ban_size == BAN_19 ) k = ban_19[y][x];
	else if ( ban_size == BAN_13 ) k = ban_13[y][x];
	else if ( ban_size == BAN_9  ) k = ban_9 [y][x];
	else                           k = ban_9 [y][x];
	if ( x==0 || y==0 || x>ban_size || y>ban_size ) k = 3;	// 盤外は強制的に枠にする。
	else if ( k == 3 ) k = 0;	// 盤内なのに枠のときは強制的に空白に。
	return k;
}


/*** 初期盤面状態から連のデータを作る。初期設定いろいろ ***/
void init_ban(void)
{
	int k,x,y,z;

	kou_iti = 0;		// 劫になる位置
	tesuu = 0;
	hama[0] = 0;		// 揚浜（黒の取り石）
	hama[1] = 0;
	total_time[0] = total_time[1] = 0;	// 累計の思考時間

	/*** それぞれの初期盤面状態を、ban[]に読み込む ***/
	// まず、外枠だけ作る。でないと下にある石が呼吸点を盤外に持ってしまう。
	for (y=0;y<ban_size+2;y++) for (x=0;x<ban_size+2;x++) {
		z = y*256 + x;
		k = init_ban_sub(x,y);	// 初期盤面の値をコピーするだけ
			
		if ( k == 3 ) ban[z] = k;
		else          ban[z] = 0;

//		if ( ban[z] == 3 ) ban_start[z] = 3;	// 初期盤面は枠だけ
	}

	for (y=0;y<ban_size+2;y++) for (x=0;x<ban_size+2;x++) {
		z = y*256 + x;
		k = init_ban_sub(x,y);	// 初期盤面の値をコピーするだけ
			
		if ( k == 0 || k == 3 ) ban[z] = k;
		else {
			if ( make_move(z,k) != MOVE_SUCCESS ) {
				PRT("初期盤面にエラー発生！！！ z = %x\n",z);
//				hyouji();
			}
		}
		ban_start[z] = ban[z];	// 初期盤面にもここで代入する
	}
}


/**************************************************/

int ban[BANMAX];		// ban[5376];	// = 21*256
int ban_start[BANMAX];	// 開始局面を常に記憶

int ban_9[11][11] = {
/*   A B C D E F G H I    */
 { 3,3,3,3,3,3,3,3,3,3,3 },
 { 3,0,0,2,1,0,1,1,2,0,3 }, // 1
 { 3,0,2,2,1,0,1,2,2,0,3 },	// 2
 { 3,2,2,1,0,1,0,1,2,0,3 },	// 3
 { 3,2,1,1,1,1,1,0,1,2,3 },	// 4
 { 3,1,2,2,1,0,1,1,2,0,3 },	// 5
 { 3,0,1,2,2,1,0,1,2,0,3 },	// 6 
 { 3,1,1,1,2,2,1,1,2,0,3 },	// 7
 { 3,1,0,0,1,2,1,1,2,0,3 },	// 8 
 { 3,0,1,1,1,2,2,2,2,0,3 },	// 9 
 { 3,3,3,3,3,3,3,3,3,3,3 }
};
int ban_13[15][15] = {
/*   A B C D E F G H I J K L M  */
 { 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 },
 { 3,0,1,1,1,2,0,2,1,0,0,0,0,0,3 },
 { 3,1,0,1,2,2,2,1,1,1,0,0,0,0,3 },
 { 3,1,1,2,2,0,2,2,2,1,0,1,0,0,3 },
 { 3,2,2,2,0,0,0,0,2,2,1,0,0,0,3 },
 { 3,1,2,0,0,0,0,0,0,2,2,1,0,0,3 },
 { 3,1,1,2,2,0,0,0,0,2,1,1,1,0,3 },
 { 3,0,0,1,2,2,0,0,0,2,2,2,1,1,3 },
 { 3,0,1,0,1,2,0,0,0,2,1,1,2,2,3 },
 { 3,0,1,0,1,2,0,0,0,2,1,0,2,0,3 },
 { 3,0,0,1,2,0,0,0,0,2,1,1,2,2,3 },
 { 3,1,1,2,2,2,2,2,2,2,2,1,1,1,3 },
 { 3,2,2,1,1,1,2,1,1,1,2,2,2,1,3 },
 { 3,0,2,0,1,0,1,0,1,0,1,1,1,0,3 },
 { 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 }
};
int ban_19[21][21] = {
/*   A B C D E F G H J K L M N O P Q R S T */
/*   1 2 3 4 5 6 7 8 9 a b c d e f10111213	 */
 { 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 },

 { 3,0,0,0,0,0,0,0,0,0,0,0,0,2,1,0,2,1,1,0,3 },
 { 3,0,0,0,0,1,0,0,2,0,2,0,1,0,1,1,1,1,2,1,3 },
 { 3,0,0,1,0,0,2,2,0,2,1,1,0,1,2,1,2,1,2,0,3 },
 { 3,0,0,0,0,0,2,0,0,2,1,1,2,0,2,2,2,2,1,1,3 },
 { 3,0,1,0,1,1,2,0,2,2,2,1,2,0,0,0,2,0,2,0,3 },
 { 3,0,0,2,1,2,2,2,0,2,1,1,1,0,0,0,2,1,2,0,3 },
 { 3,0,1,2,1,1,2,0,1,2,2,1,0,1,1,2,0,2,0,0,3 },
 { 3,0,1,2,1,0,2,1,0,1,2,2,2,1,0,1,2,0,0,0,3 },
 { 3,0,1,2,2,2,0,2,2,2,1,2,2,1,1,2,2,0,0,0,3 },
 { 3,1,0,1,0,2,0,1,2,2,1,2,1,2,2,2,0,2,0,0,3 },
 { 3,0,1,2,2,1,1,0,1,2,1,2,1,2,0,1,1,2,0,0,3 },
 { 3,0,1,2,1,2,0,0,1,2,1,1,1,2,0,0,1,1,2,0,3 },
 { 3,0,2,0,1,2,0,1,1,1,0,2,1,2,0,1,0,2,2,0,3 },
 { 3,0,0,0,2,2,1,2,2,1,0,0,2,0,2,0,1,2,1,0,3 },
 { 3,0,0,2,0,0,2,2,1,0,0,0,1,0,2,0,1,0,1,0,3 },
 { 3,0,0,2,2,2,0,2,1,0,0,1,2,0,1,0,0,1,0,0,3 },
 { 3,0,2,1,0,2,2,1,1,2,1,1,0,0,0,1,0,0,0,0,3 },
 { 3,0,2,1,1,0,2,2,2,1,1,0,0,0,0,0,0,0,0,0,3 },
 { 3,0,0,0,0,1,0,0,0,2,0,0,0,0,0,0,0,0,0,0,3 },

 { 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 }
};
