/* cgf_wsk.cpp */
// WinSock���g�����ʐM�ΐ�p�Bwsock32.lib �������N����K�v����
#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
//#include <winsock.h>

#include "cgf.h"
#include "cgf_wsk.h"


// nngs�ɐڑ�����e�X�g

char nngs_player_name[2][TMP_BUF_LEN] = {
//	"aya",
//	"katsunari",
	"test1",
	"test2",
//	"brown",
};

SOCKET Sock = INVALID_SOCKET;

char *stone_str[2] = { "B","W" };

#define NNGS_LOGIN_TOTYU -3	// login�����̓r��
#define NNGS_LOGIN_END   -2	// login�������I������
#define NNGS_GAME_END    -1	// �΋ǏI��
#define NNGS_UNKNOWN     -4	// ���Ӗ��ȃf�[�^�B��������B

int nngs_minutes = 40;		// 40���؂ꕉ��
//int already_log_in = -1;

// 0...PASS, 1�ȏ�...��B-1�ȉ�...�G���[�⃍�O�C������
// nngs�̓��e�����
//int fTurn;	// ���Ԃ̎� 0�A���Ԃ̎� 1
int nngsAnalysis(char *str, int fTurn)
{
	char tmp[TMP_BUF_LEN];

	// AI������nngs
	if ( fAIRyusei ) {
		if ( strstr(str,"Login:") ) {	// ���s�R�[�h��1�s�𔻒f���Ă�̂Ŏ��s����
			sprintf(tmp,"%s\n",nngs_player_name[fTurn]);	// ���[�U���Ń��O�C��
			NngsSend(tmp);	
			return NNGS_LOGIN_TOTYU;
		}
		
		if ( strstr(str,"No Name Go Server (NNGS) version") ) {
			NngsSend("set client TRUE\n");	// �V���v���ȒʐM���[�h��
			return NNGS_LOGIN_TOTYU;
		}

		if ( strstr(str,"Set client to be True") ) {
			/*if ( fTurn == 0 ) {
				sprintf(tmp,"match %s B %d %d 0\n",nngs_player_name[1-fTurn],ban_size,nngs_minutes);
				NngsSend(tmp);	// ���Ԃ̏ꍇ�ɁA������\������
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
			NngsSend(tmp);	// ���Ԃ̏ꍇ�ɁA�������󂯂�B
			Sleep(1000);
			return NNGS_LOGIN_TOTYU;
		}

		if (/* fTurn == 0 &&*/ strstr(str,"accepted.") ) {	// �����������̂ŏ���𑗂��B
			return NNGS_LOGIN_END;
		}

	} else {	// UEC�t��nngs
/*
		if ( strstr(str,"Login:") ) {	// ���s�R�[�h��1�s�𔻒f���Ă�̂Ŏ��s����
			sprintf(tmp,"%s\n",nngs_player_name[fTurn]);	// ���[�U���Ń��O�C��
			NngsSend(tmp);	
			return NNGS_LOGIN_TOTYU;
		}
*/
		if ( strstr(str,"No Name Go Server (NNGS) version") ) {
			NngsSend("set client TRUE\n");	// �V���v���ȒʐM���[�h��
			return NNGS_LOGIN_TOTYU;
		}
		if ( strstr(str,"Set client to be True") ) {
			if ( fTurn == 0 ) {
				sprintf(tmp,"match %s B %d %d 0\n",nngs_player_name[1-fTurn],ban_size,nngs_minutes);
				NngsSend(tmp);	// ���Ԃ̏ꍇ�ɁA������\������
			}
			return NNGS_LOGIN_TOTYU;
		}	

		sprintf(tmp,"Match [%dx%d]",ban_size,ban_size);	// "Match [19x19]"
		if ( strstr(str,tmp) ) {
			sprintf(tmp,"match %s W %d %d 0\n",nngs_player_name[1-fTurn],ban_size,nngs_minutes);
			NngsSend(tmp);	// ���Ԃ̏ꍇ�ɁA�������󂯂�B
			return NNGS_LOGIN_END;
		}

		if ( fTurn == 0 && strstr(str,"accepted.") ) {	// �����������̂ŏ���𑗂��B
			return NNGS_LOGIN_END;
		}	
	}


	if ( strstr(str,"Illegal") ) return NNGS_GAME_END;	// Error�̏ꍇ

	sprintf(tmp,"(%s): ",stone_str[1-fTurn]);
	char *p = strstr(str,tmp);
	if ( p != NULL ) {
		int x,y;

		p += 5;
		if ( strstr(p,"Pass") ) {
			PRT("���� -> PASS!\n");
			return 0;
		} else {
			x = *p;
			y = atoi(p+1);
			PRT("���� -> %c%d\n",x,y);
			x = x  - 'A' - (x > 'I') + 1;	// 'I'�𒴂����� -1	
			return (ban_size +1 - y)*256 + x;
		}
	}	

	// Pass���A�������ꍇ�ɗ���(PASS������̑��肩���PASS�͗��Ȃ��j
	if ( strstr(str,"You can check your score") ) {
		sprintf(tmp,"done\n");	// �n���v�Z���郂�[�h���I���邽�߂� "done" �𑗂�
								// ���̌�A"resigns." �����񂪗���B����ŃT�[�o�ɐ����Ȋ����Ƃ��ĕۑ������B
		NngsSend(tmp);
		return 0;	// PASS��Ԃ�
	}

	// �I�Ǐ������I������B"9 {Game 1: test2 vs test1 : Black resigns. W 10.5 B 6.0}"
	if ( strstr(str,"9 {Game") && strstr(str,"resigns.") ) {
		sprintf(tmp,"%s vs %s",nngs_player_name[1],nngs_player_name[0]);
		if ( strstr(str,tmp) ) return NNGS_GAME_END;	// game end
	}

	sprintf(tmp,"{%s has disconnected}",nngs_player_name[1-fTurn]);
	if ( strstr(str,tmp) ) return NNGS_GAME_END;	// �ΐ푊�肪�ڑ���؂����B

	// �ǂ��炩�̎��Ԃ��؂ꂽ�ꍇ
	if ( strstr(str,"forfeits on time") &&
	     strstr(str,nngs_player_name[0]) && strstr(str,nngs_player_name[1]) ) {
		return NNGS_GAME_END;
	}
	return NNGS_UNKNOWN;	// ���Ӗ��ȃf�[�^
}

