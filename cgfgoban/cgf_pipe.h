// cgf_pipe.h

// cgf_gtp.cpp
void SendGTP(char *str);
int ReadGTP(void);	// �p�C�v����GnuGo�̏o�͂�ǂݏo��
int ReadCommmand(int *z);	// �R�}���h����M����B��̎�M�����˂�BError�̏ꍇ��0��Ԃ��B

int open_gnugo_pipe(void);	// �p�C�v���쐬
void close_gnugo_pipe(void);
int PlayGnugo(int endgame_type);
void UpdateGnugoBoard(int z);	// Gnugo���̔Ֆʂ�������
void PutStoneGnuGo(int z, int black_turn);
int init_gnugo_gtp(char *sName);
void FinalStatusGTP(int *p_endgame);	// GTP Engine�Ŏ�������Ŏ����\�����s��

//void test_gnugo_loop(void);

extern int fGnugoPipe;
extern char sGtpPath[][TMP_BUF_LEN];	// GTP�G���W���̏ꏊ
//extern char sGnugoDir[];

