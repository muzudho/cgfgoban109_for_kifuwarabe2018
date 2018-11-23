/*** cgf_win.cpp --- Windows�Ɉˑ����镔������Ɋ܂� --- ***/
//#pragma warning( disable : 4115 4201 4214 4057 4514 )  // �x�����b�Z�[�W 4201 �𖳌��ɂ���B
#include <Windows.h>
#include <MMSystem.h>	// PlaySound()���g���ꍇ�ɕK�v�BWindows.h �̌�ɑg�ݍ��ށB����� WINMM.LIB ��ǉ������N
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <CommCtrl.h>	// ToolBar ���g�p���邽�߂̒�` comctl32.lib �������N����K�v����
#if (_MSC_VER >= 1100)	// VC++5.0�ȍ~�ŃT�|�[�g
#include <zmouse.h>		// IntelliMouse���T�|�[�g���邽�߂ɖ�����胊���N�i���v�H�j
#endif

#include "cgf.h"
#include "cgf_rc.h"
#include "cgf_wsk.h"
#include "cgf_pipe.h"

//#include "cgfthink.h"

// �����ՖʁA����(�����)�A�萔�A��ԁA�Ֆʂ̃T�C�Y�A�R�~
int (*cgfgui_thinking)(
	int init_board[],	// �����Ֆ�
	int kifu[][3],		// ����  [][0]...���W�A[][1]...�΂̐F�A[][2]...����ԁi�b)
	int tesuu,			// �萔
	int black_turn,		// ���(����...1�A����...0)
	int board_size,		// �Ֆʂ̃T�C�Y
	double komi,		// �R�~
	int endgame_type,	// 0...�ʏ�̎v�l�A1...�I�Ǐ����A2...�}�`��\���A3...���l��\���B
	int endgame_board[]	// �I�Ǐ����̌��ʂ�������B
);

// �΋ǊJ�n���Ɉ�x�����Ă΂�܂��B
void (*cgfgui_thinking_init)(int *ptr_stop_thinking);

// �΋ǏI�����Ɉ�x�����Ă΂�܂��B
void (*cgfgui_thinking_close)(void);

int InitCgfGuiDLL(void);	// DLL������������B
void FreeCgfGuiDLL(void);	// DLL���������B


// ���݋ǖʂŉ������邩�A���w��
enum EndGameType {
	ENDGAME_MOVE,			// �ʏ�̎�
	ENDGAME_STATUS,			// �I�Ǐ���
	ENDGAME_DRAW_FIGURE,	// �}�`��`��
	ENDGAME_DRAW_NUMBER 	// ���l������
};

// �}�`��`�����̎w��B�ՖʁA�΂̏�ɋL����������B
// (�` | �F) �Ŏw�肷��B���Ŏl�p�������ꍇ�� (FIGURE_SQUARE | FIGURE_BLACK)
enum FigureType {
	FIGURE_NONE,			// ���������Ȃ�
	FIGURE_TRIANGLE,		// �O�p�`
	FIGURE_SQUARE,			// �l�p
	FIGURE_CIRCLE,			// �~
	FIGURE_CROSS,			// �~
	FIGURE_QUESTION,		// "�H"�̋L��	
	FIGURE_HORIZON,			// ����
	FIGURE_VERTICAL,		// �c��
	FIGURE_LINE_LEFTUP,		// �΂߁A���ォ��E��
	FIGURE_LINE_RIGHTUP,	// �΂߁A��������E��
	FIGURE_BLACK = 0x1000,	// ���ŏ����i�F�w��)
	FIGURE_WHITE = 0x2000,	// ���ŏ���	(�F�w��j
};

// �������ŃZ�b�g����l
// ���̈ʒu�̐΂��u���v���u���v���B�s���ȏꍇ�́u���v�ŁB
// ���̈ʒu�̓_���u���n�v�u���n�v�u�_���v���B
enum GtpStatusType {
	GTP_ALIVE,				// ��
	GTP_DEAD,				// ��
	GTP_ALIVE_IN_SEKI,		// �Z�L�Ŋ��i���g�p�A�u���v�ő�p���ĉ������j
	GTP_WHITE_TERRITORY,	// ���n
	GTP_BLACK_TERRITORY,	// ���n
	GTP_DAME				// �_��
};

int endgame_board[BANMAX];	// �I�Ǐ����A�}�`�A���l�\���p�̔Ֆ�
int endgame_board_type;		// ���݁A�I�ǁA�}�`�A���l�̂ǂ��\������

/* �v���g�^�C�v�錾 ---------------------- �ȉ�Windows�ŗL�̐錾���܂� */
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK DrawWndProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK PrintProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK AboutDlgProc( HWND, UINT, WPARAM, LPARAM );
int init_comm(HWND hWnd);	// �ʐM�ΐ�̏����������B���s�����ꍇ��0��Ԃ�
void GameStopMessage(HWND hWnd,int ret, int z);	// ���b�Z�[�W��\�����đ΋ǂ��~
void StopPlayingMode(void);				// �΋ǒ��f�����B�ʐM�ΐ풆�Ȃ�|�[�g�Ȃǂ����B
void PaintBackScreen( HWND hWnd );
void GetScreenParam( HWND hWnd,int *dx, int *sx,int *sy, int *db );
int ChangeXY(LONG lParam);
void OpenPrintWindow(HWND);
void KifuSave(void);
int  KifuOpen(void);
void SaveIG1(void);					// ig1�t�@�C���������o��
int count_endgame_score_chinese(void);	// �������[���Ōv�Z�i�Տ�Ɏc���Ă��銈�΂̐��̍��݂̂��Ώہj
void endgame_score_str(char *str, int flag);	// �n�𐔂��Č��ʂ𕶎���ŕԂ�
int IsCommunication(void);	// ���݁A�ʐM�ΐ풆��
void PaintUpdate(void);		// ������������
void UpdateRecentFile(char *sAdd);		// �t�@�C�����X�g���X�V�B��ԍŋߊJ�������̂�擪�ցB
void UpdateRecentFileMenu(HWND hWnd);	// ���j���[��ύX����
void DeleteRecentFile(int n);			// �t�@�C�����X�g����1�����폜�B�t�@�C�����������̂ŁB
BOOL LoadRegCgfInfo(void);		// ���W�X�g���������ǂݍ���
BOOL SaveRegCgfInfo(void);		// ���W�X�g����  ������������
void PRT_to_clipboard(void);	// PRT�̓��e���N���b�v�{�[�h�ɃR�s�[
int IsDlgDoneMsg(MSG msg);		// WinMain()�̃��b�Z�[�W���[�v�ŏ�������Ɏg��
void LoadRegCpuInfo(void);		// ���W�X�g������CPU�̏���ǂݍ���
void SGF_clipout(void);			// SGF�̊������N���b�v��
void MenuColorCheck(HWND hWnd);	// �ՖʐF�̃��j���[�Ƀ`�F�b�N��
void MenuEnablePlayMode(int);	// ���j���[��΋ǒ����ǂ����ŒW�F�\����
void UpdateLifeDethButton(int check_on);	// �����\�����͑S�Ẵ��j���[���֎~����
BOOL LoadMainWindowPlacement(HWND hWnd,int flag);	// ���W�X�g��������i��ʃT�C�Y�ƈʒu�j��ǂݍ���
BOOL SaveMainWindowPlacement(HWND hWnd,int flag);	// ���W�X�g��  �ɏ��i��ʃT�C�Y�ƈʒu�j����������
int IsGnugoBoard(void);	// Gnugo�Ƒ΋ǒ����H
void UpdateStrList(char *sAdd, int *pListNum, const int nListMax, char sList[][TMP_BUF_LEN] );	// ������z��̌�⃊�X�g���X�V�B��ԍŋߑI�񂾂��̂�擪�ցB
int SendUpdateSetKifu(HWND hWnd, int z, int gui_move );		// �����ʐM�ő�������A���̎�ԂɈڂ�����

BOOL CALLBACK CommProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);	// ��M�҂�������_�C�A���O�{�b�N�X�֐�
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
void clipboard_to_board(void);	// �N���b�v�{�[�h����Ֆʂ�ǂݍ���


HWND ghWindow;					/* WINDOWS�̃n���h��  */
HINSTANCE ghInstance;			/* �C���X�^���X       */
HWND hPrint = NULL;				/* ���o�͗p�̃��[�h���X�_�C�A���O */
extern HWND hWndToolBar;		// TOOLBAR�p�̃n���h��
HWND hWndDraw;
HWND hWndMain;
HACCEL hAccel;					// �A�N�Z�����[�^�L�[�̃e�[�v���̃n���h��
HWND hWndToolBar;				// ToolBar

#define PMaxY 400				// ����ʂ̏c�T�C�Y�i�S���j
#define PHeight 42				// ��ʂ̕W������(32���ʏ�) 
#define PRetX 100				// 100�s�ŉ��s
#define PMaxX 256				// 1�s�̍ő啶����(�Ӗ��Ȃ��H�j

char cPrintBuff[PMaxY][PMaxX];	// �o�b�t�@			
static int nHeight;
static int nWidth;
static int PRT_y;
static int PRT_x = 0;

int ID_NowBanColor = ID_BAN_1;	// �f�t�H���g�̔Ղ̐F
int fFileWrite = 0;				// �t�@�C���ɏ������ރt���O
int UseDefaultScreenSize;		// �Œ�̉�ʃT�C�Y���g��
char cDragFile[MAX_PATH];		// �h���b�O���ꂽ�J���ׂ��t�@�C���� ---> ���̓_�u���N���b�N�ŊJ���Ƃ���
char sCgfDir[MAX_PATH];			// cgfgoabn.exe ������f�B���N�g��
int SoundDrop = 0;				// ���艹�Ȃ�
int fInfoWindow = 0;			// ���\������
int fPassWindows;				// Windows�ɏ�����n���Ă�B�\���ȊO��S�ăJ�b�g
int fChineseRule = 0;			// �������[���̏ꍇ
int thinking_utikiri_flag;
int turn_player[2] = { 0,1 };	// �΋ǎ҂̎�ނ�����B��{�� ��:�l��, ��:Computer
char sDllName[] = "CPU";		// �W���̖��O�ł��B�K���ɕҏW���ĉ������B
//char sGnugoDir[MAX_PATH];		// gnugo.exe������t���p�X
int fGnugoLifeDeathView=0;		// gnugo�̔���Ŏ����\��������
int fATView = 0;				// ���W�\����A�`T�ɁB�`�F�X�`���iA-T��I���Ȃ��j
int fAIRyusei = 1;				// AI����nngs
int fShowConsole = 0;

enum TurnPlayerType {
	PLAYER_HUMAN,
	PLAYER_DLL,
	PLAYER_NNGS,
	PLAYER_SGMP,
	PLAYER_GNUGO,
};

/*
2006/03/01 1.02 Visual Studio 2005 Express Edtion�ł��R���p�C���ł���悤�ɁB
2007/01/22      ���O��GTP�G���W��PATH,nngs�T�[�o���𕡐��L������悤�ɁB
2007/09/19 1.03 MoGo�������悤�ɁBstderr�𖳎�����悤�ɁBversion������������ꍇ�͍폜
2008/01/31 1.04 UEC�t��nngs�ɑΉ��BGTP�G���W�������݃`�F�b�N�𖳎��BProgram Files�̂悤�ɋ󔒂��͂��ނƔF���ł��Ȃ������Bpath����傫���B
2009/06/25 1.05 nngs�̐؂ꕉ���̎���(��)��ݒ�\�ɁB
2011/11/16 1.06 Win64�ł̃R���p�C���G���[�������B�x���͂�������o�܂��E�E�E�B
2013/11/14 1.07 �R�~��7.0���w��\�ɁB
2015/03/16 1.08 nngs��human�������𑗂��悤�ɁBAT���W�\��
2018/07/12 1.09 AI������nngs�ɑΉ��B�N������Console���\���\�ɁB
*/
char sVersion[32] = "CgfGoBan 1.09";	// CgfGoban�̃o�[�W����
char sVerDate[32] = "2018/07/15";		// ���t

int dll_col     = 2;	// DLL�̐F�B�ʐM�ΐ펞�ɗ��p�B
int fNowPlaying = 0;	// ���ݑ΋ǒ����ǂ����B

int total_time[2];		// �v�l���ԁB���[0]�A���[1]�̗݌v�v�l���ԁB

int RetZ;				// �ʐM�Ŏ󂯎���
int RetComm = 0;		// �ʐM��Ԃ������B0...Err, 1...��, 2...OK, 3...NewGame, 4...Answer

int nCommNumber = 1;	// COM�|�[�g�̔ԍ��B

int nHandicap = 0;		// �u�΂͂Ȃ�
int fAutoSave = 0;		// �����̎����Z�[�u�i�ʐM�ΐ펞�j
int RealMem_MB;			// ��������
int LatestScore = 0;	// �Ō�Ɍv�Z�����n
int fGtpClient = 0;		// GTP�̘��S�Ƃ��ē����ꍇ

static clock_t gct1;	// ���Ԍv���p

#define RECENT_FILE_MAX 20		// �ŋߊJ�����t�@�C���̍ő吔�B
char sRecentFile[RECENT_FILE_MAX][TMP_BUF_LEN];	// �t�@�C���̐�΃p�X
int nRecentFile = 0;			// �t�@�C���̐�
#define IDM_RECENT_FILE 2000	// ���b�Z�[�W�̐擪
#define RECENT_POS 12			// �ォ��12�Ԗڂɑ}��

char sYourCPU[TMP_BUF_LEN];		// ���̃}�V����CPU��FSB

#define RGB_BOARD_MAX 8	// �ՖʐF�̐�

COLORREF rgbBoard[RGB_BOARD_MAX] = {	// �Ֆʂ̐F
	RGB(231,134, 49),	// ��{(������)
	RGB(255,128,  0),	// �W
	RGB(198,105, 57),	// �Z���ؖڐF
	RGB(192, 64,  9),	// �Z
	RGB(255,188,121),	// �S���i�ʂ̐F�j
	RGB(  0,255,  0),	// ��
	RGB(255,255,  0),	// ��
	RGB(255,255,255),	// ��
};

