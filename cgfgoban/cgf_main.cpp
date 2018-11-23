/*** CFG_MAIN.C ***/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "cgf.h"

int check_ban[BANMAX];		// ���ɂ��̐΂����������ꍇ��1

int ishi = 0;	// ������΂̐�(�ċA�֐��Ŏg��)
int dame = 0;	// �A�̃_���̐�(�ċA�֐��Ŏg��)

int dir4[4] = { +0x001, +0x100, -0x001, -0x100 };


// �֐��̃v���g�^�C�v�錾
void count_dame(int tz);				// �_���Ɛ΂̐��𒲂ׂ�
void count_dame_sub(int tz, int col);	// �_���Ɛ΂̐�����ċA�֐�
int get_z(int x,int y);					// (x,y)��1�̍��W�ɕϊ�

// (x,y)��1�̍��W�ɕϊ�
int get_z(int x,int y)
{
	return (y+1)*256 + (x+1);
}

// �ʒu tz �ɂ�����_���̐��Ɛ΂̐����v�Z�B���ʂ̓O���[�o���ϐ��ɁB
void count_dame(int tz)
{
	int i;

	dame = ishi = 0;
	for (i=0;i<BANMAX;i++) check_ban[i] = 0;
	count_dame_sub(tz, ban[tz]);
}

// �_���Ɛ΂̐�����ċA�֐�
// 4�����𒲂ׂāA�󔒂�������+1�A�����̐΂Ȃ�ċA�ŁB����̐΁A�ǂȂ炻�̂܂܁B
void count_dame_sub(int tz, int col)
{
	int z,i;

	check_ban[tz] = 1;			// ���̐΂͌����ς�	
	ishi++;							// �΂̐�
	for (i=0;i<4;i++) {
		z = tz + dir4[i];
		if ( check_ban[z] ) continue;
		if ( ban[z] == 0 ) {
			check_ban[z] = 1;	// ���̋�_�͌����ς�
			dame++;				// �_���̐�
		}
		if ( ban[z] == col ) count_dame_sub(z,col);	// ���T���̎����̐�
	}
}

// �΂�����
void del_stone(int tz,int col)
{
	int z,i;
	
	ban[tz] = 0;
	for (i=0;i<4;i++) {
		z = tz + dir4[i];
		if ( ban[z] == col ) del_stone(z,col);
	}
}

// ���i�߂�Bz ... ���W�Acol ... �΂̐F
int make_move(int z, int col)
{
	int i,z1,sum,x,y,del_z = 0;
	int all_ishi = 0;	// ������΂̍��v
	int un_col = UN_COL(col);

	if ( z == 0 ) {	// PASS�̏ꍇ
		kou_iti = 0;
		return MOVE_SUCCESS;
	}
	if ( z == kou_iti ) {
		PRT("move() Err: �R�E�Iz=%04x\n",z);
		return MOVE_KOU;
	}
	x = z & 0xff;
	y = z >> 8;
	if ( x < 1 || x > ban_size || y < 1 || y > ban_size || ban[z] != 0 ) {
		PRT("move() Err: ��_�ł͂Ȃ��Iz=%04x\n",z);
		return MOVE_EXIST;
	}

	ban[z] = col;	// �Ƃ肠�����u���Ă݂�
		
	for (i=0;i<4;i++) {
		z1 = z + dir4[i];
		if ( ban[z1] != un_col ) continue;
		// �G�̐΂����邩�H
		count_dame(z1);
		if ( dame == 0 ) {
			hama[col-1] += ishi;
			all_ishi += ishi;
			del_z = z1;	// ���ꂽ�΂̍��W�B�R�E�̔���Ŏg���B
			del_stone(z1,un_col);
		}
	}
	// ���E��𔻒�
	count_dame(z);
	if ( dame == 0 ) {
		PRT("move() Err: ���E��! z=%04x\n",z);
		ban[z] = 0;
		return MOVE_SUICIDE;
	}

	// ���ɃR�E�ɂȂ�ʒu�𔻒�B�΂�1����������ꍇ�B
	kou_iti = 0;	// �R�E�ł͂Ȃ�
	if ( all_ishi == 1 ) {
		// ���ꂽ�΂�4�����Ɏ����̃_��1�̘A��1��������ꍇ�A���̈ʒu�̓R�E�B
		kou_iti = del_z;	// ��荇�������ꂽ�΂̏ꏊ���R�E�̈ʒu�Ƃ���
		sum = 0;
		for (i=0;i<4;i++) {
			z1 = del_z + dir4[i];
			if ( ban[z1] != col ) continue;
			count_dame(z1);
			if ( dame == 1 && ishi == 1 ) sum++;
		}
		if ( sum >= 2 ) {
			PRT("�P����āA�R�E�̈ʒu�֑łƁA�P�̐΂�2�ȏ����Hz=%04x\n",z);
			return MOVE_FATAL;
		}
		if ( sum == 0 ) kou_iti = 0;	// �R�E�ɂ͂Ȃ�Ȃ��B
	}
	return MOVE_SUCCESS;
}