// �f�[�^���M
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

// Login����('\n'��҂����ɂ�������)
void login_nngs(int fTurn)
{
	char tmp[256];
	sprintf(tmp,"%s\n",nngs_player_name[fTurn]);
	NngsSend(tmp);	// �����Ƀ��[�U���Ń��O�C��
}

//char sNNGS_IP[TMP_BUF_LEN] = "nngs.computer-go.jp";	// nngs.computer-go.jp 9696	
//char sNNGS_IP[TMP_BUF_LEN] = "192.168.11.11";

int fUseNngs = 0;	// nngs�o�R�Ŕ񓯊��őΐ킷��ꍇ
extern HWND ghWindow;					/* WINDOWS�̃n���h��  */

// �T�[�o�Ɛڑ�����B���s�����0��Ԃ�
int connect_nngs(void)
{
	WSADATA WsaData;
	SOCKADDR_IN Addr;
	unsigned short nPort = 9696;	// �T�[�o�̃|�[�g�ԍ�
	unsigned long addrIP;			// IP���ϊ�����鐔�l
	PHOSTENT host = NULL;			// �z�X�g���
	int i;
//	char sIP[] = "192.168.0.1";		// GPW�t�ł̃T�[�o��IP
//	char sIP[] = "202.250.144.34";	// nngs.computer-go.jp 9696

	// WinSock�̏�����
	if ( WSAStartup(0x0101, &WsaData) != 0 ) {
		PRT("Error!: WSAStartup(): %d\n",WSAGetLastError());
		return 0;
	}

	// �\�P�b�g�����
	Sock = socket(PF_INET, SOCK_STREAM, 0);
	if ( Sock == INVALID_SOCKET ) {
		PRT("Error: socket(): %d\n", WSAGetLastError());
		return 0;
	}
	
	// IP�A�h���X�𐔒l�ɕϊ�
	addrIP = inet_addr(sNngsIP[0]);
	if ( addrIP == INADDR_NONE ) {
		host = gethostbyname(sNngsIP[0]);		// �z�X�g����������擾����IP�𒲂ׂ�
		if ( host == NULL ) {
			PRT("Error: gethostbyname(): \n");
			return 0;
		}
		addrIP = *((unsigned long *)((host->h_addr_list)[0]));
	} else {
//		host = gethostbyaddr((const char *)&addrIP, 4, AF_INET);	// Local�Ȋ��ł͎��s����
	}

	if ( host ) {	// IP���Ȃǂ�\��
		PRT("������ = %s\n",host->h_name);
		for (i=0; host->h_aliases[i]; i++) PRT("�ʖ� = %s\n",host->h_aliases[i]);
		for (i=0; host->h_addr_list[i]; i++) {
			IN_ADDR *ip;
			ip = (IN_ADDR *)host->h_addr_list[i];
			PRT("IP = %s\n",inet_ntoa(*ip));
		}
	} else PRT("host = NULL,sIP=%s\n",sNngsIP[0]);

	// �񓯊��ŒʐM����ꍇ
	// �ڑ��A���M�A��M�A�ؒf�̃C�x���g���E�B���h�E���b�Z�[�W�Œʒm�����悤��
	if ( fUseNngs && WSAAsyncSelect(Sock, ghWindow, WM_USER_ASYNC_SELECT, FD_CONNECT | FD_WRITE | FD_READ | FD_CLOSE) == SOCKET_ERROR ) {
		PRT("�\�P�b�g�C�x���g�ʒm�ݒ�Ɏ��s\n");
		return 0;
	}

	// �T�[�o�[��IP�A�h���X�ƃ|�[�g�ԍ��̎w��
	memset(&Addr, 0, sizeof(Addr));
	Addr.sin_family      = AF_INET;
	Addr.sin_port        = htons(nPort);
	Addr.sin_addr.s_addr = addrIP;

	// �T�[�o�[�֐ڑ�	 
	if ( connect( Sock, (LPSOCKADDR)&Addr, sizeof(Addr) ) == SOCKET_ERROR ) {
		if ( WSAGetLastError() != WSAEWOULDBLOCK ) {
			PRT("Error: connect(): \n");
			return 0;
		}
	}

//	close_nngs();	// �ڑ��̐ؒf
	return 1;
}