COLORREF rgbBack  = RGB(  0,  0,  0);	// ���A�w�i�̐F
//COLORREF rgbBack  = RGB(231,255,231);	// �w�i�̐F
//COLORREF rgbBack  = RGB(247,215,181);	// �w�i�̐F
//COLORREF rgbBack  = RGB(  0,128,  0);	// �Z��
//COLORREF rgbBack  = RGB(  5, 75, 49);	// �Z��(�ʂ̔w�i)

COLORREF rgbText  = RGB(255,255,255);	// �ʒu�A���O�̐F

COLORREF rgbBlack = RGB(  0,  0,  0);
COLORREF rgbWhite = RGB(255,255,255);

#define NAME_LIST_MAX 30
char sNameList[NAME_LIST_MAX][TMP_BUF_LEN];	// �΋ǎ҂̖��O�̌��
int nNameList;								// ���O���̐�
//int nNameListSelect[2];					// ���݂̑΋ǎ҂̔ԍ�

#define GTP_PATH_MAX 30
char sGtpPath[GTP_PATH_MAX][TMP_BUF_LEN];	// GTP�G���W���̏ꏊ
int nGtpPath;								// ���O���̐�

#define NNGS_IP_MAX 30
char sNngsIP[NNGS_IP_MAX][TMP_BUF_LEN];	// nngs�̃T�[�o��
int nNngsIP;							// ���̐�

#define GUI_DLL_MOVE  0
#define GUI_USER_PASS 1
#define GUI_GTP_MOVE  2

// SGF �ɂ���f�[�^
char sPlayerName[2][TMP_BUF_LEN];	// �΋ǎ҂̖��O�B[0] ���A[1] ���B
char sDate[TMP_BUF_LEN];			// ���t
char sPlace[TMP_BUF_LEN];			// �ꏊ
char sEvent[TMP_BUF_LEN];			// ��
char sRound[TMP_BUF_LEN];			// ����킩
char sKomi[TMP_BUF_LEN];			// ����
char sTime[TMP_BUF_LEN];			// ����
char sGameName[TMP_BUF_LEN];		// �΋ǖ�
char sResult[TMP_BUF_LEN];			// ����

#define CGF_ICON "CGF1"


#ifdef CGF_E
char szTitleName[] = "CgfGoBan";
char szInfoWinName[] = "Information Window";
#else
char szTitleName[] = "CGF���";
char szInfoWinName[] = "���\��";
#endif

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow )
{
	MSG      msg;
	WNDCLASS wc;
	static char szAppName[] = "CGFGUI";
	static HBRUSH hBrush = NULL;

	ghInstance = hInstance;

#ifdef CGF_E 
	SetThreadLocale(LANG_ENGLISH);	// �������ꏊ���p��ɁB���\�[�X���p��ɕς��B�e�X�g�p�B
#endif

	if ( !hPrevInstance ) {
		wc.lpszClassName = szAppName;
		wc.hInstance     = hInstance;
		wc.lpfnWndProc   = WndProc;
		wc.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wc.hIcon         = LoadIcon( hInstance, CGF_ICON );
		wc.lpszMenuName  = szAppName;
//		hBrush = CreateSolidBrush( RGB( 0,128,0 ) );	// �w�i��΂�(wc.���̃I�u�W�F�N�g�͏I�����Ɏ����I�ɍ폜�����j
//		wc.hbrBackground = hBrush;//	GetStockObject(WHITE_BRUSH);
	    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH); 
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		if (!RegisterClass( &wc )) return FALSE;		/* �N���X�̓o�^ */


	    wc.style         = CS_HREDRAW | CS_VREDRAW;
	    wc.lpfnWndProc   = DrawWndProc;
	    wc.cbClsExtra    = 0;
	    wc.cbWndExtra    = 4;
	    wc.hInstance     = hInstance;
	    wc.hIcon         = NULL;
	    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		hBrush = CreateSolidBrush( rgbBack );	// �w�i�F(wc.���̃I�u�W�F�N�g�͏I�����Ɏ����I�ɍ폜�����j
		wc.hbrBackground = hBrush;//	GetStockObject(WHITE_BRUSH);
//		wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	    wc.lpszMenuName  = NULL;
	    wc.lpszClassName = "DrawWndClass";
	    if (! RegisterClass (&wc)) return FALSE;
	}
	else {
//		MessageBox(NULL,"�Q�N�����悤�Ƃ��Ă��܂�\n�I�������ĉ�����","����",IDOK);
//		return FALSE;
	}

	// ���̍s��CreateWindow�̎��ɒu���Ƃ��߁I�BWM_CREATE�𔭍s���Ă��܂�����I
	if ( lpszCmdLine != NULL ) {
		if ( strstr(lpszCmdLine,"--mode gtp") ) fGtpClient = 1;	// GTP�̘��S�Ƃ��ē���
		else strcpy(cDragFile,lpszCmdLine);	// �_�u���N���b�N���ꂽ�J���ׂ��t�@�C����
	}
	/********** �E�C���h�E�̍쐬 ********/
	hWndMain = CreateWindow(
						szAppName,						/* Class name.   */                                  
//						lpszCmdLine,        			/* �N���� Pram.  */
						szTitleName,					/* Title         */		       					
						WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,	/* Style bits.   */
						  0, 20,						/* X,Y - default.*/
						640,580,						/* CX, CY        */
						NULL,							/* �e�͂Ȃ�      */
						NULL,							/* Class Menu.   */
						hInstance,						/* Creator       */
						NULL							/* Params.       */
					   );

	ghWindow = hWndMain;

	ShowWindow( hWndMain, cmdShow );
	UpdateWindow(hWndMain);

	hAccel = LoadAccelerators(hInstance, "CGF_ACCEL");	// �A�N�Z�����[�^�L�[�̃��[�h

	while( GetMessage( &msg, 0, 0, 0) ) {
		if ( IsDlgDoneMsg(msg) ) {	// �A�N�Z�����[�^�L�[��_�C�A���O�̏���
			TranslateMessage( &msg );					// keyboard input.
			DispatchMessage( &msg );
        }
	}
	return msg.wParam;
}


