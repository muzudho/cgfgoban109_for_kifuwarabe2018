/* cgf_term.cpp */
/*
  ���̃\�[�X�́A�͌�̒ʐM�v���g�R�� SGMP(Standard Go Modem Protocol
- Revision 1.0) �p�ł��B�ʐM�|�[�g�̃I�[�v���A�N���[�Y�A�ʐM�f�[�^��
��M�A���M�Ȃǂ��s���Ă��܂��B

  ����� VisualC++6.0 �ŏ�����Ă��܂��BMFC �͂��������g���Ă��܂���B
SDK�݂̂ł��B�܂��X���b�h�œ��삵�Ă��܂���B������̎��ԁiWM_TIMER�j
�ŒʐM�o�b�t�@���̂����ɍs���`���Ƃ��Ă��܂��B

  FOST�t�ł͒ʐM�͕K�{�ł͂���܂��񂪁A����͂��ƁA�������Ԃ̃n���f��
���܂��B����쐬�������́A�Q�l�ɂ��Ă���������΍K���ł��B

�E���ӁI
�����Ă����V�[�P���X�r�b�g�����������ǂ����̔���͂��Ă��܂���B

���̃\�[�X�Ɋւ��ẮA���i�R���G�j�͒��쌠��������܂��B
�Ĕz�z�A�]�ځA�R�s�[�A�����͎��R�ɍs���č\���܂���B

2005/07/14 �R�� �G
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

//#define CGFCOM_9x9	// CGFCOM.DLL��9�H�őΐ킷��ꍇ�͒�`����BCGFCOM.DLL�̃o�O�΍�

#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "cgf.h"
#include "cgf_wsk.h"

#define TIMER_ID 1		/* �ʐM��M�p�^�C�}�[ID    */
#define INTERVAL 20		/* 20ms����TIMER���Ă΂�� */

BOOL NEAR OpenConnection( void );
BOOL NEAR CloseConnection( void );
BOOL CheckCommBuff(void);
BOOL CALLBACK CommProc( HWND ,UINT, WPARAM, LPARAM);
BOOL AnalyzeStr(void);
void ReturnOK(void);
void AnswerQuery(void);
void ReadMove(void);
void Delay(DWORD dWait);	// ���ԑ҂��̊֐�

HANDLE hCommDev = INVALID_HANDLE_VALUE;
BOOL fCommConnect = 0;	// �ڑ���Ԃ�����

static int nLen = 0;	// �O�`�RByte�B��M�����o�C�g��
static BYTE StartByte;
static BYTE CheckSum;
static BYTE CommHI;		// �R�}���h�̏�ʃo�C�g
static BYTE CommLO;		//           ���ʃo�C�g
static BOOL BitH = 0;	// �����̃V�[�P���X�r�b�g
static BOOL BitY = 0;	// ����̃V�[�P���X�r�b�g
static BOOL BitHH,BitYY;// �V�����V�[�P���X�r�b�g
static BOOL WaitOK = 0;	// �ŏ��̓j���[�g�������

static BYTE lpRead[4+1];	// ��M�p������
static BYTE lpSend[4+1];	// ���M�p������

static DWORD RetByte;	// �P�Ȃ�_�~�[�̕ϐ�

extern HWND ghWindow;					/* WINDOWS�̃n���h��  */
extern HINSTANCE ghInstance;			/* �C���X�^���X       */

static BOOL fEndDlg;	// �_�C�A���O���I������t���O




