/* cgf_wsk.h */
//#define WM_USER_GETSERV (WM_USER+1)
//#define WM_USER_ASYNC_GETHOST (WM_USER+2)	// �z�X�g�����擾�����Ƃ��ɒʒm
#define WM_USER_ASYNC_SELECT  (WM_USER+3)	// �\�P�b�g�C�x���g��ʒm

// nngs
extern int fUseNngs;		// nngs�o�R�őΐ킷��ꍇ
extern int fPassWindows;	// Windows�ɏ�����n���Ă�B�\���ȊO��S�ăJ�b�g

//extern char sNNGS_IP[TMP_BUF_LEN];
extern char sNngsIP[][TMP_BUF_LEN];	// nngs�̃T�[�o��
extern char nngs_player_name[2][TMP_BUF_LEN];
extern int nngs_minutes;		// 40���؂ꕉ��
extern int fAIRyusei;

// �֐��̃v���g�^�C�v�錾
void OnNngsSelect(HWND hWnd, WPARAM wParam, LPARAM lParam);	// �񓯊���Select���󂯎��Bnngs�p
BOOL CALLBACK NngsWaitProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);		// nngs����̏���҂�
void close_nngs(void);	// �ڑ��̐ؒf

void change_z_str(char *str, int z);
void NngsSend(char *ptr);