/************** �E�B���h�E�葱�� ****************/						
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	HMENU hMenu;
	int i,k,z,ret;
	MEMORYSTATUS MemStat;	// ��������������p

	if ( fPassWindows ) for (;;) {	// �v�l���͂��ׂăJ�b�g����B
		if ( msg == WM_CLOSE ) { PRT("\n\n���ݎv�l��...�ʂ̃{�^�����B\n\n"); return 0; }
		if ( msg == WM_COMMAND && (LOWORD(wParam) == IDM_DLL_BREAK || LOWORD(wParam) == IDM_BREAK) ) {
			PRT("\n\n���ݎv�l��...��������~�B\n\n");
			thinking_utikiri_flag = 1;
			return 0;
		}
		if ( msg == WM_USER_ASYNC_SELECT ) break;	// �ʐM���͎󂯎��
		if ( msg == WM_COMMAND && LOWORD(wParam) == IDM_KIFU_SAVE ) break;
		return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}

	switch( msg ) {
		case WM_CREATE:											// �����ݒ�
#ifdef CGF_DEV
			UseDefaultScreenSize = 1;	// �Œ�̉�ʃT�C�Y���g��(���J��OFF)
#endif
            // �`��pWindow�̐���
			hWndDraw = CreateWindowEx(WS_EX_CLIENTEDGE,
				"DrawWndClass", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
				hWnd, NULL, ghInstance, NULL);
			if ( !hWndDraw ) return FALSE;

            CreateTBar(hWnd);				// �c�[���o�[�𐶐�
//		    DragAcceptFiles(hWnd, TRUE);	// �h���b�O���h���b�v�����t�@�C�����󂯕t����B

			init_ban();

			// ���W�X�g��������i��ʃT�C�Y�ƈʒu�j��ǂݍ���
			if ( LoadMainWindowPlacement(hWnd,1) == 0 ) PRT("Registory Error (If first time, it's OK)\n");
			// ���W�X�g���������ǂݍ���
			if ( LoadRegCgfInfo() == 0 ) PRT("Registory Error... something new (If first time, it's OK)\n");
			UpdateRecentFileMenu(hWnd);	// ���j���[��ύX����

			if ( fInfoWindow ) SendMessage(hWnd,WM_COMMAND,IDM_PRINT,0L);		// ���\��Window�̕\��
			else CheckMenuItem( GetMenu(hWnd), IDM_PRINT, MF_UNCHECKED);
			MenuColorCheck(hWnd);	// �ՖʐF�̃��j���[�Ƀ`�F�b�N��

			GlobalMemoryStatus(&MemStat);
			RealMem_MB = MemStat.dwTotalPhys/(1024*1024);	// �ς�ł����������
			LoadRegCpuInfo();	// ���W�X�g������CPU�̏���ǂݍ���
			PRT("����������= %d MB, ",MemStat.dwTotalPhys/(1024*1024));
			PRT("�󕨗�=%dMB, ����=%d%%\n",MemStat.dwAvailPhys/(1024*1024), MemStat.dwMemoryLoad);

			if ( SoundDrop ) { SoundDrop = 0; SendMessage(hWnd,WM_COMMAND,IDM_SOUND,0L); } // ���ʉ����I��
			if ( fATView   ) { fATView   = 0; SendMessage(hWnd,WM_COMMAND,IDM_ATVIEW,0L); }
			if ( fShowConsole ) { fShowConsole = 0; SendMessage(hWnd,WM_COMMAND,IDM_SHOW_CONSOLE,0L); }

			if ( UseDefaultScreenSize == 0 ) {
				SendMessage(hWnd,WM_COMMAND,IDM_BAN_CLEAR,0L);	// ��ʏ���(��A��̃��W�X�g���o�^��ON��!!!�j
			}

			sprintf( sPlayerName[0],"You");
			sprintf( sPlayerName[1],sDllName);

//			PRT("--------->%s\n",GetCommandLine());
			GetCurrentDirectory(MAX_PATH, sCgfDir);	// ���݂̃f�B���N�g�����擾

			SendMessage(hWnd,WM_COMMAND,IDM_BAN_CLEAR,0L);	// ��ʏ���

			SetFocus(hWnd);	// ���hPrint����������Focus���������ɍs���Ă��܂��̂ŁB

			if ( strlen(cDragFile) ) PostMessage(hWnd,WM_COMMAND,IDM_KIFU_OPEN,0L);	// �������J��
//			SendMessage(hWnd,WM_COMMAND,IDM_THINKING,0L); DestroyWindow(hWnd);	// Profile�p�̃e�X�g

			// DLL��ǂݍ���
			ret = InitCgfGuiDLL();
			if ( ret != 0 ) {
				char sTmp[TMP_BUF_LEN];
				if ( ret == 1 ) sprintf(sTmp,"DLL is not found.");
				else            sprintf(sTmp,"Fail GetProcAddress() ... %d",ret);
				MessageBox(NULL,sTmp,"DLL initialize Error",MB_OK); 
				PRT("not able to use DLL.\n");
				return FALSE;
			} else {
				// DLL������������B
				cgfgui_thinking_init(&thinking_utikiri_flag);
				HWND Stealth = FindWindowA("ConsoleWindowClass", NULL);
				if ( Stealth && fShowConsole == 0 ) {
					ShowWindow(Stealth, SW_HIDE);
// 					HWND hWnd = GetConsoleWindow();
				}
			}

			SendMessage(hWnd,WM_COMMAND,IDM_BREAK,0L);	// �΋ǒ��f��Ԃ�
/*
			PRT("fGtpClient = %d\n",fGtpClient);
			if ( fGtpClient ) {
				ShowWindow( hWnd, SW_HIDE );
				if ( hPrint && IsWindow(hPrint)==TRUE ) ShowWindow( hPrint, SW_HIDE );
				{
					extern void test_gtp_client(void);
					test_gtp_client();
				}
				DestroyWindow(hWnd);	// GTP�o�R�̏ꍇ�͂����ɏI������B
//				PRT_to_clipboard();		// PRT�̓��e���N���b�v�{�[�h�ɃR�s�[
			}
*/
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ) {
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
			
				case IDM_TEST:
					gct1 = clock();	// �T���J�n���ԁB1000���̂P�b�P�ʂő���B
//					{ void init_gnugo_pipe(int); init_gnugo_pipe(1); }	// GnuGo��GTP�o�R�ŘA���ΐ�
					PRT("%2.1f �b\n", GetSpendTime(gct1));
					break;

				case IDM_THINKING:			// �v�l���[�`���Ăяo��
					z = ThinkingDLL(ENDGAME_MOVE);	// ����ʒu��Ԃ�
					ret = SendUpdateSetKifu(hWnd, z, GUI_DLL_MOVE);		// �����ʐM�ő�������A���̎�ԂɈڂ�����
					break;
				
				case IDM_PASS:
					if ( fNowPlaying == 0 ) return 0;	 
					ret = SendUpdateSetKifu(hWnd, 0, GUI_USER_PASS);	// �����ʐM�ő�������A���̎�ԂɈڂ�����
					break;

				case IDM_RESIGN:
					if ( fNowPlaying == 0 ) return 0;	 
					ret = SendUpdateSetKifu(hWnd, -1, GUI_USER_PASS);
					break;

				case IDM_GNUGO_PLAY:	// GnuGo(GTP Engine)�Ɏ��ł�����
					z = PlayGnugo(ENDGAME_MOVE);	// ����ʒu��Ԃ�
					ret = SendUpdateSetKifu(hWnd, z, GUI_GTP_MOVE);		// �����ʐM�ő�������A���̎�ԂɈڂ�����
					break;

				case IDM_INIT_COMM:		// �ʐM�ΐ�̏���������
					if ( init_comm(hWnd) ) PostMessage(hWnd,WM_COMMAND,IDM_NEXT_TURN,0L);
					else StopPlayingMode();
					break;

				case IDM_NEXT_TURN:		// ���̎�͂ǂ����ׂ���������
					// COM���m�̑ΐ풆�ɒ�~���邽��(GnuGo���m�ł��ACOM�|GnuGo��)
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
						gct1 = clock();	// �v�l�J�n����
						// �A���p�X�Ȃ�I�Ǐ������Ă�Œ��f
//						if ( kifu[tesuu-1][0]==0 && UseDefaultScreenSize == 0 ) SendMessage(hWnd,WM_COMMAND,IDM_DEADSTONE,0L);	// �I�Ǐ������Ă�
						int next = turn_player[GetTurnCol()-1];
//						if ( next == PLAYER_HUMAN ) ;	// �������Ȃ�
						if ( next == PLAYER_DLL   ) PostMessage(hWnd,WM_COMMAND,IDM_THINKING,0L);
						if ( next == PLAYER_NNGS  ) PostMessage(hWnd,WM_COMMAND,IDM_NNGS_WAIT,0L);
						if ( next == PLAYER_SGMP  ) PostMessage(hWnd,WM_COMMAND,IDM_SGMP_WAIT,0L);
						if ( next == PLAYER_GNUGO ) PostMessage(hWnd,WM_COMMAND,IDM_GNUGO_PLAY,0L);
					}
					break;

				case IDM_NNGS_WAIT:		// �ʐM����̎��҂�
				case IDM_SGMP_WAIT:
				{
					int fBreak = 1;
					for (;;) {
						gct1 = clock();
						if ( fUseNngs ) {
							if ( DialogBoxParam(ghInstance, "NNGS_WAIT", hWnd, (DLGPROC)NngsWaitProc, 1) == FALSE ) {
								PRT("���C�����[�v�ɖ߂�܂�\n");
								break;
							}
							if ( RetZ < 0 ) break;	// nngs�̏ꍇ�� 0...PASS, 1�ȏ�...�ꏊ, �}�C�i�X...�G���[������
							RetComm = 1;
						} else if ( DialogBox(ghInstance, "COMM_DIALOG", hWnd, (DLGPROC)CommProc) == FALSE ) {
							PRT("���C�����[�v�ɖ߂�܂�\n");
							break;
						}
						if ( RetComm == 2 ) continue;	// OK����M���Ĕ��������͂�����x�҂�
						if ( RetComm != 1 ) {	// move �ȊO�͈�x�҂�
							if ( RetComm == 3 ) {
								PRT("NewGame ���܂����Ă���I�ł�OK��Ԃ��Ă����悤\n");
								ReturnOK();	// �����𖞂������̂�OK�𑗂�A���҂�
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
						// ���̎�Ԃ̏����Ɉڂ�
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
							char sTmp[] = "�ǖʂ����������܂����H";
#endif
							fRet = MessageBox(hWnd,sTmp,"Start Game",MB_YESNOCANCEL | MB_DEFBUTTON2);
							if ( fRet == IDCANCEL ) break;
						}
						DialogBoxParam(ghInstance, "START_DIALOG", hWnd, (DLGPROC)StartProc, fRet);
					}
					PaintUpdate();		// �����̖��O���X�V���邽�ߑS��ʏ��������B
					break;

				case IDM_GNUGO_SETTING:
					DialogBox(ghInstance, "GTP_SETTING", hWnd, (DLGPROC)GnuGoSettingProc);
					break;

				case IDM_CLIPBOARD:
					hyouji_clipboard();
					break;

				case IDM_FROM_CLIPBOARD:
					clipboard_to_board();	// �N���b�v�{�[�h����Ֆʂ�ǂݍ���
					break;

				case IDM_PRT_TO_CLIP:
					PRT_to_clipboard();		// PRT�̓��e���N���b�v�{�[�h�ɃR�s�[
					break;

				case IDM_SGF_CLIPOUT:
					SGF_clipout();			// SGF�Ŋ������N���b�v�{�[�h��
					break;

				case IDM_DEADSTONE:			// �c�[���o�[�̃{�^���������ꂽ�ꍇ
				case IDM_VIEW_LIFEDEATH:	// �I�Ǐ����A���ɐ΂�\��
				case IDM_VIEW_FIGURE:		// �}�`��\��
				case IDM_VIEW_NUMBER:		// ���l��\��
					{
						int id = LOWORD(wParam);
						int ids[3] = {IDM_VIEW_LIFEDEATH, IDM_VIEW_FIGURE, IDM_VIEW_NUMBER };  

						hMenu = GetMenu(hWnd);
						for (i=0;i<3;i++) CheckMenuItem(hMenu, ids[i], MF_UNCHECKED);	// ���j���[��S��OFF
						k = 0;
						if ( id==IDM_DEADSTONE ) {
							if ( endgame_board_type==0 ) id = ids[0];
							else k = endgame_board_type;
						}
						if ( id==IDM_VIEW_LIFEDEATH ) k = ENDGAME_STATUS;
						if ( id==IDM_VIEW_FIGURE    ) k = ENDGAME_DRAW_FIGURE;
						if ( id==IDM_VIEW_NUMBER    ) k = ENDGAME_DRAW_NUMBER;
						if ( k == endgame_board_type ) {	// �������̂��ēx�I��
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
					SoundDrop = (SoundDrop == 0);	// ���艹�t���O
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
					MenuColorCheck(hWnd);	// �ՖʐF�̃��j���[�Ƀ`�F�b�N��
					PaintUpdate();
					break;

				case IDM_BREAK:	// �΋ǒ��f
					fNowPlaying = 0;
					MenuEnablePlayMode(0);	// �΋ǒ��͊����ړ����j���[�Ȃǂ��֎~����
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
					move_tesuu(tesuu - 1);		// �C�ӂ̎萔�̋ǖʂɔՖʂ��ړ�������
					PaintUpdate();
					break;

				case IDM_BACK10:
					move_tesuu(tesuu - 10);		// �C�ӂ̎萔�̋ǖʂɔՖʂ��ړ�������
					PaintUpdate();
					break;

				case IDM_BACKFIRST:
					move_tesuu( 0 );			// �C�ӂ̎萔�̋ǖʂɔՖʂ��ړ�������
					PaintUpdate();
					break;

				case IDM_FORTH1:
					move_tesuu(tesuu + 1);		// �C�ӂ̎萔�̋ǖʂɔՖʂ��ړ�������
					PaintUpdate();
					break;

				case IDM_FORTH10:
					move_tesuu(tesuu + 10);		// �C�ӂ̎萔�̋ǖʂɔՖʂ��ړ�������
					PaintUpdate();
					break;

				case IDM_FORTHEND:
					move_tesuu(all_tesuu);		// �C�ӂ̎萔�̋ǖʂɔՖʂ��ړ�������
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

			// RecentFile���I�����ꂽ�ꍇ
			if ( IDM_RECENT_FILE <= wParam && wParam < IDM_RECENT_FILE+RECENT_FILE_MAX ) {
				int n =  wParam - IDM_RECENT_FILE;
				strcpy(cDragFile, sRecentFile[n]);
//				PRT("%s\n",InputDir);
				if ( KifuOpen() == FALSE ) {
					DeleteRecentFile(n);	// �t�@�C�����X�g����폜�B�t�@�C�����������̂ŁB
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

		case WM_USER_ASYNC_SELECT:	// �񓯊��̃\�P�b�g�ʒm(nngs�T�[�o�Ƃ̒ʐM�p)
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
			if ( SaveMainWindowPlacement(hWnd  ,1) == 0 ) PRT("���W�X�g���P�̃G���[�i�������݁j\n");
			SaveRegCgfInfo();	// ���W�X�g���ɓo�^
			FreeCgfGuiDLL();	// DLL���������B
			PostQuitMessage(0);
			break;
		
		default:
			return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}
	return 0L;
}

// �ʐM�ΐ�̏����������B���s�����ꍇ��0��Ԃ�
int init_comm(HWND hWnd)
{
	if ( fUseNngs ) {
		// �ŏ���WinSock�����������ăT�[�o�ɐڑ��܂ŁB
		if ( DialogBoxParam(ghInstance, "NNGS_WAIT", hWnd, (DLGPROC)NngsWaitProc, 0) == FALSE ) {
			PRT("nngs�ڑ����s\n");
			return 0;
		}
		PRT("Login�܂Ő���I��\n");
		return 1;
	}

	if ( OpenConnection() == FALSE ) {
		PRT("�ʐM�|�[�g�I�[�v���̎��s�I\n");
		return 0;
	}
	PRT("���������Z�[�u=%d\n",fAutoSave);
	if ( turn_player[1]==PLAYER_SGMP ) {	// �����炪���i���j�̎��� NEWGAME �𑗂�
		if ( SendNewGame() == FALSE ) {	// NewGame �𑗂�
			PRT("���C�����[�v�ɖ߂�܂�\n");
			CloseConnection();
			return 0;
		}
		// ���őł�
		return 1;
	} else {		// DLL�����Ԃ̎�
		if ( DialogBox(ghInstance, "COMM_DIALOG", hWnd, (DLGPROC)CommProc) == FALSE ) {
			PRT("���C�����[�v�ɖ߂�܂�\n");
			CloseConnection();
			return 0;
		}
		if ( RetComm != 3 ) {
			PRT("NewGame �����Ȃ��ˁE�E�ERetComm=%d\n",RetComm);
			CloseConnection();
			return 0;
		}
//		PRT("NewGame �����������I\n");

		int k = SendQuery(11);	// �΂̐F������
		if ( k == 1 ) {
			PRT("���Ȃ̂ɔ��Ȃ́H\n");
			CloseConnection();
			return 0;
		}
		k = SendQuery(8);	// �݂��悩�ǂ���������
		if ( k >= 2 ) {
			PRT("�u����͂��܂���� k=%d �����Ǒ��s\n",k);
//			CloseConnection();
//			return 0;
		}
		ReturnOK();	// �����𖞂������̂�OK�𑗂�A�w�����҂�
		return 1;
	}
//	return 0;
}

// ���݁A�ʐM�ΐ풆��
int IsCommunication(void)
{
	int i,n;

	for (i=0;i<2;i++) {
		n = turn_player[i];
		if ( n == PLAYER_NNGS || n == PLAYER_SGMP ) return 1;
	}
	return 0;
}

// �΋ǒ��f�����B�ʐM�ΐ풆�Ȃ�|�[�g�Ȃǂ����B
void StopPlayingMode(void)
{
	if ( IsCommunication() ) CloseConnection();
	SendMessage(ghWindow,WM_COMMAND, IDM_BREAK, 0L);
	PaintUpdate();
}

// �����ʐM�ő�������A���̎�ԂɈڂ�����
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
		if ( SoundDrop ) sndPlaySound("stone.wav", SND_ASYNC | SND_NOSTOP);	// MMSYSTEM.H ���C���N���[�h����K�v����B�܂�WinMM.lib�������N
	}

	// ���̎�Ԃ̏����Ɉڂ�
	PostMessage(hWnd,WM_COMMAND,IDM_NEXT_TURN,0L);
	return 0;
}

/*
// �u�΂�2�q�`9�q�܂ł͉��̂悤�ɁB

�{�{��	// 2�q  
�{�{�{
���{�{

�{�{��	// 3�q
�{�{�{
���{��

���{��	// 5�q
�{���{
���{��

���{��	// 6�q
���{��
���{��

���{��	// 7�q
������
���{��

������	// 8�q
���{��
������

�{�{�{�{��	// 10�q
�{�������{
�{�������{
�{�������{
�{�{�{�{�{

�{�{�{�{��	// 12�q
�{�������{
�{�������{
�{�������{
���{�{�{��
*/
// �u�΂�ݒ�
void SetHandicapStones(void)
{
	int i,k;
	
	SendMessage(ghWindow,WM_COMMAND,IDM_BAN_CLEAR,0L);	// �Ֆʏ�����
	k = nHandicap;
	if ( ban_size == BAN_19 ) {
		ban[0x0410] = 1;		// �Ίp���ɂ͕K���u��
		ban[0x1004] = 1;	
		if ( k >= 4 ) {
			ban[0x0404] = 1;	// 4�q�ȏ�͐��͑S��
			ban[0x1010] = 1;	
		}
		if ( k >= 6 ) {
			ban[0x0a04] = 1;	// 6�q�ȏ�͍��E�����̐�
			ban[0x0a10] = 1;	
		}
		if ( k >= 8 ) {
			ban[0x040a] = 1;	// 8�q�ȏ�͏㉺�����̐�
			ban[0x100a] = 1;
		}
		if ( k == 3 ) ban[0x1010] = 1;
		if ( k == 5 || k == 7 || k >= 9 ) ban[0x0a0a] = 1;
	} else if ( ban_size == 13 ) {
		ban[0x040a] = 1;		// �Ίp���ɂ͕K���u��
		ban[0x0a04] = 1;	
		if ( k >= 4 ) {
			ban[0x0404] = 1;	// 4�q�ȏ�͐��͑S��
			ban[0x0a0a] = 1;	
		}
		if ( k >= 6 ) {
			ban[0x0704] = 1;	// 6�q�ȏ�͍��E�����̐�
			ban[0x070a] = 1;	
		}
		if ( k >= 8 ) {
			ban[0x0407] = 1;	// 8�q�ȏ�͏㉺�����̐�
			ban[0x0a07] = 1;
		}
		if ( k == 3 ) ban[0x0a0a] = 1;
		if ( k == 5 || k == 7 || k >= 9 ) ban[0x0707] = 1;
	} else if ( ban_size == BAN_9 ) {
		ban[0x0307] = 1;		// �Ίp���ɂ͕K���u��
		ban[0x0703] = 1;	
		if ( k >= 4 ) {
			ban[0x0303] = 1;	// 4�q�ȏ�͐��͑S��
			ban[0x0707] = 1;	
		}
		if ( k >= 6 ) {
			ban[0x0503] = 1;	// 6�q�ȏ�͍��E�����̐�
			ban[0x0507] = 1;	
		}
		if ( k >= 8 ) {
			ban[0x0305] = 1;	// 8�q�ȏ�͏㉺�����̐�
			ban[0x0705] = 1;
		}
		if ( k == 3 ) ban[0x0707] = 1;
		if ( k == 5 || k == 7 || k >= 9 ) ban[0x0505] = 1;
	}

	for (i=0;i<BANMAX;i++) {
		ban_start[i] = ban[i];	// �J�n�Ֆʂɂ����
	}
	SendMessage(ghWindow,WM_COMMAND,IDM_SET_BOARD,0L);	// �Ֆʏ�����
}


/* ���\���_�C�A���O */
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
		hBrush = CreateSolidBrush( RGB( 0,0,128 ) );	// �w�i��΂�(wc.���̃I�u�W�F�N�g�͏I�����Ɏ����I�ɍ폜�����j
		wc.hbrBackground = hBrush;//	GetStockObject(WHITE_BRUSH);
//		wc.hbrBackground = GetStockObject(WHITE_BRUSH);
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		if ( !RegisterClass(&wc) ) MessageBox(ghWindow,"���\��Winodw�̓o�^�Ɏ��s","�G���[",MB_OK);
	}
	/********** �E�C���h�E�̍쐬 ********/
//	GetWindowRect( hWnd, &rc );	// ���݂̉�ʂ̐�Έʒu���擾�B
//	PRT("%d,%d,%d,%d\n",rc.left, rc.top,rc.right, rc.bottom);

	hPrint = CreateWindow(
				"PrintClass",						/* Class name.   */                                  
				szInfoWinName,						/* Title.        */
				WS_VSCROLL | WS_OVERLAPPEDWINDOW,	/* Style bits.   */
//				rc.right, rc.top,					/* X,Y  �\�����W */
				640, 20,							/* X,Y  �\�����W */
//				600, 50,							/* X,Y  �\�����W */
//				700,560,							/* CX, CY �傫�� */
				700,PHeight*17,						/* CX, CY �傫�� */
				hWnd,								/* �e�͂Ȃ�      */
				NULL,								/* Class Menu.   */
				ghInstance,							/* Creator       */
				NULL								/* Params.       */
			   );

	LoadMainWindowPlacement(hPrint,2);	// ���W�X�g����������擾
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
			PRT_y = PMaxY - PHeight;		// ��{��25�s�\��
			SetScrollRange( hWnd, SB_VERT, 0, PRT_y, FALSE);	
			SetScrollPos( hWnd, SB_VERT, PRT_y, FALSE );
			hDC = GetDC(hWnd);

//			hFont = GetStockObject(OEM_FIXED_FONT);
//			hFont = GetStockObject(SYSTEM_FONT);
//			hFont = GetStockObject(ANSI_FIXED_FONT);

			hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);	// Win98�ŕ�����������Ȃ��悤��
			hOldFont = (HFONT)SelectObject(hDC,hFont);

			GetTextMetrics(hDC, &tm);
			nHeight = tm.tmHeight;
			nHeight -= 2;	// �킴��2�h�b�g�c�����炵�Ă���B���ʂ������� ���FIXED_FONT�Ŏg�p
			nWidth  = tm.tmAveCharWidth;
			SetTextColor( hDC, RGB(255,255,255) );	// �����𔒂�
			SetBkColor( hDC, RGB(0,0,128) );		// �w�i��Z����
			SetBkMode( hDC, TRANSPARENT);			// �w�i�������Ȃ��i�����̏㉺��1�h�b�g�������̂�h���j
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
				case SB_THUMBPOSITION:	// �T���ړ�
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