BOOL NEAR OpenConnection( void )
{            
	DCB dcb;	// �ʐM���\����
    COMMTIMEOUTS CommTimeOuts;	// �ʐM�^�C���A�E�g�̍\����
	char szCommNumber[6];

	BitH = 0;	// �����̃V�[�P���X�r�b�g
	BitY = 0;	// ����̃V�[�P���X�r�b�g

	BitH = BitY = BitHH = BitYY = 0;	// �S�ẴV�[�P���X�r�b�g��������

	// �ʐM�|�[�g�̃I�[�v��
	sprintf( szCommNumber, "COM%d",nCommNumber);
	PRT("%s�ŒʐM�����݂܂��B\n",szCommNumber);
	hCommDev = CreateFile( szCommNumber, GENERIC_READ | GENERIC_WRITE,
				         0,                    // exclusive access
				         NULL,                 // no security attrs
				         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hCommDev == INVALID_HANDLE_VALUE ) return(FALSE);	// �ʐM�|�[�g�I�[�v�����s

	// �ʐM�^�C���A�E�g�̐ݒ�
	CommTimeOuts.ReadIntervalTimeout         = MAXDWORD;
    CommTimeOuts.ReadTotalTimeoutMultiplier  =    0;
    CommTimeOuts.ReadTotalTimeoutConstant    = 1000;
    CommTimeOuts.WriteTotalTimeoutMultiplier =    0;
    CommTimeOuts.WriteTotalTimeoutConstant   = 1000;
    if ( !SetCommTimeouts( hCommDev, &CommTimeOuts ) ) {
        CloseHandle(hCommDev);
        return (FALSE); // SetCommTimeouts error
    }

	GetCommState( hCommDev, &dcb );	// �ʐM���̎擾
	dcb.BaudRate = 2400;			// 2400�{�[�ɐݒ�
	dcb.ByteSize = 8;				// 8�f�[�^�r�b�g
	dcb.Parity = NOPARITY;			// �p���e�B�Ȃ�
	dcb.StopBits = ONESTOPBIT;		// 1�X�g�b�v�r�b�g
	SetCommState( hCommDev, &dcb );	// �ʐM���̐ݒ�
	return ( TRUE ) ;
}

BOOL NEAR CloseConnection( void )
{
	if ( fUseNngs ) { close_nngs();	return TRUE; }
	return ( CloseHandle( hCommDev ) );
}

// �ʐM�o�b�t�@����1byte�Âǂݍ��ށB
BOOL CheckCommBuff(void )
{
	BOOL	fReadStat;
	COMSTAT	ComStat;
	DWORD	dwError, dwLen;

	ClearCommError( hCommDev, &dwError, &ComStat );	// ��M�f�[�^�����邩�`�F�b�N
	dwLen = ComStat.cbInQue;	// ���̃o�C�g��

	if (dwLen > 0) {
		fReadStat = ReadFile( hCommDev, (LPSTR *)lpRead, 1, &dwLen, NULL ); // 1Byte�����ǂ݂Ƃ�
//		PRT("data=%02x: nLen=%d, StartByte=%x, Sum=%x, ComH=%x, ComL=%x\n",lpRead[0], nLen,StartByte, CheckSum,CommHI,CommLO);
		if ( fReadStat == FALSE ) PRT("��M�����ɂăG���[���������܂���\n");
		else return (TRUE);
	}
	return (FALSE);
}


// ��M�҂�������_�C�A���O�{�b�N�X�֐�
// �_�C�A���O���I�����鎞�́A�肪�Ԃ��Ă����ꍇ�ƁA�G���[�A�L�����Z���A�̏ꍇ�B
BOOL CALLBACK CommProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/ )
{
	switch(msg) {
		case WM_INITDIALOG:
			fEndDlg = FALSE;
			RetComm = 0;	// �G���[�ŏI�������ꍇ
			SetTimer(hDlg, TIMER_ID, INTERVAL, NULL);		// �^�C�}�[���Z�b�g
			return TRUE;

		case WM_COMMAND:
			if ( wParam == IDCANCEL ) {
				KillTimer(hDlg, TIMER_ID);	// �^�C�}�[��~
				EndDialog(hDlg,FALSE);			// ���f�ŏI��
			}
			return TRUE;
/*
		// WinSock���g���ĒʐM����ꍇ
		case WM_USER_ASYNC_SELECT:
			OnServerAsyncSelect(hDlg, wParam, lParam);	// �񓯊���Select���󂯎��
			return TRUE;
*/
		case WM_TIMER:
			if ( CheckCommBuff() ) {
				if ( AnalyzeStr() == FALSE ) {
					PRT("��M�������͂ŃG���[���������܂����B\n");
					KillTimer(hDlg, TIMER_ID);	// �^�C�}�[��~
					EndDialog(hDlg,FALSE);		// �G���[�ŏI��
				}
			}
			if ( fEndDlg ) {
				KillTimer(hDlg, TIMER_ID);	// �^�C�}�[��~
				EndDialog(hDlg,TRUE);		// ����I��
			}
			return TRUE;
	}
	return FALSE;
}

