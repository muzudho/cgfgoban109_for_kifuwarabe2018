// cgf_pipe.cpp
// GnuGo��GTP�őΐ킷�邽��
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


// GTP�̃G���W�� gnugo.exe �����݂��Ă邩
int IsGtpEngineExist(void)
{
	char str[MAX_PATH];
	
	strcpy(str,sGtpPath[0]);
	char *p = strstr(str," ");	// "Program Files"�Ŏ��s����
	if ( p != NULL ) *p = 0;

	struct _finddata_t c_file;
	long hFile = _findfirst( str, &c_file );
	if ( hFile == -1L ) return 0;
	return 1;
}


SECURITY_ATTRIBUTES sa;
HANDLE hGnuOutRead, hGnuOutWrite;	// GnuGo����̏o�͂�ǂݎ�邽�߂̃p�C�v
HANDLE hGnuInRead,  hGnuInWrite;	// Gnu�֓��͂𑗂邽�߂̃p�C�v
HANDLE hGnuErrRead, hGnuErrWrite;	// Gnu�̃G���[��ǂݎ�邽��
HANDLE hGnuOutReadDup,hGnuInWriteDup,hGnuErrReadDup;	// �e�ŃR�s�[���Ďg�����߁B

STARTUPINFO si;
PROCESS_INFORMATION pi;

// �p�C�v���쐬
int open_gnugo_pipe()
{
	fGnugoPipe = 0;
//	if ( IsGtpEngineExist() == 0 ) return 0;

	// �����p�C�v���쐬
//	SECURITY_ATTRIBUTES sa;
	sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle       = TRUE;
	sa.lpSecurityDescriptor = NULL;
//	HANDLE hGnuOutRead, hGnuOutWrite;	// GnuGo����̏o�͂�ǂݎ�邽�߂̃p�C�v
//	HANDLE hGnuInRead,  hGnuInWrite;	// Gnu�֓��͂𑗂邽�߂̃p�C�v
//	HANDLE hGnuErrRead, hGnuErrWrite;	// Gnu�̃G���[��ǂݎ�邽��
//	HANDLE hGnuOutReadDup,hGnuInWriteDup;	// �e�ŃR�s�[���Ďg�����߁B

	// GnuGo�̓���(STDIN)�p�̃p�C�v�����B
	CreatePipe(&hGnuInRead, &hGnuInWrite, &sa, 0);
	DuplicateHandle(GetCurrentProcess(), hGnuInWrite, GetCurrentProcess(), &hGnuInWriteDup, 0,
			     FALSE,	/* �p�����Ȃ� */ DUPLICATE_SAME_ACCESS);
	CloseHandle(hGnuInWrite);	// �ȍ~�̓R�s�[���g���̂ŕ���B

	// GnuGo�̏o��(STDOUT)�p�̃p�C�v�����B
	CreatePipe(&hGnuOutRead, &hGnuOutWrite, &sa, 0);
	// �n���h���̃R�s�[������āA�e�ł͂�����𗘗p����B�q�Ɍp������Ȃ��悤�ɁB
	DuplicateHandle(GetCurrentProcess(), hGnuOutRead, GetCurrentProcess(), &hGnuOutReadDup, 0,
			     FALSE,	/* �p�����Ȃ� */ DUPLICATE_SAME_ACCESS);
	CloseHandle(hGnuOutRead);	// �ȍ~�̓R�s�[���g���̂ŕ���B
#if 0	// �R�����g�A�E�g�����MoGo�������悤�ɂȂ�B��ʂ�stderr��f���o���Ă邩��H
	// GnuGo�̃G���[(STDERR)�p�̃p�C�v�����B
	CreatePipe(&hGnuErrRead, &hGnuErrWrite, &sa, 0);
	DuplicateHandle(GetCurrentProcess(), hGnuErrRead, GetCurrentProcess(), &hGnuErrReadDup, 0,
			     FALSE,	/* �p�����Ȃ� */ DUPLICATE_SAME_ACCESS);
	CloseHandle(hGnuErrRead);	// �ȍ~�̓R�s�[���g���̂ŕ���B
#endif

	// �v���Z�X�ɓn�����̏���
//	STARTUPINFO si;
//	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb          = sizeof(STARTUPINFO);
	si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
//	si.wShowWindow = SW_SHOWDEFAULT;
	si.wShowWindow = SW_HIDE;	// �R�}���h�v�����v�g�̉�ʂ��B���ꍇ�B
	si.hStdInput   = hGnuInRead;	// GnuGo�ւ̓��͂͂��̃n���h���ɑ�����B�p�C�v�Ōq�����Ă�̂�hGnuInWriteDup���疽�߂𑗂�B
	si.hStdOutput  = hGnuOutWrite;	// GnuGo��  �o�͂͂��̃n���h���ɑ�����B�p�C�v�Ōq�����Ă�̂�hGnuOutReadDup�œǂݎ���B
	si.hStdError   = hGnuErrWrite;	// GnuGo�̃G���[�o�͂�����ꍇ�B    �@    �p�C�v�Ōq�����Ă�̂�hGnuErrReadDup�œǂݎ���B

	// GnuGo���N��
//	CreateProcess(NULL, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --clock 2400 --level 12 --autolevel", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
//	CreateProcess(NULL, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --clock 300 --level 12 --autolevel", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
//	CreateProcess(NULL, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --level 12", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
//	CreateProcess(NULL, "c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	PRT("%s\n",sGtpPath[0]);
	int fRet = CreateProcess(NULL, sGtpPath[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	if ( fRet == 0 ) { PRT("Fail CreateProcess\n"); return 0; }
//	PRT("c:\\go\\gnugo\\gnugo.exe --mode gtp --never-resign --level 12");

	// �����g��Ȃ��n���h�������B
	CloseHandle(pi.hThread);
	CloseHandle(hGnuInRead);
	CloseHandle(hGnuOutWrite);
	CloseHandle(hGnuErrWrite);

	Sleep(500);	// ���S�̂��߂ɑ҂B
//	WaitForInputIdle(pi.hProcess, INFINITE);	// GnuGo�����͂��󂯎���悤�ɂȂ�܂ő҂� ---> CgfGoban��CgfGoban����ĂԂƌł܂�B

	PRT("open gnugo pipe...\n");
	fGnugoPipe = 1;
	return 1;
//	test_gnugo_loop();
}

void close_gnugo_pipe()
{
    // �q�v���Z�X�̃n���h�������B
	CloseHandle(pi.hProcess);
	// GnuGo�Ƃ̒ʐM�Ŏg���Ă����n���h�������B
	CloseHandle(hGnuInWriteDup);
	CloseHandle(hGnuOutReadDup);
	CloseHandle(hGnuErrReadDup);
	fGnugoPipe = 0;
}

// �p�C�v����GnuGo�̏o�͂�ǂݏo��
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
		if ( c != '\n' && c < 32 ) continue;	// '\r' �Ƃ��͖���
		if ( c == 9 ) c = 32;	// TAB��SPACE�ɕϊ�
		buf[n]=c;
		n++;
		if ( n >= BUF_GTP_SIZE ) { PRT("Over!\n"); debug(); }
//		PRT("%c(%x)",c,c);
		if ( c == '\n' ) break;	// �R�}���h���I������
	}

	buf[n] = 0;
	PRT("ReadGTP %s",buf);
	if ( buf[0] == '#') return 1;	// �R�����g�͖���
	if ( buf[0] == '\n' ) return 1;	// ���s�����̍s�͋�؂�B�܂��f�[�^������
	strcpy(BufGetGTP,buf);
	return 2;
}