#if (_MSC_VER >= 1100)		// VC++5.0�ȍ~�ŃT�|�[�g
		case WM_MOUSEWHEEL:	// ---> ����͖������ <zmouse.h> �Œ�`���Ă���B
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
//				PRT("�E�N���b�N���ꂽ�� (x,y)=%d,%d\n",pt.x,pt.y);
				hMenu = LoadMenu(ghInstance, "PRT_POPUP");
				hSubmenu = GetSubMenu(hMenu, 0);
			
				ClientToScreen(hWnd, &pt);
				TrackPopupMenu(hSubmenu, TPM_LEFTALIGN, pt.x, pt.y, 0, ghWindow, NULL);	// POPUP�̐e�����C���ɂ��Ȃ��ƃ��b�Z�[�W����������΂Ȃ��B
				DestroyMenu(hMenu);
			}
			break;

		case WM_CLOSE:
			fInfoWindow = 0;	// ���[�U��PrintWindow�݂̂���悤�Ƃ����ꍇ�B
			return( DefWindowProc( hWnd, msg, wParam, lParam) );

		case WM_DESTROY:
			hMenu = GetMenu(ghWindow);
			CheckMenuItem(hMenu, IDM_PRINT, MF_UNCHECKED);
			DeleteObject(SelectObject(hDC, hOldFont));	/* �t�H���g����� */
			if ( SaveMainWindowPlacement(hPrint,2) == 0 ) PRT("���W�X�g���Q�̃G���[�i�������݁j\n");
			hPrint = NULL;
			break;

		default:
			return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}
	return 0L;
}


#define CGF_URL "http://www32.ocn.ne.jp/~yss/cgfgoban.html"

/* �A�o�E�g�̕\�� */
LRESULT CALLBACK AboutDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/ )
{
	char sTmp[TMP_BUF_LEN];

	switch( msg ) {
		case WM_INITDIALOG:
			sprintf(sTmp,"%s %s",sVersion,sVerDate);
			SetDlgItemText(hDlg, IDM_CGF_VERSION, sTmp);
			SetDlgItemText(hDlg, IDM_HOMEPAGE, CGF_URL);
			return TRUE;	// TRUE ��Ԃ��� SetFocus(hDlg); �͕K�v�Ȃ��H

		case WM_COMMAND:
			if ( wParam == IDM_HOMEPAGE ) ShellExecute(hDlg, NULL, CGF_URL, NULL, NULL, SW_SHOWNORMAL);	// ���ꂾ����IE5.0?���N�������
			if ( wParam == IDOK || wParam == IDCANCEL ) EndDialog( hDlg, 0);
			break;
	}
	return FALSE;
}

