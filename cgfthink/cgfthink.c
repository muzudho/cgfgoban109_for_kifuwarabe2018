// CgfGoban.exe�p�̎v�l���[�`���̃T���v��
// 2005/06/04 - 2005/07/15 �R�� �G
// �����Ŏ��Ԃ������ł��B
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <windows.h>

#include "cgfthink.h"


// �v�l���f�t���O�B0�ŏ���������Ă��܂��B
// GUI�́u�v�l���f�{�^���v�������ꂽ�ꍇ��1�ɂȂ�܂��B
int *pThinkStop = NULL;	


#define BOARD_MAX ((19+2)*256)			// 19�H�Ղ��ő�T�C�Y�Ƃ���

int board[BOARD_MAX];
int check_board[BOARD_MAX];		// ���ɂ��̐΂����������ꍇ��1

int board_size;	// �Ֆʂ̃T�C�Y�B19�H�Ղł�19�A9�H�Ղł�9

// ���E�A�㉺�Ɉړ�����ꍇ�̓�����
int dir4[4] = { +0x001,-0x001,+0x100,-0x100 };

int ishi = 0;	// ������΂̐�(�ċA�֐��Ŏg��)
int dame = 0;	// �A�̃_���̐�(�ċA�֐��Ŏg��)
int kou_z = 0;	// ���ɃR�E�ɂȂ�ʒu
int hama[2];	// [0]... ����������΂̐�, [1]...����������΂̐�
int sg_time[2];	// �݌v�v�l����

#define UNCOL(x) (3-(x))	// �΂̐F�𔽓]������

// move()�֐��Ŏ��i�߂����̌���
enum MoveResult {
	MOVE_SUCCESS,	// ����
	MOVE_SUICIDE,	// ���E��
	MOVE_KOU,		// �R�E
	MOVE_EXIST,		// ���ɐ΂�����
	MOVE_FATAL		// ����ȊO
};



// �֐��̃v���g�^�C�v�錾
void count_dame(int tz);				// �_���Ɛ΂̐��𒲂ׂ�
void count_dame_sub(int tz, int col);	// �_���Ɛ΂̐��𒲂ׂ�ċA�֐�
int move_one(int z, int col);			// 1��i�߂�Bz ... ���W�Acol ... �΂̐F
void print_board(void);					// ���݂̔Ֆʂ�\��
int get_z(int x,int y);					// (x,y)��1�̍��W�ɕϊ�
int endgame_status(int endgame_board[]);		// �I�Ǐ���
int endgame_draw_figure(int endgame_board[]);	// �}�`��`��
int endgame_draw_number(int endgame_board[]);	// ���l������(0�͕\������Ȃ�)


// �ꎞ�I��Windows�ɐ����n���܂��B
// �v�l���ɂ��̊֐����ĂԂƎv�l���f�{�^�����L���ɂȂ�܂��B
// ���b30��ȏ�Ă΂��悤�ɂ���ƃX���[�Y�ɒ��f�ł��܂��B
void PassWindowsSystem(void)
{
	MSG msg;

	if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) ) {
		TranslateMessage( &msg );						// keyboard input.
		DispatchMessage( &msg );
	}
}

#define PRT_LEN_MAX 256			// �ő�256�����܂ŏo�͉�
static HANDLE hOutput = INVALID_HANDLE_VALUE;	// �R���\�[���ɏo�͂��邽�߂̃n���h��

// printf()�̑�p�֐��B
void PRT(const char *fmt, ...)
{
	va_list ap;
	int len;
	static char text[PRT_LEN_MAX];
	DWORD nw;

	if ( hOutput == INVALID_HANDLE_VALUE ) return;
	va_start(ap, fmt);
	len = _vsnprintf( text, PRT_LEN_MAX-1, fmt, ap );
	va_end(ap);

	if ( len < 0 || len >= PRT_LEN_MAX ) return;
	WriteConsole( hOutput, text, (DWORD)strlen(text), &nw, NULL );
}