// �R�}���h����M����B��̎�M�����˂�BError�̏ꍇ��0��Ԃ��B
int ReadCommmand(int *z)
{
	int x=0,y=0,fRet;
	char move[BUF_GTP_SIZE];

	*z = 0;
	
	for (;;) {
		fRet = ReadGTP();	// �p�C�v����GnuGo�̏o�͂�ǂݏo��
		if ( fRet == 0 ) return 0;
		if ( fRet == 1 ) break;	// ����I��
//		if ( BufGetGTP[0] == '\n' ) break;	// ���S�I��
		if ( BufGetGTP[0] == '=' && strlen(BufGetGTP) > 2 ) {
			memset(move,0,sizeof(move));
			sscanf(BufGetGTP+2,"%s",move);	// id �͖���
			PRT("move=%s\n",move);
			if      ( stricmp(move,"pass")   ==0 ) *z =  0;//GTP_MOVE_PASS;		// GnuGo��"PASS"�Ƒ����Ă���B�������ł́H
			else if ( stricmp(move,"resign") ==0 ) *z = -1;//GTP_MOVE_RESIGN;	// �����̏ꍇ(nngs�ɑ����đ��v�H���肪�����邩���jSGMP�͔�Ή��B
			else {			   // "Q4" �Ȃ�
				x = move[0];
				if ( x > 'I' ) x--;	// 'I'�͑��݂��Ȃ��̂�
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

// �p�C�v����GnuGo�̃G���[�o�͂�ǂݏo��
int ReadGnuGoErr() {
	char szOut[BUF_GTP_SIZE];
	DWORD dwRead;

	// ���l�ɃG���[�o�͂̏���
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


void PaintUpdate(void);	// ��ʂ̑S��������

extern int fAutoSave;		// �����̎����Z�[�u�i�ʐM�ΐ펞�j


// �������Z�b�g���Ď��i�߂ĉ�ʂ��X�V�B�G���[�̏ꍇ��0�ȊO��Ԃ��B
int set_kifu_time(int z,int col,int t)
{
	int k;
	PRT("set_kifu_time z=(%2d,%2d)=%04x,col=%d,tesuu=%d,t=%d\n",z&0xff,z>>8,z,col,tesuu,t);
	if ( z < 0 ) return MOVE_RESIGN;

	k = make_move(z,col);
	if ( k != MOVE_SUCCESS ) {
		int x = z & 0xff;
		int y = z >> 8;
		PRT("���肪���[���ᔽ���ȁA�Ǝv������x,y,z=%d,%d,%04x\n",x,y,z);
		return k;
	}

	tesuu++;	all_tesuu = tesuu;
	kifu[tesuu][0] = z;		// �������L��
	kifu[tesuu][1] = col;	// �΂̐F
	kifu[tesuu][2] = t;		// �v�l����

	if ( z==0 ) {
		if ( tesuu > 1 && kifu[tesuu-1][0] == 0  ) {
			PRT("�Q��A���p�X�ł���\n");
			return MOVE_PASS_PASS;
		}
	}
	if ( tesuu > 2000 ) {
		PRT("�萔��2000��𒴂��܂���\n");
		return MOVE_KIFU_OVER;
	}

	PaintUpdate();
	return MOVE_SUCCESS;
}

// �o�ߎ��Ԃ�b�ŕԂ�(double)
double GetSpendTime(clock_t start_time)
{
	return (double)(clock() - start_time) / CLOCKS_PER_SEC;
}

// �݌v�̎v�l���Ԃ��v�Z���Ȃ��������B���݂̎�Ԃ̗݌v�v�l���Ԃ�Ԃ�
int count_total_time(void)
{
	int i,n;
	
	total_time[0] = 0;	// ���̍��v�̎v�l���ԁB
	total_time[1] = 0;	// ���
	for (i=1; i<=tesuu; i++) {
		if ( (i & 1) == 1 ) n = 0;	// ���
		else                n = 1;
		total_time[n] += kifu[i][2];
	}
	return total_time[tesuu & 1];		// �v�l���ԁB���[0]�A���[1]�̗݌v�v�l���ԁB
}


// ���W��'K15'�̂悤�ȕ�����ɕϊ�
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

// GnuGo�̏�����
int init_gnugo_gtp(char *sName)
{
	const int NAME_STR_MAX = 64;
	int z;
	char str[256];
	sprintf(str,"boardsize %d\n",ban_size);	// 19�H�Ղ�
	SendGTP(str);
	if ( ReadCommmand(&z) == 0 ) return 0;	// �p�C�v���猋�ʂ�ǂݏo��
	SendGTP("clear_board\n");				// �Ֆʂ�������
	if ( ReadCommmand(&z) == 0 ) return 0;
	sprintf(str,"komi %.1f\n",komi);		// �R�~��ݒ肷��
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
	if ( strlen(BufGetGTP+2) > NAME_STR_MAX ) *(BufGetGTP+2 + NAME_STR_MAX) = 0;	// MoGo�̃o�[�W��������������
	strcat(sName," ");
	strcat(sName,BufGetGTP+2);
	n = strlen(sName);
	if ( n > 0 && sName[n-1] < ' ' ) sName[n-1] = 0;
	return 1;
}

// Gnugo�ɑł�����
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

// Gnugo�̔Ֆʂɐ΂�u��
void PutStoneGnuGo(int z, int black_turn)
{
	int x;
	char str[BUF_GTP_SIZE],str2[BUF_GTP_SIZE];

	if ( z <  0 ) return;
	change_z_str(str, z);
	if ( black_turn ) sprintf(str2,"play black %s\n",str);
	else              sprintf(str2,"play white %s\n",str);
	SendGTP(str2);
	ReadCommmand(&x);	// OK��'='����M
}

// Gnugo���̔Ֆʂ�������
void UpdateGnugoBoard(int z)
{
	int black_turn = (GetTurnCol() == 1);

	PutStoneGnuGo(z, black_turn);
}

static const char *status_names[6] =
 {"alive", "dead", "seki", "white_territory", "black_territory", "dame"};

// Gnugo�̎�������Ŏ����\�����s��
void FinalStatusGTP(int *p_endgame)
{
	char str[BUF_GTP_SIZE],str2[BUF_GTP_SIZE];
	int x,y,z,i,k;

	if ( fGnugoPipe==0 ) {
		if ( open_gnugo_pipe()==0 ) return;
		//Gnugo���̔Ֆʂ����������Đ΂�u��
		init_gnugo_gtp(str);
		for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
			z = ((y+1)<<8) + x+1;
			k = ban[z];
			if ( k==0 ) continue;
			PutStoneGnuGo(z, (k==1));
		}
	}

#if 1	// MoGo�ł͔�Ή��Ȃ̂�
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + x+1 ;
		change_z_str(str, z);
		sprintf(str2,"final_status %s\n",str);
		SendGTP(str2);
		k = 0;
		if ( ReadCommmand(&k) == 0 ) break;	// �p�C�v���猋�ʂ�ǂݏo��
		for (i=0;i<6;i++) {
			if ( strstr(BufGetGTP+2,status_names[i]) != NULL ) break;
		}
		*(p_endgame+z) = i;
	}
#else

	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + x+1;
		k = ban[z];
		int status = 0;	// ������Ԃ͑S����
		if ( k==0 ) status = 5;	// dame
		*(p_endgame+z) = status;
	}
	SendGTP("final_status_list dead\n");	// ���΂̂݁Agogui��score�����̎d�l�Ȃ̂�

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
		if ( c != '\n' && c < 32 ) continue;	// '\r' �Ƃ��͖���
		if ( c == 9 ) c = 32;	// TAB��SPACE�ɕϊ�
		buf[n]=c;
		n++;
		if ( n >= BUF_GTP_SIZE ) { PRT("Over!\n"); debug(); }
//		PRT("%c(%x)",c,c);
//		if ( c == '\n' ) break;	// �R�}���h���I������
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
//		if ( (c == '\n' && n==1) || (c=='\n' && n>1 && buf[n-2]=='\n') ) break;	// �R�}���h���I������
//	PRT("%s",buf);
//	for (;;) {
//		int fRet = ReadGTP();	// �p�C�v����GnuGo�̏o�͂�ǂݏo��
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

	fAutoSave = 1;		// �����̎����Z�[�u�i�ʐM�ΐ펞�j

	for (;;) {

		// GnuGo�̏�����
		if ( init_gnugo_gtp() == 0 ) return;

//		PRT("\\r=%d,\\n=%d\n",'\r','\n');
		i = 0;
		if ( dll_col == 1 ) goto cgf_first;

		for (i=0;i<1000;i++) {
			char str[BUF_GTP_SIZE],str2[BUF_GTP_SIZE];

			// �p�C�v����GnuGo�̃G���[�o�͂�ǂݏo��
//			if ( ReadGnuGoErr() == FALSE ) break;	// ���܂��ǂݎ��Ȃ��H

			ct1 = clock();	// �T���J�n���ԁB1000���̂P�b�P�ʂő���B

			if ( tesuu&1 ) SendGTP("genmove white\n");
			else           SendGTP("genmove black\n");
			if ( ReadCommmand(&z) == 0 ) break;
		
			fRet = set_kifu_time(z,GetTurnCol(),(int)GetSpendTime(ct1));
			if ( fRet ) break;
cgf_first:

			ct1 = clock();	// �T���J�n���ԁB1000���̂P�b�P�ʂő���B
#if 0
for (;;) {
	for (i=0;i<300;i++) {
		x = (rand() % 17) + 2;	// 2-18�B�Ղ̒[�ɂ͑ł��Ȃ��B
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
			z = ThinkingDLL(0);	// �c�莞�Ԕ���t���̎v�l���[�`���Ăяo��
#endif

			if ( thinking_utikiri_flag ) break;

			change_z_str(str, z);
			if ( tesuu & 1 ) sprintf(str2,"play white %s\n",str);
			else             sprintf(str2,"play black %s\n",str);

			SendGTP(str2);
			if ( ReadCommmand(&x) == 0 ) break;	// OK��'='����M

			fRet = set_kifu_time(z,GetTurnCol(),(int)GetSpendTime(ct1));
			if ( fRet ) break;
		}

		if ( thinking_utikiri_flag ) break;
		if ( fRet != 2 ) break;
		break;
	}
}
#endif