// Gnugo�p�̐ݒ������_�C�A���O�̏���
BOOL CALLBACK GnuGoSettingProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/)
{
	HWND hWnd;
	int i;

	switch(msg) {
		case WM_INITDIALOG:
			if ( nGtpPath == 0 ) {	// ���o�^�Ȃ�W����ǉ��B�Ōオ�ŐV�B
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
			if ( wParam == IDCANCEL ) EndDialog(hDlg,FALSE);		// �L�����Z��
			if ( wParam == IDOK ) {
				char sTmp[TMP_BUF_LEN];
				GetDlgItemText(hDlg, IDM_GTP_PATH, sTmp, TMP_BUF_LEN-1);
				if ( strlen(sTmp) > 0 ) {	// ������z��̌�⃊�X�g���X�V�B
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



// �΋Ǐ�����ݒ肷��_�C�A���O�{�b�N�X�֐�
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
	static char *handi_str[handi_max] = { "�Ȃ�","2�q","3�q","4�q","5�q","6�q","7�q","8�q","9�q" };
	static char *err_str[] = { "���҂Ƃ��ʐM�΋ǂ͎w��ł��܂���" };
	static char *sKomi[3] = { "�Ȃ�","����","�ڔ�" };
	static char *sSize[3] = { "9�H","13�H","19�H" };
#endif
	static int fInitBoard = 0;	// �ǖʂ�����������ꍇ

	switch(msg) {
		case WM_INITDIALOG:
			SetFocus(hDlg);
//			PRT("lParam=%d\n",lParam);
			if ( lParam == IDYES ) {	// ���������đ΋�
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

			// �Œ�ł�1�̖��O��o�^����
			UpdateStrList(sPlayerName[1], &nNameList,NAME_LIST_MAX, sNameList );
			UpdateStrList(sPlayerName[0], &nNameList,NAME_LIST_MAX, sNameList );
			for (n=0;n<2;n++) {
				int type_col[2] = { IDC_BLACK_NAME, IDC_WHITE_NAME };
				HWND hWnd = GetDlgItem(hDlg, type_col[n]);
				SendMessage(hWnd, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
				for (i=0;i<nNameList;i++) SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)sNameList[i]);
				SendMessage(hWnd, CB_SETCURSEL, n, 0L);
			}
			SendMessage(hDlg, WM_COMMAND, MAKELONG(IDC_BLACK_TYPE,CBN_SELCHANGE), 0L);	// NNGS,SGMP�̕\����disenable�ɂ���
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
			
			// �u��
			{
				HWND hWnd = GetDlgItem(hDlg, IDM_HANDICAP);
				for (i=0;i<handi_max;i++) SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)handi_str[i]);
				SendMessage(hWnd, CB_SETCURSEL, 0, 0L);
//				if ( ban_size != BAN_19 ) EnableWindow(hWnd,FALSE);
				n = nHandicap;
				if ( n < 0 || n > handi_max ) n = 0;
				if ( n > 1 ) SendMessage(hWnd, CB_SETCURSEL, n-1, 0L);
			}

			// �R�~
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

			// �Ղ̃T�C�Y
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

			if ( nNngsIP == 0 ) {	// ���o�^�Ȃ�W����ǉ��B�Ōオ�ŐV�B
				UpdateStrList("192.168.0.1",         &nNngsIP,NNGS_IP_MAX, sNngsIP );
				UpdateStrList("nngs.computer-go.jp", &nNngsIP,NNGS_IP_MAX, sNngsIP );	// CGF��nngs�T�[�o
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
			if ( wParam == IDCANCEL ) EndDialog(hDlg,FALSE);		// �L�����Z��
			if ( wParam == IDOK ) {
				// ���͂��`�F�b�N
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

//				gct1 = clock();			// user �̎v�l���Ԍv���p
				
				for (n=0;n<2;n++) {
					int type_col[2] = { IDC_BLACK_NAME, IDC_WHITE_NAME };
					GetDlgItemText(hDlg, type_col[n], sPlayerName[n], TMP_BUF_LEN-1);
					if ( strlen(sPlayerName[n]) > 0 ) {	// ������z��̌�⃊�X�g���X�V�B
						UpdateStrList(sPlayerName[n], &nNameList,NAME_LIST_MAX, sNameList );
					}
				}
			
				// �V�K�΋ǂł͔Ֆʂ�������
				if ( fInitBoard ) {
					// �T�C�Y
					n = SendDlgItemMessage(hDlg, IDC_BOARD_SIZE, CB_GETCURSEL, 0, 0L);
					ban_size = BAN_9;
					if ( n == 1 ) ban_size = BAN_13;
					if ( n == 2 ) ban_size = BAN_19;
					init_ban();
					SendMessage(ghWindow,WM_COMMAND,IDM_BAN_CLEAR,0L);	// �Ֆʏ�����

					// �u��
					n = SendMessage(GetDlgItem(hDlg, IDM_HANDICAP), CB_GETCURSEL, 0, 0L);
					if ( n == CB_ERR ) { PRT("�u�Ύ擾Err\n"); n = 0; }
					if ( n ) {	// �u�΂�����
						nHandicap = n + 1;	// �������őł�
						SetHandicapStones();
					} else nHandicap = 0;
				}

				// �R�~
				n = SendDlgItemMessage(hDlg, IDM_KOMI, CB_GETCURSEL, 0, 0L);
				komi = n/2.0;
				if ( komi < 0 ) komi = 0;
				
				// NNGS���̎擾
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

				// COM�|�[�g�̎擾
				nCommNumber = 1 + SendDlgItemMessage(hDlg,IDM_COM_PORT, CB_GETCURSEL, 0, 0L);
		
				// �΋ǒ��ɓ���
				fNowPlaying = 1;
				MenuEnablePlayMode(0);	// �΋ǒ��͊����ړ����j���[���֎~����

				// GnuGo�̏�����
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
					// Gnugo���̔Ֆʂ�ݒ�
					int x,y,z,k;
					for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
						z = ((y+1)<<8) + x+1 ;
						k = ban[z];
						if ( k==0 ) continue;
						PutStoneGnuGo(z, (k==1));
					}
				}

				if ( sgmp ) {
					PostMessage(ghWindow,WM_COMMAND,IDM_INIT_COMM,0L);	// �ʐM�ΐ�̏���������
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
						// �W���̖��O���Z�b�g
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

// ���b�Z�[�W��\�����đ΋ǂ��~
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
		"���E��",
		"�R�E",
		"���ɐ΂�����܂�",
		"���̑��̃G���[",
		"�A���p�X�ŏI�ǂ��܂�",
		"�萔���������邽�ߒ��f���܂�",
		"�����ł�",
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


/*** �`��p�qWINDOW�̏����B����Window�ɕ`����s���B ***/
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
		"�����\�����͐΂͒u���܂���B\n",
		"�����ɂ͐΂�����܂���B\n",
		"���E��ł��I",
		"���ł���I",
		"�����\�����͐΂͎��܂���B\n",
#endif
	};

	if ( fPassWindows ) {	// �v�l���͂��ׂăJ�b�g����B
		return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}

	switch( msg ) {
		case WM_PAINT:
			PaintBackScreen(hWnd);
			return 0;

		case WM_LBUTTONDOWN:
			if ( fNowPlaying == 0 ) {	// �ՖʕҏW�@
//				SetIshi( z, 1 );
				return 0; 
			}
			z = ChangeXY(lParam);
//			PRT("����%x  ",z);
			if ( z == 0 ) return 0;
			if ( endgame_board_type ) {
				PRT("%s",err_str[0]);
				return 0;
			}
			if ( ban[z] != 0 ) {
				PRT("%s",err_str[1]);
				return 0;
			}

			col = GetTurnCol();		// ��Ԃƒu�΂��肩�H�ŐF�����߂�

			k = make_move(z,col);
			if ( k==MOVE_SUICIDE ) {
				MessageBox(hWnd,err_str[2],"Move error",MB_OK);	// ���E��
				return 0;
			}
			if ( k==MOVE_KOU ) {
				MessageBox(hWnd,err_str[3],"Move error",MB_OK);	// �R�E
				return 0;
			}
			if ( k!=MOVE_SUCCESS ) {
				PRT("move_err\n");
				debug();
			}
			
			if ( IsGnugoBoard()    ) UpdateGnugoBoard(z);
			if ( IsCommunication() ) SendMove(z);

			tesuu++;	all_tesuu = tesuu;
			kifu[tesuu][0] = z;	// �������L��
			kifu[tesuu][1] = col;	// ��ԂŐF�����߂�
			kifu[tesuu][2] = (int)GetSpendTime(gct1);

			PaintUpdate();
			if ( SoundDrop ) {
//				MessageBeep(MB_OK);	// ����炵�Ă݂܂���
				sndPlaySound("stone.wav", SND_ASYNC | SND_NOSTOP);	// MMSYSTEM.H ���C���N���[�h����K�v����B�܂�WinMM.lib�������N
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
			if ( fNowPlaying == 0 ) {	// �ՖʕҏW�@
				SetIshi( z, 2 );
			}
			return 0;
*/
		default:
			return( DefWindowProc( hWnd, msg, wParam, lParam) );
	}
//	return 0L;
}

// �`��pWindow�𓯎��ɓ�����
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

// Windows�ֈꎞ�I�ɐ����n���BUser ����̎w���ɂ��v�l��~�B
void PassWindowsSystem(void)
{
	MSG msg;

	fPassWindows = 1;	// Windows�ɏ�����n���Ă�B
	if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) ) {
		TranslateMessage( &msg );						// keyboard input.
		DispatchMessage( &msg );
	}
	fPassWindows = 0;
}	


/*** �������Z�[�u���� ***/
void KifuSave(void)
{
	OPENFILENAME ofn;
	char szDirName[256];
	char szFile[256], szFileTitle[256];
	UINT  cbString;
	char  chReplace;    /* szFilter������̋�؂蕶�� */
	char  szFilter[256];
	char  Title[256];	// Title
	HANDLE hFile;		// �t�@�C���̃n���h��
	int i;
	unsigned long RetByte;		// �������܂ꂽ�o�C�g���i�_�~�[�j
	static int OnlyOne = 0;	// �A���ΐ�̉񐔁i�ŏ���1�񂾂��������܂��)�B

	time_t timer;
	struct tm *tblock;
	unsigned int month,day,hour,minute;

	/* �V�X�e�� �f�B���N�g�������擾����szDirName�Ɋi�[ */
	GetSystemDirectory(szDirName, sizeof(szDirName));
	szFile[0] = '\0';

	if ((cbString = LoadString(ghInstance, IDS_SAVE_FILTER,
	        szFilter, sizeof(szFilter))) == 0) {
	    return;
	}
	chReplace = szFilter[cbString - 1]; /* ���C���h �J�[�h���擾 */

	for (i = 0; szFilter[i] != '\0'; i++) {
	    if (szFilter[i] == chReplace)
	       szFilter[i] = '\0';
	}


	/* �����̎擾 */
	timer = time(NULL);
	tblock = localtime(&timer);   /* ���t�^�������\���̂ɕϊ����� */
//	PRT("Local time is: %s", asctime(tblock));
//	second = tblock->tm_sec;      /* �b */
	minute = tblock->tm_min;      /* �� */
	hour   = tblock->tm_hour;     /* �� (0�`23) */
	day    = tblock->tm_mday;     /* �����̒ʂ����� (1�`31) */
	month  = tblock->tm_mon+1;    /* �� (0�`11) */
//	year   = tblock->tm_year + 1900;     /* �N (���� - 1900) */
//	week   = tblock->tm_wday;     /* �j�� (0�`6; ���j = 0) */
//	PRT( "Date %02x-%02d-%02x / Time %02x:%02x:%02x\n", year, month, day, hour, minute, second );

//	sprintf( szFileTitle, "%02d%02d%02d%02d", month, day, hour, minute );	// �t�@�C��������t�{�b��
//	if ( fAutoSave ) sprintf( szFile, "fig%03d.sgf",++OnlyOne );	// �����Z�[�u�̎��B
	if ( fAutoSave ) sprintf( szFile, "%x%02d@%02d%02d.sgf", month, day, hour,++OnlyOne );	// �����Z�[�u�̎��B
	else             sprintf( szFile, "%x%02d-%02d%02d",     month, day, hour, minute );	// �t�@�C��������t�{�b��


	/* �\���̂̂��ׂẴ����o��0�ɐݒ� */
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
	ofn.lpstrInitialDir = NULL;		// �J�����g�f�B���N�g����
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt	= "sgf";	// �f�t�H���g�̊g���q�� ".sgf" ��
	
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

		// �^�C�g���Ƀt�@�C������t����
		sprintf(Title,"%s - %s",ofn.lpstrFile, szTitleName);
		SetWindowText(ghWindow, Title);

		// �t�@�C�����X�g���X�V�B��ԍŋߊJ�������̂�擪�ցB
		UpdateRecentFile(ofn.lpstrFile);
		UpdateRecentFileMenu(ghWindow);

		if ( stricmp(ofn.lpstrFile + ofn.nFileExtension, "sgf" ) == 0 || fAutoSave ) {
			SaveSGF();	// SGF�ŏ����o��
		} else {
			SaveIG1();	// IG1 �`���ŏ������ށB
		}
		WriteFile( hFile,(void *)SgfBuf, strlen(SgfBuf), &RetByte, NULL );
		CloseHandle(hFile);		// �t�@�C�����N���[�Y
	}
}


/*** ���������[�h���� ***/
int KifuOpen(void)
{
	OPENFILENAME ofn;
	char szDirName[256];
	char szFile[256], szFileTitle[256];
	UINT  cbString;
	char  chReplace;    /* szFilter������̋�؂蕶�� */
	char  szFilter[256];
	char  Title[256];	// Title

	HANDLE hFile;		// �t�@�C���̃n���h��
	int i;

	szFile[0] = '\0';
	if ((cbString = LoadString(ghInstance, IDS_LOAD_FILTER, szFilter, sizeof(szFilter))) == 0) return FALSE;
	chReplace = szFilter[cbString - 1]; /* ���C���h �J�[�h���擾 */
	for (i = 0; szFilter[i] != '\0'; i++) if (szFilter[i] == chReplace) szFilter[i] = '\0';

	/* �\���̂̂��ׂẴ����o��0�ɐݒ� */
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = ghWindow;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile= szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	GetCurrentDirectory(sizeof(szDirName), szDirName);	// �J�����g�f�B���N�g�������擾����szDirName�Ɋi�[
	ofn.lpstrInitialDir = szDirName;	// �J�����g�f�B���N�g����
//	ofn.lpstrInitialDir = NULL;			// ---> ���ꂾ��Win98�ȍ~��*.amz�t�@�C�����Ȃ���MyDocuments���W���ɂ����
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if ( strlen(cDragFile) || GetOpenFileName(&ofn) ) {
		if ( strlen(cDragFile) ) {	// �_�u���N���b�N�ŋN�����ꂽ��
			char *p;

			if ( cDragFile[0] == '"' ) {	// �O��� "" ������
				strcpy(szFile,cDragFile+1);
				strcpy(cDragFile,szFile);
				cDragFile[ strlen(cDragFile)-1 ] = 0;
			}

			strcpy(szFile,cDragFile);	// szFile�ɃR�s�[
			ofn.lpstrFile = szFile;
			p = strrchr( ofn.lpstrFile, '.' );	// �Ō�̃s���I�h��T��
			if ( p == NULL ) ofn.nFileExtension = 0;	// �ǂ݂̂��G���[
			else             ofn.nFileExtension = (p - ofn.lpstrFile)+1;
			PRT("%s,%d,%s\n",ofn.lpstrFile,ofn.nFileExtension,ofn.lpstrFile + ofn.nFileExtension);
			strcpy(ofn.lpstrFileTitle,ofn.lpstrFile);
			cDragFile[0] = '\0';	// �t�@�C����������	
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
 
		// �^�C�g���Ƀt�@�C������t����
		sprintf(Title,"%s - %s",ofn.lpstrFileTitle, szTitleName);
		SetWindowText(ghWindow, Title);

		// �t�@�C�����X�g���X�V�B��ԍŋߊJ�������̂�擪�ցB
		UpdateRecentFile(ofn.lpstrFile);
		UpdateRecentFileMenu(ghWindow);


		PRT("\n%s\n",ofn.lpstrFile);

		// �S�Ẵt�@�C�����o�b�t�@�ɓǂݍ���
		ReadFile( hFile, (void *)SgfBuf, SGF_BUF_MAX, &nSgfBufSize, NULL );
		CloseHandle(hFile);		// �t�@�C�����N���[�Y

		int fSgf = 0;
		// SGF (Standard Go Format) ��ǂݍ��ށB
		if ( stricmp(ofn.lpstrFile + ofn.nFileExtension, "sgf" ) == 0 ) fSgf = 1;
		// IG1 (Nifty-Serve GOTERM) ��ǂݍ��ށB
		if ( stricmp(ofn.lpstrFile + ofn.nFileExtension, "ig1" ) == 0 ) fSgf = 0;

		LoadSetSGF(fSgf);
	}

//	PRT("�ǂݍ��񂾎萔=%d,�S�萔=%d\n",tesuu,all_tesuu);
	return TRUE;
}


unsigned long nSgfBufSize;	// �o�b�t�@�Ƀ��[�h�����T�C�Y
unsigned long nSgfBufNum;	// �o�b�t�@�̐擪�ʒu������
char SgfBuf[SGF_BUF_MAX];	// �o�b�t�@

// SGF�p�̃o�b�t�@����1�o�C�g�ǂݏo���B
char GetSgfBuf(int *RetByte)
{
	*RetByte = 0;
	if ( nSgfBufNum >= nSgfBufSize ) return 0;
	*RetByte = 1;
	return SgfBuf[nSgfBufNum++];
}

// ��s�ǂݍ���
int ReadOneLine( char *lpLine ) 
{
	int i,RetByte;
	char c;

	i = 0;
	for ( ;; ) {
		c = GetSgfBuf(&RetByte);
		if ( RetByte == 0 ) break;	// EOF;
		if ( c == '\r' ) c = ' ';	// ���׍H�B���������Ӗ��Ȃ�
		lpLine[i] = c;
		i++;
		if ( i >= 255 ) break;
		if ( c == '\r' || c == '\n' ) break;	// ���s��NULL��
	}
	lpLine[i] = 0;	// �Ō��NULL������邱�ƁI
	return (i);
}

// SGF��ǂݍ���
int ReadSGF(void)
{
	int RetByte,nLen;
	int i,x,y,z;
	char c;
	char sCommand[256];	// SGF �p�̃R�}���h���L��
	BOOL fKanji;
	BOOL fAddBlack = 0;		// �u���΃t���O 


	nSgfBufNum = 0;

	for ( ;; ) {
		c = GetSgfBuf( &RetByte );
		if ( RetByte == 0 ) break;	// EOF;
		if ( c == '\r' || c == '\n' ) continue;	// ���s�R�[�h�̓p�X
		if ( c == '(' ) break;
	}
	if ( c != '(' )	{
		PRT("SGF�t�@�C���̓ǂݍ��ݎ��s�B'(' ��������Ȃ�\n");
//		break;
	}

	nLen = 0; 
	fKanji = 0 ;
	for ( ;; ) {
		c = GetSgfBuf( &RetByte );
		if ( RetByte == 0 ) break;	// EOF;
		if ( fKanji ) {		// �����̏ꍇ�͖�������2�����ڂ�ǂށB
			sCommand[nLen] = c;
			nLen++;
			fKanji = 0;
			continue;
		}
		if ( c == '\r' || c == '\n' ) continue;	// ���s�R�[�h�̓p�X
		if ( 0 <= c && c < 0x20 ) continue;	// �󔒈ȉ��̃R�[�h�iTAB�܂ށj���p�X�B
		if ( c == ';' ) {
			fAddBlack = 0;		// �u���΃t���O�I�t
			nLen = 0;			// ���������Ă݂�B
			continue;			// ��؂�A���݂͖����B
		}
		if ( c == ')' && nLen == 0 ) break;		// SGF File �̏I���B
	
		if ( fAddBlack != 0 && ( !(nLen==0 && c != '[') ) ) {	// �u���΂̎��̏���
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
					PRT("�u���Δ͈̓G���[\n");
					break;
				}
				z = y * 256 + x;
				ban_start[z] = fAddBlack;
				PRT("A%c[%c%c]",'B'*(fAddBlack==1)+'W'*(fAddBlack==2), sCommand[1],sCommand[2]);
			}
			continue;
		}

				
		if ( nLen == 0 ) {	// �ŏ��̈ꕶ���ځB
			if ( c=='[' || c==']' ) {
				PRT("[]?,");
//				break;
				continue;
			}
//			if ( 'a' <= c && c <= 'z' ) continue;	// �������͖����B
			if ( 'A' <= c && c <= 'Z' ) {			// �啶���B�R�}���h�B
				sCommand[nLen] = c;
				nLen++;
				fAddBlack = 0;		// �u���΃t���O�I�t
			}
			continue;
		}		
		if ( nLen == 1 ) {
			if ( 'A' <= c && c <= 'Z' ) {			// �啶���B�R�}���h�B
				sCommand[nLen] = c;
				nLen++;

				if ( strncmp(sCommand,"AB",2)==0 ) {	// �u���΁i����u���j
					// ���ɑ啶���A�܂��� ";" ������܂Œu���΂�T���B
					// AB[jp][jd][pj][dj][jj][dp][pd][pp][dd]  ;W[qf];B[md];W[qn];B[mp];W[nj];
					fAddBlack = 1;
					nLen = 0;
//					PRT("AB�𔭌��I\n");
				}
				if ( strncmp(sCommand,"AW",2)==0 ) {	// �u���΁i����u���j
					fAddBlack = 2;
					nLen = 0;
				}

				continue;
			}
			if ( c == '[' ) {	// B,W,C �i���A���A�R�����g�j�̎��B
				sCommand[nLen] = c;
				nLen++;
			}
			continue;
		}
		if ( nLen >= 2 ) {
			if ( nLen == 2 && sCommand[1] != '[' && c != '[' ) continue;
			sCommand[nLen] = c;
			if ( nLen < 128 ) nLen++;	// 80�����ȏ�͖�������
			if ( IsKanji( (unsigned char)c ) ) fKanji = 1;
//			PRT("%d:%d,",fKanji,(unsigned char)c );

			if ( c == ']' ) {	// �f�[�^�I���B
				if ( sCommand[1] != '[' || sCommand[0] == 'C' ) {	// �ړ���ȊO�͖���
					sCommand[nLen] = 0;

					if ( strncmp(sCommand,"WL",2)==0 || strncmp(sCommand,"BL",2)==0 ) {	// ���A���̎c�莞��
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

					if ( sCommand[0] == 'C' && tesuu > 0 ) PRT("%3d:",tesuu);	// �r���̃R�����g
					PRT("%s\n",sCommand);
																				
					if ( strncmp(sCommand,"PB",2)==0 ) {	// �΋ǎҖ�����荞��
						sCommand[nLen-1] = 0;
						strcpy( sPlayerName[0], &sCommand[3] );
					}
					if ( strncmp(sCommand,"PW",2)==0 ) {
						sCommand[nLen-1] = 0;
						strcpy( sPlayerName[1], &sCommand[3] );
					}

					if ( strncmp(sCommand,"SZ",2)==0 ) {	// �Ղ̑傫��
						sCommand[nLen-1] = 0;
						ban_size = atoi( &sCommand[3] );
//						if ( ban_size == BAN_19 || ban_size == BAN_13 || ban_size == BAN_9 ) ;
//						else ban_size = 19;
						init_ban();
						// �Ֆʂ������� ---> �u����̂���
						for (i=0;i<BANMAX;i++) {
							ban_start[i] = ban[i];	// �J�n�ǖʂ�ݒ�B������Ԃ͑S�ĂO
							if ( ban_start[i] != 3 ) ban_start[i] = 0;
						}
						SendMessage(hWndMain,WM_COMMAND,IDM_BAN_CLEAR,0L);	// �I�[�v���łł͔Ֆʏ�����
					}

					if ( strncmp(sCommand,"RE",2)==0 ) {	// ����
						sCommand[nLen-1] = 0;
						strcpy( sResult, &sCommand[3] );
					}
					if ( strncmp(sCommand,"PC",2)==0 ) {	// �ꏊ
						sCommand[nLen-1] = 0;
						strcpy( sPlace, &sCommand[3] );
					}
					if ( strncmp(sCommand,"GN",2)==0 ) {	// ���E���h
						sCommand[nLen-1] = 0;
						strcpy( sRound, &sCommand[3] );
					}
					if ( strncmp(sCommand,"EV",2)==0 ) {	// �C�x���g��
						sCommand[nLen-1] = 0;
						strcpy( sEvent, &sCommand[3] );
					}
					if ( strncmp(sCommand,"DT",2)==0 ) {	// ���t
						sCommand[nLen-1] = 0;
						strcpy( sDate, &sCommand[3] );
					}
					if ( strncmp(sCommand,"KM",2)==0 ) {	// �R�~
						sCommand[nLen-1] = 0;
						komi = atof(&sCommand[3] );
					}
					if ( strncmp(sCommand,"HA",2)==0 ) {	// �u�΂̐�
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
				// �ړ���ȊO�͖���
				if ( sCommand[0] == 'V' || sCommand[0] == 'N' ) {	// V[B+5]�͕]���l�H
					PRT("%s",sCommand);
					nLen = 0;
					continue;
				}
						
				if ( sCommand[0] == 'T' && sCommand[1] =='[' ) {	// �����
					sCommand[nLen-1] = 0;
					kifu[tesuu][2] = atoi( sCommand+2 );
					nLen = 0;
					continue;
				}
						
				if ( sCommand[0] != 'B' && sCommand[0] != 'W' ) {	// �ړ���ȊO�͖���
					sCommand[nLen] = 0;
					PRT("%s ---> �R�}���h�G���[�I�I�I\n",sCommand);
//					nLen = 0;
					break;
				}
//				if ( tesuu == 0 && sCommand[0] == 'W' ) fWhiteFirst = 1;	// ������ł�
				c = sCommand[2];
				if ( c >= 'A' && c<= 'Z' ) c += 32;	// ugf�Ή�
				x = c - 'a' + 1;
				c = sCommand[3];
				if ( c >= 'A' && c<= 'Z' ) { c += 32; c = 'a'-2 + 'u' - c; }
				y = c - 'a' + 1;
				if ( x == 20 || y == 20 ) {
					x = 0;
					y = 0;
				} else if ( nLen != 5 || x < 1 || x > ban_size || y < 1 || y > ban_size ) {
					sCommand[nLen] = 0;
					PRT("%s:  �ړ��ꏊ�G���[\n",sCommand);
//					nLen = 0;
//					break;
					PRT("�s���ȏꏊ�����A�p�X�Ƃ݂Ȃ��đ��s\n");
					x = 0; y = 0;
				}
				z = y * 256 + x;
				tesuu++;	all_tesuu = tesuu;
				kifu[tesuu][0] = z;
				kifu[tesuu][1] = 2 - (sCommand[0] == 'B');	// �΂̐F
				kifu[tesuu][2] = 0;	// �v�l���ԁA���̌゠��ΐݒ肳���B
				sCommand[nLen] = 0;
//				PRT("%s, z=%04x: ",sCommand,z);
				nLen = 0;
			}

			if ( nLen >= 255 ) {
				PRT("�f�[�^�̓��e��255�����ȏ�B���ƒ�����\n");
				break;
			}
		}
	}

//	for (i=0;i<2;i++) {	if ( strstr( sPlayerName[i], "Dai") != 0 ) sprintf(sPlayerName[i],"GnuGo3.5.4");}
//	komi = 6;

	return 0;
}

// SGF�̃o�b�t�@�ɏ����o��
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
	nSgfBufSize = nSgfBufNum;	// �ꉞ���
}

// SGF�������o��
void SaveSGF(void)
{
	int i,j,x,y,z,m0,m1,s0,s1;
	int fSgfTime,fAdd;
	time_t timer;
	struct tm *tb;

	nSgfBufNum = 0;

	// �����̎擾
	timer = time(NULL);
	tb = localtime(&timer);   /* ���t�^�������\���̂ɕϊ����� */

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

	// �Ֆʂ�������������
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
	// �����2��p�X
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
		// �����ɏ���Ԃ�ۑ��B�Ǝ��g���B
		if ( fSgfTime ) { 
			SGF_OUT("T[%2d]",kifu[i][2]);
		}

		j++;
		if ( j==10 - 4*fSgfTime ) {	// 10�育�Ƃɉ��s����B
			SGF_OUT("\r\n");
			j = 0;
		}
	}
	SGF_OUT(")\r\n");
}

// ig1�t�@�C����ǂ�
int ReadIG1(void)
{
	int i,x=0,y,yl,yh=0,z, xFlag,yFlag=0, nLen;
	int RetByte;		// �ǂݍ��܂ꂽ�o�C�g��
	char c;
	char  lpLine[256];	// �P����ǂ�
	
	nSgfBufNum = 0;
	
	for ( i=0;i<255;i++) {
		sPlayerName[0][i] = 0;
		sPlayerName[1][i] = 0;
	}

	// ��s�����[�h����
	// "INT" ��������܂Ń��[�h
	for ( ;; ) {
		nLen = ReadOneLine( lpLine );
		if ( nLen == 0 ) {
			PRT("EOF - �ǂݍ��ݎ��s�BMOV ��������Ȃ�\n");
			break;
		}
		lpLine[nLen] = 0;
		PRT(lpLine);
		if ( lpLine[0] == 'M' && lpLine[1] == 'O' && lpLine[2] == 'V' ) break;
		if ( strstr( lpLine, "PLAYER_BLACK" ) ) strncpy( sPlayerName[0], &lpLine[13], nLen-13-1);
		if ( strstr( lpLine, "PLAYER_WHITE" ) ) strncpy( sPlayerName[1], &lpLine[13], nLen-13-1);
		if ( strstr( lpLine, "���F" ) ) strncpy( sPlayerName[0], &lpLine[4], nLen-4-1);
		if ( strstr( lpLine, "���F" ) ) strncpy( sPlayerName[1], &lpLine[4], nLen-4-1);
	}

	if ( nLen ) {	
		tesuu = 0;
		xFlag = 1;
		for ( ;; ) {
			c = GetSgfBuf( &RetByte );
			if ( RetByte == 0 ) break;	// EOF;
			if ( c == '\r' || c == '\n' ) continue;	// ���s�R�[�h�̓p�X
			if ( c <= 0x20 ) continue;	// �󔒈ȉ��̃R�[�h�iTAB�܂ށj�͋�؂�
			if ( c == '-' ) break;
			if ( xFlag == 1 && 'A' <= c && c <= 'T'	) {
				x = c  - 'A' - (c > 'I') + 1;	// 'I'�𒴂�����-1	
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
				kifu[tesuu][1] = 2 - ( tesuu & 1 );	// ��肪��
			}
		}
	}
	return 0;
}

// ig1�t�@�C���������o��
void SaveIG1(void)
{
	int i,j,x,y,z;
	time_t timer;
	struct tm *tb;

	nSgfBufNum = 0;

	// �����̎擾
	timer = time(NULL);
	tb = localtime(&timer);   /* ���t�^�������\���̂ɕϊ����� */

	// \n �������� 0x0a �݂̂�����D�܂����Ȃ��̂� 0x0d(\r)��ǉ� 
	SGF_OUT("#COMMENT\r\n");
	SGF_OUT("#--- CgfGoBan �����t�@�C���iGOTERM�ƌ݊�������j---\r\n");
	SGF_OUT("#�΋Ǔ� %4d-%02d-%02d\r\n",tb->tm_year+1900,tb->tm_mon+1,tb->tm_mday); 
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

// SGF�����[�h���ĔՖʂ��\������
void LoadSetSGF(int fSgf)
{
	int i;

	tesuu = all_tesuu = 0;
	nHandicap = 0;

	// �Ֆʂ������� ---> �u����̂���
	for (i=0;i<BANMAX;i++) {
		ban_start[i] = ban[i];	// �J�n�ǖʂ�ݒ�B������Ԃ͑S�ĂO
		if ( ban_start[i] != 3 ) ban_start[i] = 0;
	}

	if ( fSgf ) ReadSGF();	// SGF��ǂݍ���
	else        ReadIG1();	// ig1�t�@�C����ǂ�

#ifdef CGF_E
	PRT("Move Numbers=%d\n",tesuu);
#else
	PRT("�ǂݍ��񂾎萔=%d\n",tesuu);
#endif
	// �Ֆʂ������Ֆʂ�
	for (i=0;i<BANMAX;i++) ban[i] = ban_start[i];
//	PRT("ban_size=%d\n",ban_size);
	reconstruct_ban();
	tesuu = all_tesuu;

	for (i=1; i<=tesuu; i++) {
		int z   = kifu[i][0];
		int col = kifu[i][1];
		if ( make_move(z,col) != MOVE_SUCCESS ) {
			PRT("�����Ǎ��G���[ z=%x,col=%d,tesuu=%d \n",z,col,i);
			break;
		}
	}

	PaintUpdate();
}

/*** printf �̑�p ***/
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

	//// FILE���o�p ////
	if ( fFileWrite == 1 ) {
		strcpy(sOut,sCgfDir);
		strcat(sOut,"\\output");	// �f���o���t�@�C����
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
			if ( PRT_x == PRetX - 1 ) {nRet++;PRT_Scroll();}	// ����100�s
			else PRT_x++;
		}
	}
	if ( hPrint != NULL && IsWindow(hPrint) ) {
		if ( nRet == 1 ) {	// �X�N���[�����Ȃ��ꍇ�������`��
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

// PRT�̓��e���N���b�v�{�[�h�ɃR�s�[
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
//		PRT("�N���b�v�{�[�h��PRT���o�͂��܂���\n");
	}

}


/*** �͌�Ղ̏������W�l���擾�i�͌�Վ��̂̏k���ɔ�����j***/
void GetScreenParam( HWND hWnd,int *dx, int *sx,int *sy, int *db )
{
	RECT rc;
	
	GetClientRect( hWnd, &rc );

	*sx = 22;			// �͌�Ղ̍��[�����W	 
	*sy = 20;

	*dx = 2*(rc.bottom - *sy -15 ) / ( 2*ban_size+1);	// -15 �͋C��
		
	if ( *dx <= 10 ) *dx = 10;	// �Œ��10
	if ( *dx & 1 ) *dx -= 1;	// ������
//	if ( !(*dx & 2) ) *dx -= 2;	// 18 + 4*x �̔{����

	*db = (*dx*3) / 4;	// �͌�Ղ̒[�Ɛ��̊Ԋu
	if ( !(*db & 1) ) *db += 1;	// ���
//	if ( iBanSize==BAN_19 ) *dx = 26;
//	if ( iBanSize==BAN_13 ) *dx = 38;
//	if ( iBanSize==BAN_9  ) *dx = 50;
//	*dx = 26;		// ���ڂ̊Ԋu (640*480..20, 800*600..25, 1280*780..34)
					// 18 <= dx <= 36 (24 +- 4 �̔{���͈Â��ՂŘg���ɂ���)
}

/*** WM_PAINT �ŉ�ʂ��ĕ`�悷��֐� ***/

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
	"��", 
	"��",
	"�g�l�F%d",
	"��%3d��",
	"���̔Ԃł�",
	"���̔Ԃł�",
	"����",
	"%s�� %.1f �ڏ�",
	"(����)",
	"��", 
	"��",
#endif
};