// �΋ǊJ�n���Ɉ�x�����Ă΂�܂��B
DLL_EXPORT void cgfgui_thinking_init(int *ptr_stop_thinking)
{
	// ���f�t���O�ւ̃|�C���^�ϐ��B
	// ���̒l��1�ɂȂ����ꍇ�͎v�l���I�����Ă��������B
	pThinkStop = ptr_stop_thinking;

	// PRT()����\�����邽�߂̃R���\�[�����N������B
	AllocConsole();		// ���̍s���R�����g�A�E�g����΃R���\�[���͕\������܂���B
	SetConsoleTitle("CgfgobanDLL Infomation Window");
	hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	PRT("�f�o�b�O�p�̑��ł��BPRT()�֐��ŏo�͂ł��܂��B\n");

	// ���̉��ɁA�������̊m�ۂȂǕK�v�ȏꍇ�̃R�[�h���L�q���Ă��������B
}

// �΋ǏI�����Ɉ�x�����Ă΂�܂��B
// �������̉���Ȃǂ��K�v�ȏꍇ�ɂ����ɋL�q���Ă��������B
DLL_EXPORT void cgfgui_thinking_close(void)
{
	FreeConsole();
	// ���̉��ɁA�������̉���ȂǕK�v�ȏꍇ�̃R�[�h���L�q���Ă��������B
}

// �v�l���[�`���B����1���Ԃ��B
// �{�̂��珉���ՖʁA�����A�萔�A��ԁA�Ղ̃T�C�Y�A�R�~�A����������ԂŌĂ΂��B
DLL_EXPORT int cgfgui_thinking(
	int dll_init_board[],   // �����Ֆ�
	int dll_kifu[][3],      // ����  [][0]...���W�A[][1]...�΂̐F�A[][2]...����ԁi�b)
	int dll_tesuu,          // �萔
	int dll_black_turn,     // ���(����...1�A����...0)
	int dll_board_size,     // �Ֆʂ̃T�C�Y
	double dll_komi,        // �R�~
	int dll_endgame_type,   // 0...�ʏ�̎v�l�A1...�I�Ǐ����A2...�}�`��\���A3...���l��\���B
	int dll_endgame_board[] // �I�Ǐ����̌��ʂ�������B
)
{
	int z, col, t, i, xxyy, ret_z;

	// ���݋ǖʂ������Ə����Ֆʂ�����
	for (i = 0; i < BOARD_MAX; i++) board[i] = dll_init_board[i]; // �����Ֆʂ��R�s�[
	board_size = dll_board_size;
	hama[0] = hama[1] = 0;
	sg_time[0] = sg_time[1] = 0;    // �݌v�v�l���Ԃ�������
	kou_z = 0;

	for (i = 0; i < dll_tesuu; i++) {
		z = dll_kifu[i][0];   // ���W�Ay*256 + x �̌`�œ����Ă���
		col = dll_kifu[i][1];   // �΂̐F
		t = dll_kifu[i][2];   // �����
		sg_time[i & 1] += t;
		if (move_one(z, col) != MOVE_SUCCESS) break;
	}

#if 0   // ���f����������ꍇ�̃T���v���B0��1�ɂ���΃R���p�C������܂��B
	for (i = 0; i < 300; i++) {               // 300*10ms = 3000ms = 3�b�҂��܂��B
		PassWindowsSystem();            // �ꎞ�I��Windows�ɐ����n���܂��B
		if (*pThinkStop != 0) break;  // ���f�{�^���������ꂽ�ꍇ�B
		Sleep(10);                      // 10ms(0.01�b)��~�B
	}
#endif

	// �I�Ǐ����A�}�`�A���l��\������ꍇ
	if (dll_endgame_type == GAME_END_STATUS) return endgame_status(dll_endgame_board);
	if (dll_endgame_type == GAME_DRAW_FIGURE) return endgame_draw_figure(dll_endgame_board);
	if (dll_endgame_type == GAME_DRAW_NUMBER) return endgame_draw_number(dll_endgame_board);

	// �T���v���̎v�l���[�`�����Ă�
	if (dll_black_turn) col = BLACK;
	else                  col = WHITE;

	// (2017-03-17 Add begin)
	// �t�@�C�������o��
	{
		FILE* fp;
		char* fname = "out.txt";

		//fp = fopen(fname, "w");
		//if (fp == NULL) {
		if (0 != fopen_s(&fp, fname, "w")) {
			printf("%s file can not open.\n", fname);
			return -1;
		}

		// ���ǖʂ��o��
		for (int y = 0; y < 21; y++)
		{
			for (int x = 0; x < 21; x++)
			{
				int i = y * 256 + x;
				char buffer[20];
				itoa(board[i], buffer, 10);
				fprintf(fp, buffer);
				fprintf(fp, ",");
			}
			fprintf(fp, "\n");
		}
		{
			char buffer[20];

			// �A�Q�n�}���o��
			itoa(hama[0], buffer, 10);
			fprintf(fp, buffer);
			fprintf(fp, ",");

			itoa(hama[1], buffer, 10);
			fprintf(fp, buffer);
			fprintf(fp, ",");

			// �݌v�v�l���Ԃ��o��
			itoa(sg_time[0], buffer, 10);
			fprintf(fp, buffer);
			fprintf(fp, ",");

			itoa(sg_time[1], buffer, 10);
			fprintf(fp, buffer);
			fprintf(fp, ",");

			// �R�E���o��
			xxyy = (kou_z % 256) * 100 + (kou_z / 256);
			itoa(xxyy, buffer, 10);
			fprintf(fp, buffer);
			fprintf(fp, ",");

			// ��ԁi�΂̐F�j���o��
			itoa(col, buffer, 10);
			fprintf(fp, buffer);
			fprintf(fp, ",");

			// ����̎w������o��
			xxyy = (z % 256) * 100 + (z / 256);
			itoa(xxyy, buffer, 10);
			fprintf(fp, buffer);
			fprintf(fp, ",");

			fprintf(fp, "\n");
		}

		// �����Ń��b�N�����B
		fclose(fp);
		PRT("out.txt�o�� / ");
	}

	// ����
	ret_z = -1;
	// ��������܂Ń��g���C�B
	while (-1 == ret_z) {
		// �t�@�C���Ǎ�
		FILE* pInFile;
		char* pInFileName = "in.txt";
		char buffer[100];

		if (0 != fopen_s(&pInFile, pInFileName, "r")) {
			// �t�@�C����Ǎ��߂Ȃ�����

			// 0.1 �b�X���[�v
			Sleep(100);
		}
		else
		{
			if (fgets(buffer, 100, pInFile) != NULL) {
				xxyy = atoi(buffer);
				ret_z = (xxyy % 100) * 256 + xxyy / 100;

				fclose(pInFile);

				// �t�@�C������Ă���A�폜�B
				if (remove(pInFileName) == 0) {
					// �t�@�C���̍폜����
					PRT("in.txt���� / ");
				}
				else {
					// �t�@�C���̍폜���s
					PRT("�y���I�z�i���Q���jin.txt �t�@�C���̍폜���s / ");
				}
			}
			else
			{
				fclose(pInFile);
			}
		}
	}
	// (2017-03-17 Add end)

	PRT("%4d��� (%3d�b : %3d�b) %s %4d\n", dll_tesuu, sg_time[0], sg_time[1], dll_black_turn == 0 ? "��" : "��", xxyy);
	//  print_board();
	return ret_z;
}