// �ڑ��̐ؒf
void close_nngs(void)
{
	if ( Sock != INVALID_SOCKET ) closesocket(Sock);
	Sock = INVALID_SOCKET;
	WSACleanup();
}



HWND hNngsWaitDlg = NULL;
#define IDM_NNGS_MES 1000

// �񓯊���Select���󂯎��Bnngs�p
void OnNngsSelect(HWND /*hWnd*/, WPARAM wParam, LPARAM lParam)
{
	static char buf[TMP_BUF_LEN];
	static int buf_size = 0;

	if ( WSAGETASYNCERROR(lParam) != 0 ) {
		PRT("�\�P�b�g�C�x���g�̒ʒm�ŃG���[=%d\n",WSAGETSELECTERROR(lParam));
//		WSAECONNABORTED  (10053)
		closesocket(Sock);		// �\�P�b�g��j��
		Sock = INVALID_SOCKET;
		return;
	}
	if ( Sock != (SOCKET)wParam) return;	// �������ׂ��\�P�b�g������

	switch (WSAGETSELECTEVENT(lParam)) {
		case FD_CONNECT:	// �ڑ��ɐ���	
			PRT("�T�[�o�ɐڑ�\n");
			buf_size = 0;
			break;

		case FD_CLOSE:		// �ڑ���؂�ꂽ
			closesocket(Sock);
			Sock = INVALID_SOCKET;
			PRT("�T�[�o����ؒf���ꂽ\n");
			break;

		case FD_WRITE:		// ���M�o�b�t�@�ɋ󂫂��ł���
			PRT("FD_WRITE\n");
			break;

		case FD_READ:		// �f�[�^����M����
//			PRT("FD_READ size=%d\n",buf_size);
			int nRecv = recv( Sock, buf + buf_size, 1, 0);
			if ( nRecv == SOCKET_ERROR ) {		// �����u���b�N���������H�Ȃ̂ŉ������Ȃ�
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
				login_nngs(dll_col-1);	// login����
				break;
			}

			if ( buf[buf_size-1] == '\n' ) {	// 1�s�󂯎����
				buf_size = 0;
				PRT("<---%s",buf);
				if ( fPassWindows ) { PRT("�v�l����nngs����̏��͖���\n"); break; }
				// �\����̓��[�`���֔��
				int ret = nngsAnalysis(buf,dll_col-1);	// nngs�̓��e�����
//				PRT("ret=%d\n",ret);
				if ( ret == NNGS_LOGIN_TOTYU ) break;
				if ( ret == NNGS_UNKNOWN     ) break;
				RetZ = ret;
				if ( hNngsWaitDlg == NULL ) {
					PRT("nngs�ҋ@�_�C�A���O���o�Ă��Ȃ�\n");
					break;
				}
				PostMessage(hNngsWaitDlg,WM_COMMAND,IDM_NNGS_MES,0);	// �ʐM�҂��_�C�A���O���I��������B
			}
			break;
	}
}


// nngs����̏���҂�
BOOL CALLBACK NngsWaitProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			hNngsWaitDlg = hDlg;
			if ( lParam == 0 ) {	// �ŏ��͐ڑ����ă��O�C������
				if ( connect_nngs() == 0 ) EndDialog(hDlg,FALSE);	// ���O�C�����s
			}
			return TRUE;

		case WM_COMMAND:
			switch ( LOWORD(wParam) ) {
				case IDCANCEL:
					EndDialog(hDlg,FALSE);			// ���f�ŏI��
					break;
				case IDM_NNGS_MES:
					EndDialog(hDlg,TRUE);			// ���炩�̃��b�Z�[�W�������̂ŏI��
					break;
			}
			return TRUE;

		case WM_DESTROY:
			hNngsWaitDlg = NULL;
			break;
	}
	return FALSE;
}