#ifdef CGF_E
char FontName[] = "FixedSys";	// "MS UI Gothic" "MS Sans Serif" "Courier New" "BatangChe"(win98�ł͌��ɂ���)
#else
char FontName[] = "�W������";
#endif

// �Տ�ɐ��l����������
void DrawValueNumber(HDC hDC,int size, int col, int xx, int yy, char *sNum)
{
	TEXTMETRIC tm;
	HFONT hFont,hFontOld;
	int n = strlen(sNum);

	hFont = CreateFont(					/* �t�H���g�̍쐬 */
		size,0,0,0,FW_NORMAL,
		0,0,0,	DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
           DEFAULT_PITCH, FontName);
	hFontOld = (HFONT)SelectObject(hDC,hFont);
	GetTextMetrics(hDC, &tm);
	if ( col==2 ) SetTextColor( hDC, rgbBlack );	// ����
	else          SetTextColor( hDC, rgbWhite );	// ����
	TextOut( hDC, xx-tm.tmAveCharWidth*n/2 - (n==3)*1, yy-size/2, sNum ,n);
	SelectObject( hDC, hFontOld );
	DeleteObject(hFont);
}

// ���ۂɕ`����s��
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

	hBrushBlack = CreateSolidBrush( rgbBlack );	// ����
	hBrushWhite = CreateSolidBrush( rgbWhite );	// ����

	SetTextColor( hDC, rgbText );	//�@�����F