// ��M������̉��
BOOL AnalyzeStr(void)
{
	BYTE c;

	c = lpRead[0];

	if ( (c & 0xfc) == 0 ) {	// 0000 00hy �X�^�[�g�r�b�g�������ꍇ
		nLen = 1;
		BitHH = ( c & 0x02) >> 1;
		BitYY = ( c & 0x01);
		StartByte = c;
		return(TRUE);
	}

	if ( (c & 0x80) == 0 ) {	// TEXT������Ǝv����
		PRT("Text=%x,%c\n",c,c);	// debug�p�̕\���֐�
		return(TRUE);
	}
	
	if ( nLen == 0 ) return(TRUE);	// �s�v�ȕ���

	if ( nLen == 1 ) {	// �Q�o�C�g�ڂ̓`�F�b�N�T��
//		CheckSum = c & 0x7f;
		CheckSum = c;
		nLen = 2;
		return(TRUE);
	}

	if ( nLen == 2 ) {	// �R�o�C�g�ڂ̓R�}���h�̏�ʃo�C�g
		CommHI = c;
		nLen = 3;
		return(TRUE);
	}

	if ( nLen == 3 ) {	// �S�o�C�g��
		nLen = 0;
		CommLO = c;							
		c = (StartByte + CommHI + CommLO) & 0x7f;
		if ( c != (CheckSum & 0x7f) ) {
			PRT("�`�F�b�N�T���G���[ CheckSum=%x, Sum=%x\n",CheckSum,c);
			return(FALSE);	// �`�F�b�N�T���G���[
		}

		PRT("Read Data    <- %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);

		// �V�[�P���X�r�b�g�̃`�F�b�N������H
		// BitH ,BitY  ���݂̃V�[�P���X�r�b�g
		// BitHH,BitYY ��M�����V�[�P���X�r�b�g
		
		if ( CommHI == 0x87 && CommLO == 0xff ) {					// OK!
			fEndDlg = TRUE;		// OK ���Ԃ��Ă�����DAILOG�𔲂���
			RetComm = 2;		// OK ������
			PRT("OK����M�B\n");
		}
		else if ( CommHI == 0x90 && CommLO == 0x80 ) ReturnOK();	// DENY
		else if ( CommHI == 0xa0 && CommLO == 0x80 ) {				// NEWGAME
			fEndDlg = TRUE;		// DAILOG�𔲂���
			RetComm = 3;		// NEWGAME ������!
			PRT("NEWGAME�������I\n");
		}
		else if ( (CommHI & 0xf0) == 0xb0 ) AnswerQuery();			// QUERY // �S�ĂO�œ����� 
		else if ( (CommHI & 0xf0) == 0xc0 ) {						// ANSWER
			fEndDlg = TRUE;		// DAILOG�𔲂���
			RetComm = 4;		// ANSWER���A���Ă���
			PRT("ANSWER���߂��Ă����B\n");
		}
		else if ( (CommHI & 0xf0) == 0xd0 ) ReadMove();				// Move
		else if ( (CommHI & 0xf0) == 0xe0 ) return(FALSE);			// Back Move
		else if ( (CommHI & 0xf0) == 0xf0 ) return(FALSE);			// �g���R�}���h�͎g��Ȃ�
		return(TRUE);
	}
	PRT("4�o�C�g�ڂ𒴂���\n");
	return(FALSE);
}

// 4�o�C�g�𑗐M
void Send4Byte(unsigned char ptr[])
{
	Delay(100);	// ���������҂B
	WriteFile( hCommDev, (LPSTR *)ptr, 4, &RetByte, NULL );
}

// OK ��Ԃ�
void ReturnOK(void)
{

//	StartByte = (BitHH << 1) + BitYY;
	StartByte = (BitYY << 1) + BitHH;	// �����Ƒ���̃r�b�g������
	CommHI = 0x87;
	CommLO = 0xff;
	CheckSum = StartByte + ( CommHI & 0x7f ) + ( CommLO & 0x7f );
	CheckSum |= 0x80;

	lpSend[0] = StartByte;
	lpSend[1] = CheckSum;
	lpSend[2] = CommHI;
	lpSend[3] = CommLO;
	Delay(100);						/* ��莞��(100ms)�҂� */
	PRT("Send Data    -> %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);
	PRT("ReturnOK()\n");
	Send4Byte(lpSend);
//	WriteFile( hCommDev,(void *)lpSend, 4, &RetByte, NULL );
}

// 
void AnswerQuery(void)
{
	PRT("Query(%d)����M\n",CommLO & 0x7f);

	BitH = ( BitH == 0 );	// �_�����Z	0 -> 1, 1 -> 0
	StartByte = (BitYY << 1) + BitH;

	if ( CommHI == 0xb0 && CommLO == 0x8b ) {	// ���Ȃ��͉��F�H
		CommHI = 0xc0;
		CommLO = 0x80 + UN_COL(dll_col);	// DLL�����̎��Q
	} else if ( CommHI == 0xb0 && CommLO == 0x88 ) {	// handicap�́H
		CommHI = 0xc0;
		CommLO = 0x81;	// �݂���Ɍ��܂��Ă邾��x�C�x�[
	} else if ( CommHI == 0xb0 && CommLO == 0x87 ) {	// ���[����
		CommHI = 0xc0;
		CommLO = 0x81;	// ���{���[��
//		CommLO = 0x82;	// Ing or Chinese
	} else if ( CommHI == 0xb0 && CommLO == 0x89 ) {	// �Ղ̃T�C�Y�́H
		CommHI = 0xc0;
//		CommLO = 0x93;	// 19*19
//		CommLO = 0x89;	// 9*9
		CommLO = 0x80 + ban_size;
	} else {
		CommHI = 0xc0;
		CommLO = 0x80;	// ��݂͂�ȓ��R�O�iunknown�j
	}
	CheckSum = StartByte + ( CommHI & 0x7f ) + ( CommLO & 0x7f );
	CheckSum |= 0x80;

	lpSend[0] = StartByte;
	lpSend[1] = CheckSum;
	lpSend[2] = CommHI;
	lpSend[3] = CommLO;
	Delay(500);						/* ��莞��(500ms)�҂� */
	PRT("Send Data    -> %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);
	PRT("Answer(%d)\n",CommLO & 0x7f);
	Send4Byte(lpSend);
//	WriteFile(hCommDev, lpSend, 4, &RetByte, NULL);
}

// ����󂯎��
void ReadMove(void)
{
	int x,y,z;

//	if ( CommHI & 0x04 ) PRT("���΂�u������\n");
//	else PRT("���΂�u������\n");
	z = CommLO & 0x7f;
	z += (CommHI & 0x03) << 7;
	PRT("�ʒu(1-361)=%d ",z);
	if ( z != 0 ) {
		z = z - 1;
#ifdef CGFCOM_9x9	// CGFCOM.DLL�p
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
		// z ���P�X�̔{���̎��o�O��
/*		y = z / 19;
		x = z - y*19;
		y = 19 - y;
		y = y << 8;
		z = y + x;
*/
	}
	PRT("(�����ϊ���=%x)\n",z);
	ReturnOK();
	RetZ = z;
	RetComm = 1;	// �肪�����ꍇ�͂P
	fEndDlg = TRUE;
}

// �� �� �𑗂�
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

	BitH = ( BitH == 0 );	// �_�����Z	0 -> 1, 1 -> 0
	StartByte = (BitYY << 1) + BitH;

	if ( z != 0 ) {
		x = z & 0x00ff;
		y = z >> 8;
#ifdef CGFCOM_9x9	// CGFCOM.DLL�p
		y = 19 - y;
		z = y*19 + x;
#else
		y = ban_size - y;
		z = y*ban_size + x;
#endif
	}
	CommHI = (z & 0x180) >> 7;
	CommHI |= 0xd0;
	if ( dll_col == 2 ) CommHI |= 0x04;	// ���������Ȃ甒�Ԃ�ݒ�
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

// NewGame�𑗂�
int SendNewGame(void)
{
	BitH = BitY = BitHH = BitYY = 0;	// �O�̂��߂����ł��V�[�P���X�r�b�g��������

	BitH = ( BitH == 0 );	// �_�����Z	0 -> 1, 1 -> 0
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
	PRT("NewGame �𑗐M\n");
	Send4Byte(lpSend);
//	WriteFile( hCommDev,(void *)lpSend, 4, &RetByte, NULL );

	// ��M������AOK ��҂�
	if ( DialogBox(ghInstance, "COMM_DIALOG", ghWindow, (DLGPROC)CommProc) == FALSE ) {
		PRT("NewGame�̒��f����\n");
		return(FALSE);
	}
	if ( RetComm != 2 ) {
		PRT("NEWGAME �𑗂����̂� OK ���A���Ă��Ȃ���B\n");
		return(FALSE);
	}
	return( TRUE );
}

// QUERY ����𑗂�B
int SendQuery(int qqq)
{
	BitH = ( BitH == 0 );	// �_�����Z	0 -> 1, 1 -> 0
	StartByte = (BitYY << 1) + BitH;

	CommHI = 0xb0;
	CommLO = 0x80 + qqq;

	CheckSum = StartByte + ( CommHI & 0x7f ) + ( CommLO & 0x7f );
	CheckSum |= 0x80;

	lpSend[0] = StartByte;
	lpSend[1] = CheckSum;
	lpSend[2] = CommHI;
	lpSend[3] = CommLO;
	Delay(500);						/* ��莞��(500ms)�҂� */
	PRT("Send Data    -> %02x:%02x:%02x:%02x :",StartByte,CheckSum,CommHI,CommLO);
	PRT("Send Query = %d \n",qqq);
	Send4Byte(lpSend);
//	WriteFile( hCommDev,(void *)lpSend, 4, &RetByte, NULL );
//	PRT("Send Query End\n");

	// ��M������AANSWER ��҂�
	if ( DialogBox(ghInstance, "COMM_DIALOG", ghWindow, (DLGPROC)CommProc) == FALSE ) {
		PRT("QUERY�̒��f����\n");
		return(FALSE);
	}
	if ( RetComm != 4 ) {
		PRT("QUERY �𑗂����̂� ANSWER ���Ԃ��Ă��Ȃ��E�E�E�B\n");
		return(FALSE);
	}
//	ReturnOK();		// ��肠����ANSWER�ɂ�OK��Ԃ��܂��傤
	return( CommLO & 0x7f );
}

// Delay ����  dWait (ms) �����҂B500 �� 500ms = 0.5�b
void Delay(DWORD dWait)
{
//	DWORD dTicks;
//	MSG msg;

	if ( dWait > 500 ) PRT("Wait...%dms: ",dWait);
	Sleep(dWait);	// Sleep�֐��̂ق����X�}�[�g�H�BCPU���Ԃ͂܂������H��Ȃ��B
/*
	dTicks = GetTickCount();	// �V�X�e�����X�^�[�g���Ă���̃~���b�𓾂�

	// �w�莞�Ԃ��o�߂���܂Ń��b�Z�[�W���[�v	 ---> �{�̂��I�������Ƃ��������Ȃ�
	while (dWait > (GetTickCount() - dTicks)) {	
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
*/
}