// ���݂̔Ֆʂ�\��
void print_board(void)
{
	int x,y,z;
	char *str[4] = { "�E","��","��","�{" };

	for (y=0;y<board_size+2;y++) for (x=0;x<board_size+2;x++) {
		z = (y+0)*256 + (x+0);
		PRT("%s",str[board[z]]);
		if ( x == board_size + 1 ) PRT("\n");
	}
}

// �I�Ǐ����i�T���v���ł͊ȒP�Ȕ��f�Ŏ��΂ƒn�̔�������Ă��܂��j
int endgame_status(int endgame_board[])
{
	int x,y,z,sum,i,k;
	int *p;

	for (y=0;y<board_size;y++) for (x=0;x<board_size;x++) {
		z = get_z(x,y);
		p = endgame_board + z;
		if ( board[z] == 0 ) {
			*p = GTP_DAME;
			sum = 0;
			for (i=0;i<4;i++) {
				k = board[z+dir4[i]];
				if ( k == WAKU ) continue;
				sum |= k;
			}
			if ( sum == BLACK ) *p = GTP_BLACK_TERRITORY;
			if ( sum == WHITE ) *p = GTP_WHITE_TERRITORY;
		} else {
			*p = GTP_ALIVE;
			count_dame(z);
//			PRT("(%2d,%2d),ishi=%2d,dame=%2d\n",z&0xff,z>>8,ishi,dame);
			if ( dame <= 1 ) *p = GTP_DEAD;
		}
	}
	return 0;
}