//	SetBkColor( hDC, RGB(0,128,0) );		//�@�w�i��Z���΂�

	// �w�i���܂��`�悷��B
	RECT rcFull;
	GetClientRect( hWnd, &rcFull );	// ���݂̉�ʂ̑傫�����擾�B�O���t�̑傫�������̃T�C�Y�ɍ��킹��
	hBrush = CreateSolidBrush(rgbBlack);
	FillRect(hDC, &rcFull, hBrush );
	DeleteObject(hBrush);

	/* �ՖʑS�̂̕\�� */
	SetRect( &rRect , sx, sy, sx+dx*(ban_size-1)+db*2, sy+dx*(ban_size-1)+db*2 );
	n = ID_NowBanColor - ID_BAN_1;
//	PRT("n=%d\n",n);
	if ( n >= RGB_BOARD_MAX || n < 0 ) { PRT("�F���Ȃ� n=%d\n",n); debug(); }
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

	/* ���̓_ */
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

	SetBkMode( hDC, TRANSPARENT);	// TRANSPARENT ... �d�˂�, OPAQUE...�h��Ԃ�

	// �莚�uCGF��Ձv�̕\��
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
		DEFAULT_PITCH | FF_SCRIPT, "�W���S�V�b�N");
	y += 10;
#endif
	hFontOld = (HFONT)SelectObject( hDC, hFont );
	SetTextColor( hDC, rgbBlack );		// �e������
	n = strlen(szTitleName);
	TextOut( hDC, x+4, y+4, szTitleName,n );
	SetTextColor( hDC, rgbWhite );	//�@�����𔒂�
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


	/* ���� */
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
			// �`�F�X�����W�n
			sprintf( sNum,"%d",ban_size-i);	
			TextOut( hDC, xx + (ban_size-i<=9)*n , sy+db-8 + i*dx, sNum ,strlen(sNum) );
			sprintf( sNum,"%c",i+'A'+(i>7) );	// A-T �\�L
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

	/* �Ֆʂɂ���΂�\�� */
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + (x+1);
	   	xx = sx+db+dx*x;
	   	yy = sy+db+dx*y;
		k = ban[z];
		if ( k == 0 ) continue;
		if ( k == 1 ) SelectObject( hDC, hBrushBlack );		// ����
		else          SelectObject( hDC, hBrushWhite );		// ����

//	   	Ellipse( hDC, xx-dx/2,yy-dx/2, xx+dx/2+1,yy+dx/2+1);		// �f�B�X�v���C�h���C�o�ɂ��H
	   	Ellipse( hDC, xx-dx/2,yy-dx/2, xx+dx/2,  yy+dx/2  );		// Windows98 ViperV550 �ł͂�����B
//		Chord( hDC,  xx-dx/2,yy-dx/2, xx+dx/2,yy+dx/2, xx-dx/2,yy-dx/2, xx-dx/2,yy-dx/2 );	// Ellipse �͓��삪���������i�͂ݏo��jWin98 ViperV550�ł͕`�悳��Ȃ����Ƃ���B

		if ( kifu[tesuu][0] == z ) {	// ���O�ɑł��ꂽ�΂Ɏ萔��\��
			sprintf( sNum,"%d",tesuu);
			n = strlen(sNum);
			if ( n==1 ) size = dx*3/4;
			else if ( n==2 ) size = dx*7/10;
			else size = dx*7/12;
			DrawValueNumber(hDC, size, k, xx, yy, sNum);
		}
	}

	
	// ������\��
	if ( endgame_board_type == ENDGAME_STATUS ) {
		for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
			z = ((y+1)<<8) + (x+1);
	   		xx = sx+db+dx*x;
	   		yy = sy+db+dx*y;
			
			k = ban[z];
			n = endgame_board[z];
			if ( k == 0 ) {
				// �m��n��\��
				hBrush = NULL;
				if ( n == GTP_WHITE_TERRITORY ) hBrush = hBrushWhite;
				if ( n == GTP_BLACK_TERRITORY ) hBrush = hBrushBlack;
				if ( hBrush == NULL ) continue;
				SelectObject( hDC, hBrush );
				Rectangle( hDC, xx-dx/4,yy-dx/4, xx+dx/4,yy+dx/4);
				continue;
			}
			if ( n != GTP_DEAD ) continue;

		   	// ����ł���΂Ɂ~������
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
	// ���l��\��
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
	// �}�`��\��
	if ( endgame_board_type == ENDGAME_DRAW_FIGURE ) {
		for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
			int m,d;
			z = ((y+1)<<8) + (x+1);
	   		xx = sx+db+dx*x;
	   		yy = sy+db+dx*y;
//			k = ban[z];
			n = endgame_board[z];
			
			m = n & ~(FIGURE_BLACK | FIGURE_WHITE);	// �F�w�������
			d = 2;	// �y���̑���
			if ( m == FIGURE_TRIANGLE || m == FIGURE_SQUARE	|| m == FIGURE_CIRCLE ) d = 1;
			if ( n & FIGURE_WHITE ) hPen = CreatePen(PS_SOLID, d, rgbWhite );
			else                    hPen = CreatePen(PS_SOLID, d, rgbBlack );
			hPenOld = (HPEN)SelectObject( hDC, hPen );

			if ( n & FIGURE_WHITE ) hBrush = hBrushWhite;
			else                    hBrush = hBrushBlack;
			SelectObject( hDC, hBrush );


			if ( m == FIGURE_TRIANGLE ) {	// �O�p�`
				int x1 = dx /  3;
				int y1 = dx /  3;
				int y2 = dx /  5;
				MoveToEx( hDC, xx   ,yy-y1, NULL );
				LineTo(   hDC, xx+x1,yy+y2 );
				LineTo(   hDC, xx-x1,yy+y2 );
				LineTo(   hDC, xx   ,yy-y1 );
			}
			if ( m == FIGURE_SQUARE ) {		// �l�p
				int x1 = dx / 4;
				Rectangle( hDC, xx-x1,yy-x1, xx+x1,yy+x1);
			}
			if ( m == FIGURE_CIRCLE ) {		// �~
				int x1 = dx / 5;
				Chord( hDC, xx-x1,yy-x1, xx+x1,yy+x1, xx-x1,yy-x1, xx-x1,yy-x1 );
			}
			if ( m == FIGURE_CROSS ) {		// �~
				int x1 = dx / 6;
				MoveToEx( hDC, xx-x1,yy-x1, NULL );
				LineTo(   hDC, xx+x1,yy+x1 );
				MoveToEx( hDC, xx-x1,yy+x1, NULL );
				LineTo(   hDC, xx+x1,yy-x1 );
			}
			if ( m == FIGURE_QUESTION ) {	// "�H"�̋L��	
				sprintf( sNum,"?" );
				size = dx*10/12;
				if ( n & FIGURE_BLACK ) k = 2;
				else                    k = 1;
				DrawValueNumber(hDC, size, k, xx, yy, sNum);
			}
			if ( m == FIGURE_HORIZON ) {	// ����
				int x1 = dx * 10 / 22;
				MoveToEx( hDC, xx-x1,yy, NULL );
				LineTo(   hDC, xx+x1,yy );
			}
			if ( m == FIGURE_VERTICAL ) {	// �c��
				int x1 = dx * 10 / 22;
				MoveToEx( hDC, xx,yy-x1, NULL );
				LineTo(   hDC, xx,yy+x1 );
			}
			if ( m == FIGURE_LINE_LEFTUP ) {	// �΂߁A���ォ��E��
				int x1 = dx * 10 / 22;
				MoveToEx( hDC, xx-x1,yy-x1, NULL );
				LineTo(   hDC, xx+x1,yy+x1 );
			}
			if ( m == FIGURE_LINE_RIGHTUP ) {	// �΂߁A��������E��
				int x1 = dx * 10 / 22;
				MoveToEx( hDC, xx-x1,yy+x1, NULL );
				LineTo(   hDC, xx+x1,yy-x1 );
			}

			SelectObject( hDC, hPenOld );
			DeleteObject(hPen);
		}
	}



	SelectObject( hDC, hBrushBlack );		// ����
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
    SelectObject( hDC, hBrushWhite );		// ����
   	yy =  225;
	Chord( hDC,  xx-n,yy-n, xx+n,yy+n, xx-n,yy-n,xx-n,yy-n );	// Ellipse �͓��삪���������i�͂ݏo��j
// 	Ellipse( hDC, xx-n,yy-n, xx+n,yy+n);


	/* ���̑��̉�ʏ�� */
	hFont = CreateFont( 16,0, 0, 0, FW_NORMAL, 0, 0, 0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT, FontName);
	hFontOld = (HFONT)SelectObject( hDC, hFont );
	SetTextColor( hDC, rgbText );	// �����F
#ifdef CGF_E
	n = 3;
#else
	n = 0;
#endif
	sprintf( sNum,info_str[0]);
	TextOut( hDC, xx-8+n,128, sNum ,strlen(sNum) );
	sprintf( sNum,"%s",sPlayerName[0]);
	TextOut( hDC, xx+20,128, sNum ,strlen(sNum) );

	SetTextColor( hDC, rgbBlack );	// ����������
	sprintf( sNum,info_str[1]);
	TextOut( hDC, xx-8+n,218, sNum ,strlen(sNum) );
	SetTextColor( hDC, rgbWhite );	// �����𔒂�
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
			// �������[���ŃJ�E���g���Ă݂�B
			// �A�Q�n�}�͖����B�Տ�Ɏc���Ă���΂̐��ƒn�̍��v�B
			endgame_score_str(sNum,1);
			TextOut( hDC, xx-16,450, sNum ,strlen(sNum) );
		}
	}

	// �v�l���Ԃ̕\��
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
		if ( tesuu > 0 && kifu[1][1]==2 ) flag = 1 - i;	// ������ł��Ă���ꍇ�͕\�����t�ɁB
		TextOut( hDC, xx-16,174+90*flag, sNum ,strlen(sNum) );
	}


	SelectObject( hDC, hFontOld );
	DeleteObject(hFont);

	DeleteObject(hBrushWhite);
	DeleteObject(hBrushBlack);
}

