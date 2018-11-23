/*** CFG_MAIN.C ***/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "cgf.h"

int check_ban[BANMAX];		// 既にこの石を検索した場合は1

int ishi = 0;	// 取った石の数(再帰関数で使う)
int dame = 0;	// 連のダメの数(再帰関数で使う)

int dir4[4] = { +0x001, +0x100, -0x001, -0x100 };


// 関数のプロトタイプ宣言
void count_dame(int tz);				// ダメと石の数を調べる
void count_dame_sub(int tz, int col);	// ダメと石の数える再帰関数
int get_z(int x,int y);					// (x,y)を1つの座標に変換

// (x,y)を1つの座標に変換
int get_z(int x,int y)
{
	return (y+1)*256 + (x+1);
}

// 位置 tz におけるダメの数と石の数を計算。結果はグローバル変数に。
void count_dame(int tz)
{
	int i;

	dame = ishi = 0;
	for (i=0;i<BANMAX;i++) check_ban[i] = 0;
	count_dame_sub(tz, ban[tz]);
}

// ダメと石の数える再帰関数
// 4方向を調べて、空白だったら+1、自分の石なら再帰で。相手の石、壁ならそのまま。
void count_dame_sub(int tz, int col)
{
	int z,i;

	check_ban[tz] = 1;			// この石は検索済み	
	ishi++;							// 石の数
	for (i=0;i<4;i++) {
		z = tz + dir4[i];
		if ( check_ban[z] ) continue;
		if ( ban[z] == 0 ) {
			check_ban[z] = 1;	// この空点は検索済み
			dame++;				// ダメの数
		}
		if ( ban[z] == col ) count_dame_sub(z,col);	// 未探索の自分の石
	}
}

// 石を消す
void del_stone(int tz,int col)
{
	int z,i;
	
	ban[tz] = 0;
	for (i=0;i<4;i++) {
		z = tz + dir4[i];
		if ( ban[z] == col ) del_stone(z,col);
	}
}

// 手を進める。z ... 座標、col ... 石の色
int make_move(int z, int col)
{
	int i,z1,sum,x,y,del_z = 0;
	int all_ishi = 0;	// 取った石の合計
	int un_col = UN_COL(col);

	if ( z == 0 ) {	// PASSの場合
		kou_iti = 0;
		return MOVE_SUCCESS;
	}
	if ( z == kou_iti ) {
		PRT("move() Err: コウ！z=%04x\n",z);
		return MOVE_KOU;
	}
	x = z & 0xff;
	y = z >> 8;
	if ( x < 1 || x > ban_size || y < 1 || y > ban_size || ban[z] != 0 ) {
		PRT("move() Err: 空点ではない！z=%04x\n",z);
		return MOVE_EXIST;
	}

	ban[z] = col;	// とりあえず置いてみる
		
	for (i=0;i<4;i++) {
		z1 = z + dir4[i];
		if ( ban[z1] != un_col ) continue;
		// 敵の石が取れるか？
		count_dame(z1);
		if ( dame == 0 ) {
			hama[col-1] += ishi;
			all_ishi += ishi;
			del_z = z1;	// 取られた石の座標。コウの判定で使う。
			del_stone(z1,un_col);
		}
	}
	// 自殺手を判定
	count_dame(z);
	if ( dame == 0 ) {
		PRT("move() Err: 自殺手! z=%04x\n",z);
		ban[z] = 0;
		return MOVE_SUICIDE;
	}

	// 次にコウになる位置を判定。石を1つだけ取った場合。
	kou_iti = 0;	// コウではない
	if ( all_ishi == 1 ) {
		// 取られた石の4方向に自分のダメ1の連が1つだけある場合、その位置はコウ。
		kou_iti = del_z;	// 取り合えず取られた石の場所をコウの位置とする
		sum = 0;
		for (i=0;i<4;i++) {
			z1 = del_z + dir4[i];
			if ( ban[z1] != col ) continue;
			count_dame(z1);
			if ( dame == 1 && ishi == 1 ) sum++;
		}
		if ( sum >= 2 ) {
			PRT("１つ取られて、コウの位置へ打つと、１つの石を2つ以上取れる？z=%04x\n",z);
			return MOVE_FATAL;
		}
		if ( sum == 0 ) kou_iti = 0;	// コウにはならない。
	}
	return MOVE_SUCCESS;
}