// �}�`��`��
int endgame_draw_figure(int endgame_board[])
{
	int x,y,z;
	int *p;

	for (y=0;y<board_size;y++) for (x=0;x<board_size;x++) {
		z = get_z(x,y);
		p = endgame_board + z;
		if ( (rand() % 2)==0 ) *p = FIGURE_NONE;
		else {
			if ( rand() % 2 ) *p = FIGURE_BLACK;
			else              *p = FIGURE_WHITE;
			*p |= (rand() % 9) + 1;
		}
	}
	return 0;
}

// ���l������(0�͕\������Ȃ�)
int endgame_draw_number(int endgame_board[])
{
	int x,y,z;
	int *p;

	for (y=0;y<board_size;y++) for (x=0;x<board_size;x++) {
		z = get_z(x,y);
		p = endgame_board + z;
		*p = (rand() % 110) - 55;
	}
	return 0;
}


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
	for (i=0;i<BOARD_MAX;i++) check_board[i] = 0;
	count_dame_sub(tz, board[tz]);
}

// �_���Ɛ΂̐�����ċA�֐�
// 4�����𒲂ׂāA�󔒂�������+1�A�����̐΂Ȃ�ċA�ŁB����̐΁A�ǂȂ炻�̂܂܁B
void count_dame_sub(int tz, int col)
{
	int z,i;

	check_board[tz] = 1;			// ���̐΂͌����ς�	
	ishi++;							// �΂̐�
	for (i=0;i<4;i++) {
		z = tz + dir4[i];
		if ( check_board[z] ) continue;
		if ( board[z] == 0 ) {
			check_board[z] = 1;	// ���̋�_�͌����ς�
			dame++;				// �_���̐�
		}
		if ( board[z] == col ) count_dame_sub(z,col);	// ���T���̎����̐�
	}
}

// �΂�����
void del_stone(int tz,int col)
{
	int z,i;
	
	board[tz] = 0;
	for (i=0;i<4;i++) {
		z = tz + dir4[i];
		if ( board[z] == col ) del_stone(z,col);
	}
}

// ���i�߂�Bz ... ���W�Acol ... �΂̐F
int move_one(int z, int col)
{
	int i,z1,sum,del_z = 0;
	int all_ishi = 0;	// ������΂̍��v
	int un_col = UNCOL(col);

	if ( z == 0 ) {	// PASS�̏ꍇ
		kou_z = 0;
		return MOVE_SUCCESS;
	}
	if ( z == kou_z ) {
		PRT("move() Err: �R�E�Iz=%04x\n",z);
		return MOVE_KOU;
	}
	if ( board[z] != 0 ) {
		PRT("move() Err: ��_�ł͂Ȃ��Iz=%04x\n",z);
		return MOVE_EXIST;
	}
	board[z] = col;	// �Ƃ肠�����u���Ă݂�
		
	for (i=0;i<4;i++) {
		z1 = z + dir4[i];
		if ( board[z1] != un_col ) continue;
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
		board[z] = 0;
		return MOVE_SUICIDE;
	}

	// ���ɃR�E�ɂȂ�ʒu�𔻒�B�΂�1����������ꍇ�B
	kou_z = 0;	// �R�E�ł͂Ȃ�
	if ( all_ishi == 1 ) {
		// ���ꂽ�΂�4�����Ɏ����̃_��1�̘A��1��������ꍇ�A���̈ʒu�̓R�E�B
		kou_z = del_z;	// ��荇�������ꂽ�΂̏ꏊ���R�E�̈ʒu�Ƃ���
		sum = 0;
		for (i=0;i<4;i++) {
			z1 = del_z + dir4[i];
			if ( board[z1] != col ) continue;
			count_dame(z1);
			if ( dame == 1 && ishi == 1 ) sum++;
		}
		if ( sum >= 2 ) {
			PRT("�P����āA�R�E�̈ʒu�֑łƁA�P�̐΂�2�ȏ����Hz=%04x\n",z);
			return MOVE_FATAL;
		}
		if ( sum == 0 ) kou_z = 0;	// �R�E�ɂ͂Ȃ�Ȃ��B
	}
	return MOVE_SUCCESS;
}