// �I�Ǐ�����������ŁA�n�̐��݂̂𐔂���B(���n �| ���n)
int count_endgame_score(void)
{
	int x,y,z,col,n,sum;
	
	sum = hama[0] - hama[1];
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + (x+1);
		col = ban[z];
		n   = endgame_board[z];
		if ( col == 0 ) {	// ��_
			if ( n == GTP_BLACK_TERRITORY ) sum++;	// ���n
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

// �������[���Ōv�Z�i�A�Q�n�}�͖������A���Ώ�����̎����̒n�Ǝ����̐΁A�̍��v�𐔂���B�_���ɑłꍇ��1�ڂƂȂ�j
int count_endgame_score_chinese(void)
{
	int x,y,z,col,n,sum;

	sum = 0;
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = ((y+1)<<8) + (x+1);
		col = ban[z];
		n   = endgame_board[z];
		if ( col == 0 ) {	// ��_
			if ( n == GTP_BLACK_TERRITORY ) sum++;	// ���n
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

// �n�𐔂��Č��ʂ𕶎���ŕԂ�
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

// ����ʂɏ����Ĉ�C�ɓ]������BBeginPaint�͂������ōs���B
void PaintBackScreen( HWND hWnd )
{
	PAINTSTRUCT ps;
	HDC hDC;
	HDC memDC;
	HBITMAP bmp,old_bmp;
	int maxX,maxY;
	RECT rc;

	hDC = BeginPaint(hWnd, &ps);

	// ���z�ő��ʃT�C�Y���擾(Desktop�̃T�C�Y�jSM_CXSCREEN ���ƃ}���`�f�B�X�v���C�ɖ��Ή��B
	GetClientRect(hWnd,&rc);
	maxX = rc.right;
	maxY = rc.bottom;
//	maxX = GetSystemMetrics(SM_CXVIRTUALSCREEN);	// �R���p�C�����ʂ�Ȃ��BVC6�ł͂��߁BVC7(.net)�Ȃ�OK�B
//	maxY = GetSystemMetrics(SM_CYVIRTUALSCREEN);
//	PRT("maxX,Y=(%d,%d)\n",maxX,maxY);

	// ���z��ʂ̃r�b�g�}�b�v���쐬
	memDC   = CreateCompatibleDC(hDC);
	bmp     = CreateCompatibleBitmap(hDC,maxX,maxY);
	old_bmp = (HBITMAP)SelectObject(memDC, bmp);

	PaintScreen( hWnd, memDC );		// ���ۂɕ`����s���֐�

	BitBlt(hDC,0,0,maxX,maxY,memDC,0,0, SRCCOPY);	// ����ʂ���C�ɕ`��

	// ��n��
	SelectObject(memDC,old_bmp);	// �֘A�t����ꂽ�I�u�W�F�N�g�����ɖ߂�
	DeleteObject(bmp);				// HDC�Ɋ֘A�t����ꂽGDI�I�u�W�F�N�g�͍폜�ł��Ȃ�����
	DeleteDC(memDC);

	EndPaint(hWnd, &ps);
}


/*** �}�E�X�̍��N���b�N�̎��̈ʒu��xy���W�ɕϊ����ĕԂ� ***/
/*** �ǂ����w���ĂȂ��Ƃ��͂O��Ԃ�                     ***/
int ChangeXY(LONG lParam)
{
	int dx,sx,sy,db;
	int x,y;
	
	GetScreenParam( hWndDraw,&dx, &sx, &sy, &db );
	x = LOWORD(lParam);
	y = HIWORD(lParam);
	x -= sx + db - dx/2;	// �Ղ̍����i�̂���ɐ΂P���̍����j�ɕ��s�ړ�
	y -= sy + db - dx/2;
	if ( x < 0	|| x > dx*ban_size ) return 0;
	if ( y < 0  || y > dx*ban_size ) return 0;
	x = x / dx;
	y = y / dx;
	return ( ((y+1)<<8) + x+1);
}

// �C�ӂ̎萔�̋ǖʂɔՖʂ��ړ�������
// �O�ɐi�ޏꍇ�͊ȒP�A�߂�ꍇ�͏��肩���蒼���B
void move_tesuu(int n)
{
	int i,z,col,loop;

//	PRT("���݂̎萔=%d, �S�萔=%d, ��]�̎萔=%d ...in \n",tesuu,all_tesuu,n);

	if ( n == tesuu ) return;
	if ( endgame_board_type ) {
		endgame_board_type = 0;
		UpdateLifeDethButton(0);
		PaintUpdate();
	}

	if ( n < tesuu ) {	// ���߂��B
		if ( n <= 0 ) n = 0;
		loop = n;
		// �Ֆʂ������Ֆʂ�
		for (i=0;i<BANMAX;i++) ban[i] = ban_start[i];
		reconstruct_ban();
	} else {			// ���i�߂�
		loop = n - tesuu;
		if ( loop + tesuu >= all_tesuu ) loop = all_tesuu - tesuu;	// �ŏI�ǖʂցB
	}

//	PRT("loop=%d,�萔=%d\n",loop,tesuu);
	for (i=0;i<loop;i++) {
		tesuu++;
		z   = kifu[tesuu][0];	// �ʒu
		col = kifu[tesuu][1];	// �΂̐F
//		PRT("���i�߂� z=%x, col=%d\n",z,col);
		if ( make_move(z,col) != MOVE_SUCCESS )	{
			PRT("�����Č����ɃG���[! tesuu=%d, z=%x,col=%d\n",tesuu,z,col);
			break;
		}
	}
//	PRT("���݂̎萔=%d, �S�萔=%d ..end\n",tesuu, all_tesuu);
}

// ���݂̎�Ԃ̐F��Ԃ��i���ݎ萔�Ə��肪���Ԃ��ǂ����Ŕ��f����j
int GetTurnCol(void)
{
	return 1 + ((tesuu+(nHandicap!=0)) & 1);	// �΂̐F
}

// ���̎�Ԃ��H
int IsBlackTurn(void)
{
	if ( GetTurnCol() == 1 ) return 1;
	return 0;
}

// Gnugo�Ƒ΋ǒ����H
int IsGnugoBoard(void)
{
	int i;
	for (i=0;i<2;i++) {
		if ( turn_player[i] == PLAYER_GNUGO ) return 1;
	}
	return 0;
}


// ��ʂ̑S��������
void PaintUpdate(void)
{
	InvalidateRect(hWndDraw, NULL, FALSE);
	UpdateWindow(hWndDraw);
	return;
}

// �ՖʐF�̃��j���[�Ƀ`�F�b�N��
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

// ���j���[�ƃc�[���o�[�̃{�^����W�F�\����
void MenuEnableID(int id, int on)
{
	HMENU hMenu = GetMenu(ghWindow);

	EnableMenuItem(hMenu, id, MF_BYCOMMAND | (on ? MF_ENABLED : MF_GRAYED) );
	SendMessage(hWndToolBar, TB_SETSTATE, id, MAKELPARAM(on ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE, 0));
}

// �΋ǒ��͊����ړ����j���[���֎~����
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

// �����\�����͑S�Ẵ��j���[���֎~����
void UpdateLifeDethButton(int check_on)
{
	MenuEnablePlayMode(check_on);
	UINT iFlags = check_on ? (TBSTATE_PRESSED | TBSTATE_ENABLED) : TBSTATE_ENABLED;
	SendMessage(hWndToolBar, TB_SETSTATE, IDM_DEADSTONE, MAKELPARAM(iFlags, 0));
}


// ToolBar�̐ݒ�
#define NUMIMAGES       17	// �{�^���̐�
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

	InitCommonControls();	// �R�����R���g���[����dll��������
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

// �c�[���o�[�{�^���̏�Ԃ�ύX
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


// WinMain()�̃��b�Z�[�W���[�v�ŏ�������Ɏg��
int IsDlgDoneMsg(MSG msg)
{
	if ( hAccel != NULL && TranslateAccelerator(hWndMain, hAccel, &msg)	!= 0 ) return 0;

//	if ( IsDlgMsg(hDlgPntFgr,      msg) == 0 ) return 0;
	return 1;
}

// ���W�X�g���̏���
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

// ���W�X�g��������i��ʃT�C�Y�ƈʒu�j��ǂݍ���	 flag == 1 ... main window, 2... print window
BOOL LoadMainWindowPlacement (HWND hWnd,int flag)	
{
	WINDOWPLACEMENT   WindowPlacement; 
	TCHAR             szWindowPlacement [TMP_BUF_LEN] ;
	HKEY              hKeyNames;
	DWORD             Size,Type,Status;

	if ( UseDefaultScreenSize ) return TRUE;	// �Œ�̉�ʃT�C�Y���g��

	Status = RegOpenKeyEx(HKEY_CURRENT_USER, CgfNamesKey, 0L, KEY_READ, &hKeyNames) ;
	if (Status == ERROR_SUCCESS) {
		// get the window placement data
		Size = sizeof(szWindowPlacement);

		// Main Window �̈ʒu�𕜌�
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
	PRT("���W�X�g���̃I�[�v���Ɏ��s�I\n");
	ShowWindow (hWnd, SW_SHOW) ;
	return (FALSE) ;
}

// ���W�X�g���ɏ��i��ʃT�C�Y�ƈʒu�j���������� flag == 1 ... main window, 2... print window
BOOL SaveMainWindowPlacement(HWND hWnd,int flag)
{
	WINDOWPLACEMENT   WindowPlacement;
	TCHAR             ObjectType [2] ;
    TCHAR             szWindowPlacement [TMP_BUF_LEN] ;
    HKEY              hKeyNames = 0 ;
    DWORD             Size ;
    DWORD             Status ;
    DWORD             dwDisposition ;
 
	if ( hWnd == NULL ) return FALSE;	// ���\����ʂ���Ă���
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

// ���W�X�g���ɏ��i���ʉ��A��ʂ̐F�j����������
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

	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,FileListName);	// Yss�̉���File�p�̊i�[�ꏊ�����
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
//	DWORD size = sizeof(str);	// ���ꂾ�ƌ����_�̕�����̒�����Ԃ��Ă��܂��BNULL����0�BERROR_MORE_DATA �ɂȂ�B
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
	if (Status == ERROR_SUCCESS) {	// ���������擾
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


// ���W�X�g���������ǂݍ��� ---> ���Window�̓��C�����E���O�ɏ������̂Ŕ��肪����E�E�E�B
BOOL LoadRegCgfInfo(void)	
{
	HKEY    hKeyNames;
	DWORD   Status;
	TCHAR   SubNamesKey[TMP_BUF_LEN] ;

	sprintf(SubNamesKey,"%s\\%s",CgfNamesKey,FileListName);	// Yss�̉���File�p�̊i�[�ꏊ��
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

		if ( UseDefaultScreenSize ) fInfoWindow = 1;	// ��Ƀf�o�O��ʂ͕\��
		RegCloseKey (hKeyNames);
		return (TRUE);
	}

	// open registry failed, use input from startup info or Max as default
	PRT("���W�X�g���̃I�[�v��(�ׂ������)�Ɏ��s�I\n");
	return (FALSE) ;
}

// ���W�X�g������CPU�̏���ǂݍ���
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
		if (Status != ERROR_SUCCESS) PRT("CPU�擾���s\n");
	}
	PRT("%s\n",sYourCPU);
}



// ������z��̌�⃊�X�g���X�V�B��ԍŋߑI�񂾂��̂�擪�ցB
void UpdateStrList(char *sAdd, int *pListNum, const int nListMax, char sList[][TMP_BUF_LEN] )
{
	int i,j,nLen;
	char sTmp[TMP_BUF_LEN];

	nLen = strlen(sAdd);
	if ( nLen >= TMP_BUF_LEN ) return;
	for (i=0;i<(*pListNum);i++) {
		if ( strcmp(sList[i], sAdd) == 0 ) break;	// ���Ƀ��X�g�ɂ���t�@�C�����J����
	}
	if ( i == (*pListNum) ) {	// �V�K�ɒǉ��B�Â����̂��폜�B
		for (i=nListMax-1;i>0;i--) strcpy(sList[i],sList[i-1]);	// �X�N���[��
		strcpy(sList[0],sAdd);
		if ( (*pListNum) < nListMax ) (*pListNum)++;
	} else {					// ���ɊJ�����t�@�C����擪�ɁB
		strcpy(sTmp,    sList[i]);
		for (j=i;j>0;j--) strcpy(sList[j],sList[j-1]);	// �X�N���[��
		strcpy(sList[0],sTmp    );
	}
}

// �t�@�C�����X�g���X�V�B��ԍŋߊJ�������̂�擪�ցB
void UpdateRecentFile(char *sAdd)
{
	UpdateStrList(sAdd, &nRecentFile, RECENT_FILE_MAX, sRecentFile );
}

// ���j���[��ύX����
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

// �t�@�C�����X�g����1�����폜�B�t�@�C�����������̂ŁB
void DeleteRecentFile(int n)
{
	int i;

	// �V�K�ɒǉ��B�Â����̂��폜�B
	for (i=n;i<nRecentFile-1;i++) strcpy(sRecentFile[i],sRecentFile[i+1]);	// �X�N���[��
	nRecentFile--;
	if ( nRecentFile < 0 ) { PRT("DeleteRecent Err!\n"); debug(); }
}



HMODULE hLib = NULL;	// �A�}�]���v�l�p��DLL�̃n���h��

#define CGFTHINK_DLL "cgfthink.dll"

// DLL������������B���������ꍇ��0���B�G���[�̏ꍇ�͂���ȊO��Ԃ�
int InitCgfGuiDLL(void)
{
	hLib = LoadLibrary(CGFTHINK_DLL);
	if ( hLib == NULL ) return 1;

	// �g���֐��̃A�h���X���擾����B
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

// DLL���������B
void FreeCgfGuiDLL(void)
{
	if ( hLib == NULL ) return;
	cgfgui_thinking_close();
	FreeLibrary(hLib);
}



#define BOARD_SIZE_MAX ((19+2)*256)		// 19�H�Ղ��ő�T�C�Y�Ƃ���

// ����ʒu��Ԃ�
int ThinkingDLL(int endgame_type)
{
	// DLL�ɓn���ϐ�
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

	// ���݂̊������Z�b�g����B
	for (i=0;i<BOARD_SIZE_MAX;i++) {
		dll_init_board[i]    = 3;	// �g�Ŗ��߂�
		dll_endgame_board[i] = 3;
	}
	for (y=0;y<ban_size;y++) for (x=0;x<ban_size;x++) {
		z = (y+1) * 256 + (x+1);
		dll_init_board[z]    = ban_start[z];
		dll_endgame_board[z] = 0;
	}
	for (i=0;i<tesuu;i++) {
		dll_kifu[i][0] = kifu[i+1][0];	// CgfGui������kifu[0]�͖��g�p�Ȃ̂ł��炷�B
		dll_kifu[i][1] = kifu[i+1][1];
		dll_kifu[i][2] = kifu[i+1][2];
	}
	dll_tesuu        = tesuu;
	dll_black_turn   = (GetTurnCol() == 1);
	dll_board_size   = ban_size;
	dll_komi         = komi;
	dll_endgame_type = endgame_type;

	// DLL�̊֐����Ăяo���B
	ct1 = clock();
	thinking_utikiri_flag = 0;	// ���f�{�^���������ꂽ�ꍇ��1�ɁB
	fPassWindows = 1;			// DLL���Ăяo�����͒��f�{�^���ȊO���g�p�s�\��
	MenuEnableID(IDM_DLL_BREAK, 1);
	ret_z = cgfgui_thinking(dll_init_board, dll_kifu, dll_tesuu, dll_black_turn, dll_board_size, dll_komi, dll_endgame_type, dll_endgame_board);
	fPassWindows = 0;
	MenuEnableID(IDM_DLL_BREAK, 0);
	PRT("�ʒu=(%2d,%2d),����=%2.1f(�b),�萔=%d,�I��=%d\n",ret_z&0xff,ret_z>>8,GetSpendTime(ct1),dll_tesuu,endgame_type);
	PRT("----------------------------------------------\n");

	// �I�ǁA�}�`�A���l�\���p�̔Ֆʂɒl���Z�b�g
	if ( endgame_type != ENDGAME_MOVE ) {
		for (i=0;i<BOARD_SIZE_MAX;i++) endgame_board[i] = dll_endgame_board[i];
	}

	return ret_z;
}

/*
void TestDllThink(void)
{
	static int fFirst = 1;	// �ŏ���1�x����DLL��������
	if ( fFirst ) {	
		if ( InitCgfGuiDLL() == 0 ) return;	// DLL������������B
		cgfgui_thinking_init(&thinking_utikiri_flag);
		fFirst = 0;
	}
	ThinkingTestDLL();
//	FreeCgfGuiDLL();	// DLL���������B
}
*/

