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

extern HWND ghWindow;					/* WINDOWS�̃n���h��  */

int ban_size = BAN_19;

int tesuu = 0;			// �萔
int all_tesuu = 0;		// �Ō�܂ł̎萔
int kifu[KIFU_MAX][3];	// ����	[0] �ʒu, [1] �F, [2] ����

int kou_iti;			// ���ɍ��ɂȂ�ʒu
int hama[2];			// ���Ɣ��̗g�l�i���΂̐��j

double komi = 6.5;		// �R�~��6�ڔ��B

 /*** �G���[���� ***/
void debug(void)
{
	PRT("debug!!!\n");
	PRT("�萔=%d\n",tesuu);
	hyouji();

#ifdef CGF_E
	MessageBox( ghWindow, "CgfGoBan's Internal Error!\nIf you push OK, Program will be terminated by force.", "Debug!", MB_OK);
#else
	MessageBox( ghWindow, "�����G���[�Œ�~���܂���", "�f�o�b�O�I", MB_OK);
#endif
	exit(1);
}

void hyouji(void)
{
	int k,x,y,z;
	char *st[5] = { "�{","��","��","��","��" };	// ���܂����̂����B�󓴂���
//	PRT("   �`�a�b�c�d�e�f�g�i�j�k�l�m�n�o�p�q�r�s\n");
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




// �N���b�v�{�[�h�ɔՖʂ��R�s�[
void hyouji_clipboard(void)
{
	int k,x,y,z,zz,n;
	HGLOBAL hMem;
	LPSTR lpStr;
	static char *cXn[20] = {"","�`","�a","�b","�c","�d","�e","�f","�g","�i","�j","�k","�l","�m","�n","�o","�p","�q","�r","�s"};
	static char *cYn[20] = {"","�P","�Q","�R","�S","�T","�U","�V","�W","�X","10","11","12","13","14","15","16","17","18","19"};

static char *cSn[] = {
"��","��","��",
"��","��","��",
"��","��","��",
"��","��","��",
"��","��","��",
"��","��","��",
};

/*
�����O�l�ܘZ������
01������������������
02������������������
03������������������
04������������������
05������������������
06������������������
07������������������
08������������������
09������������������
*/
	char sTmp[256];

	hMem = GlobalAlloc(GHND, 1024);
	lpStr = (LPSTR)GlobalLock(hMem);

	lstrcpy( lpStr, "");
	
	if ( tesuu & 1 ) lstrcat(lpStr,"��");
	else lstrcat(lpStr,"��");
	sprintf( sTmp,"%d��",tesuu);
	lstrcat(lpStr,sTmp);

	zz = kifu[tesuu][0];
	if ( zz != 0 ) {
		x = zz & 0x00ff;
		y = (zz & 0xff00) >> 8;
		sprintf(sTmp," %s-%s ",cXn[x],cYn[y]);
	} else sprintf(sTmp," PASS ");
	lstrcat(lpStr,sTmp);
	sprintf(sTmp,"�܂�  �n�} ��%d�q  ��%d�q\r\n",hama[0],hama[1]);
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
				if ( z == zz ) sprintf(sTmp,"��");
				else sprintf(sTmp,"��");		// ���܂����̂���
			} else if ( k==2 ) {
				if ( z == zz ) sprintf(sTmp,"��");
				else sprintf(sTmp,"��");	// �󓴂���
			} else {
				if ( (x==0 && y==0) || (x==0 && y==ban_size-1) || (x==ban_size-1 && y==ban_size-1) || (x==ban_size-1 && y==0) ) sprintf(sTmp,"�E");
				else if ( x==0 || x==ban_size-1 ) sprintf(sTmp,"�b");
				else if ( y==0 || y==ban_size-1 ) sprintf(sTmp,"�|");
				else sprintf(sTmp,"�{");
			}
#else
			//����������������������
			if ( k==1 ) {
				if ( z == zz ) sprintf(sTmp,"��");
				else sprintf(sTmp,"��");		// ���܂����̂���
			} else if ( k==2 ) {
				if ( z == zz ) sprintf(sTmp,"��");
				else sprintf(sTmp,"��");	// �󓴂���
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
		PRT("�N���b�v�{�[�h�ɔՖ�(�萔=%d)���o�͂��܂����B\n",tesuu);
	}
}


// SGF�̊������N���b�v��
void SGF_clipout(void)
{
	HGLOBAL hMem;
	LPSTR lpStr;

	SaveSGF();	// SGF�������o��

	hMem = GlobalAlloc(GHND, strlen(SgfBuf)+1);
	lpStr = (LPSTR)GlobalLock(hMem);
	strcpy(lpStr,SgfBuf);

	GlobalUnlock(hMem);
	if ( OpenClipboard(ghWindow) ) {
		EmptyClipboard();
		SetClipboardData( CF_TEXT, hMem );
		CloseClipboard();
//		PRT("�N���b�v�{�[�h��SGF���o�͂��܂����B\n");
	}
}


#define SBC_NUM 31	// �N���b�v�{�[�h����ǂݍ��ގ��̕����̎��
static char *sBC[SBC_NUM] = {
"��","��","��",
"��","��","��","��",	// 0x819b, 0x81fc,�R�[�h���Ⴄ�B
"��","��","��",
"��","��","��",
"��","��","��",
"�E","�|","�{",
"�b","�|","�{",
"��","��","��",
"��","��","��",
"��","��","��",
			
};

// �N���b�v�{�[�h����Ֆʂ�ǂݍ���
void clipboard_to_board(void)
{
	int i,j,k,y,z,flag=0,nLen,sgf_start=0;
	char c,*p;
	HGLOBAL hMem,hClipMem;
	LPSTR lpStr,lpClip;

	if ( OpenClipboard(ghWindow) == FALSE ) {
		PRT("�N���b�v�{�[�h�I�[�v���̎��s�ł��B\n");
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


	// 1�����Â��o���Ĕ�r�A�����B
	// ���s������Β�~
	nLen = strlen( lpStr );
	PRT("�N���b�v�{�[�h����ǂݍ��݁B�o�C�g��=%d\n",nLen);

	// SGF�t�@�C�����ǂ����̔��������B
	// ������̒���(;���A�����ďo������΁A�ŏ���(����ǂݍ��݂��J�n�B���s������͖�������B
	for (i=0;i<nLen;i++) {
		c = *(lpStr+i);	// 1�o�C�g�ǂݎ��B
		if ( c=='(' ) {
			sgf_start = i; 
			flag = 1;
			continue;
		}
		if ( c < 0x20 ) continue;	// ����
		if ( c==';' && flag ) { PRT("SGF file!\n"); break; }
		flag = 0;
	}
	if ( i != nLen ) {
		// SGF�p�̃o�b�t�@�ɃR�s�[
		nSgfBufSize = 0;
		for (i=sgf_start;i<nLen;i++) {
			SgfBuf[i-sgf_start] = *(lpStr+i);
			nSgfBufSize++;
			if ( nSgfBufSize >= SGF_BUF_MAX ) break;
		}
		LoadSetSGF(1);	// SGF�����[�h���ĔՖʂ��\������
		return;
	}
	

	// �ՖʁB��͂���
//	x = 0;
	y = 0;
//	flag = 0;	// 0...�x���W���������B1...19�̐΂�ǂݎ�蒆�B2...���s�܂œǂݍ��ݒ��B

	for (i=0;i<nLen-1;i++) {
		c = *(lpStr+i);	// 1�o�C�g�ǂݎ��B
		if ( c=='\n' || c=='\r' ) continue;		// 1�s�I��
//		if ( flag == 0 && x >= 4 ) continue;	// 4�����ȓ��ɂx���W���Ȃ���Ή��s�܂Ŗ�������B
//		if ( flag == 2 ) continue;				// ���s��҂���

		// 19��A�����ĔՖʃe�L�X�g���o������΂�����ՖʂƂ���B
		for (j=0;j<19;j++) {
			p = lpStr + i + j*2;
			if ( i+j*2 >= nLen ) break;
			for (k=0;k<SBC_NUM;k++) if ( strncmp(p,sBC[k],2) == 0 ) break;
			if ( k == SBC_NUM ) break;	// �Ֆʃe�L�X�g�ɕs��v
//			PRT("i=%d,y=%d,j=%d,k=%d\n",i,y,j,k);
		}
		if ( j==19 && y < 19 ) {	// �V�����s�𔭌��I
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

	PostMessage(ghWindow,WM_COMMAND,IDM_SET_BOARD,0L);	// ��ʂ̍č\��

/*
 �O �Ղ̔z���p�ӂ��A���������Ă����B

 �P �擪�̔��p�󔒁A�S�p�󔒂��J�b�g�B

 �Q ���̂Q������z��Ɣ�r���Ăx���W��Ԃ�
    '01'  ' 1'  '�P'  '��' �Ȃ�P�s��
    '02'  ' 2'  '�Q'  '��' �Ȃ�Q�s��
    �E�E�E
    �������A'��' ������i���i�q�̂悤�Ɂj�B

 �R ������Q���������Ă����B�P�X�񂩂܂��͕����񂪂Ȃ��Ȃ�܂ŁB
    '��'  '��' �Ȃ獕��
    '��'  '��' �Ȃ甒��
    �Ղɐ΂��㏑�����Ă����B

 �S�~�͖�������B����Ȃ���Ώ㏑������Ȃ��܂ŁB

 �x���W�̃J�E���^�������Ă����āA���̃J�E���^��菬�����Ƃ��͖�������B
*/

}



/*** �Ֆʂ̐Ώ��݂̂���A�\���̂̍č\�z ***/
void reconstruct_ban(void)
{
	int k,x,y,z;
	
	/*** ���ꂼ��̏����Ֆʏ�Ԃ��Aban_19[]�Ɉ�[�ۑ� ***/
	for (y=0;y<ban_size+2;y++) for (x=0;x<ban_size+2;x++) {
		z = y*256 + x;
		k = ban[z];
		if      ( ban_size == BAN_19 ) ban_19[y][x] = k;
		else if ( ban_size == BAN_13 ) ban_13[y][x] = k;
		else                           ban_9 [y][x] = k;
	}
	init_ban();	// �����ݒ���Ă�
}

// �����Ֆʂ̒l���R�s�[���邾��
int init_ban_sub(int x,int y)
{
	int k;
	
	if      ( ban_size == BAN_19 ) k = ban_19[y][x];
	else if ( ban_size == BAN_13 ) k = ban_13[y][x];
	else if ( ban_size == BAN_9  ) k = ban_9 [y][x];
	else                           k = ban_9 [y][x];
	if ( x==0 || y==0 || x>ban_size || y>ban_size ) k = 3;	// �ՊO�͋����I�ɘg�ɂ���B
	else if ( k == 3 ) k = 0;	// �Փ��Ȃ̂ɘg�̂Ƃ��͋����I�ɋ󔒂ɁB
	return k;
}


/*** �����Ֆʏ�Ԃ���A�̃f�[�^�����B�����ݒ肢�낢�� ***/
void init_ban(void)
{
	int k,x,y,z;

	kou_iti = 0;		// ���ɂȂ�ʒu
	tesuu = 0;
	hama[0] = 0;		// �g�l�i���̎��΁j
	hama[1] = 0;
	total_time[0] = total_time[1] = 0;	// �݌v�̎v�l����

	/*** ���ꂼ��̏����Ֆʏ�Ԃ��Aban[]�ɓǂݍ��� ***/
	// �܂��A�O�g�������B�łȂ��Ɖ��ɂ���΂��ċz�_��ՊO�Ɏ����Ă��܂��B
	for (y=0;y<ban_size+2;y++) for (x=0;x<ban_size+2;x++) {
		z = y*256 + x;
		k = init_ban_sub(x,y);	// �����Ֆʂ̒l���R�s�[���邾��
			
		if ( k == 3 ) ban[z] = k;
		else          ban[z] = 0;

//		if ( ban[z] == 3 ) ban_start[z] = 3;	// �����Ֆʂ͘g����
	}

	for (y=0;y<ban_size+2;y++) for (x=0;x<ban_size+2;x++) {
		z = y*256 + x;
		k = init_ban_sub(x,y);	// �����Ֆʂ̒l���R�s�[���邾��
			
		if ( k == 0 || k == 3 ) ban[z] = k;
		else {
			if ( make_move(z,k) != MOVE_SUCCESS ) {
				PRT("�����ՖʂɃG���[�����I�I�I z = %x\n",z);
//				hyouji();
			}
		}
		ban_start[z] = ban[z];	// �����Ֆʂɂ������ő������
	}
}


/**************************************************/

int ban[BANMAX];		// ban[5376];	// = 21*256
int ban_start[BANMAX];	// �J�n�ǖʂ���ɋL��

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
