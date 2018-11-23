/*** cgf.h ***/
#define CGF_E		// �p��ł͂�����`�B���\�[�X�͉p��ŁA���{��ł𓝈�B
//#define CGF_DEV	// �J����

#define BAN_9   9 // �X�H��
#define BAN_13 13 // 13�H��
#define BAN_19 19 // 19�H��
#define BANMAX ((19+2)*256)	// = 21*256		������21*32=672�ɂ������B8����1�ɂȂ邵�B10bit�ō��W��\���B��͂�ʓ|���E�E�E

#define KIFU_MAX 2048	// �L�^�\�ȍő�萔

extern int ban_size;	// �Ղ̑傫�� 9,13,19;
extern int tesuu;		// �萔
extern int all_tesuu;	// �Ō�܂ł̎萔
extern int kifu[KIFU_MAX][3];		// ����	 [0]...�ʒu�A[1]...�F�A[2]...�v�l���ԁi���̎�́j


// make_move()�֐��Ŏ��i�߂����̌���
enum MoveResult {
	MOVE_SUCCESS,	// ����
	MOVE_SUICIDE,	// ���E��
	MOVE_KOU,		// �R�E
	MOVE_EXIST,		// ���ɐ΂�����
	MOVE_FATAL,		// ����ȊO

	// �ȉ���set_kifu_time()�ŗ��p
	MOVE_PASS_PASS,	// �A���p�X�A�΋ǏI��
	MOVE_KIFU_OVER,	// �萔��������
	MOVE_RESIGN,	// ����
};


extern int kou_iti;			// ���ɂȂ�ꍇ�̈ʒu
extern int hama[2];			// ���Ɣ��̗g�l�i���΂̐��j

extern int ban[];			// �͌�Ղ̏�ԁB�� 0�A�� 1�A�� 2�A�� 3
extern int ban_start[];		// �͌�Ղ̊J�n��ԁB
extern int dll_col;			// dll������(1)����(2)

extern double komi;			// �R�~�B6.5��6�ڔ��B0�Ōݐ�

extern int thinking_utikiri_flag;	// ���f�t���O
extern int total_time[2];	// �݌v�̎v�l����
						
extern int ban_9[][11];		// �����Ֆʔz�u���
extern int ban_13[][15];
extern int ban_19[][21];


#define UN_COL(col) (3-(col))	// ����̐΂̐F


#define SGF_BUF_MAX 65536			// SGF��ǂݍ��ނ��߂̃o�b�t�@
extern unsigned long nSgfBufSize;	// �o�b�t�@�Ƀ��[�h�����T�C�Y
extern unsigned long nSgfBufNum;	// �o�b�t�@�̐擪�ʒu������
extern char SgfBuf[SGF_BUF_MAX];	// �o�b�t�@

#define IsKanji(c) (( 0x80<(c) && (c)<0xa0 ) || ( 0xdf<(c) && (c)<0xfd ))	// �����̐擪�o�C�g���H c ��unsigned char�łȂ��ƃo�O��

#define TMP_BUF_LEN	256			// ������p�̓K���Ȕz�񒷂�

// cgf_term.cpp
extern int RetZ;		// �ʐM�Ŏ󂯎���̍��W
extern int RetComm;		// �A���Ă����f�[�^�̎��
extern int nCommNumber;	// COM�|�[�g�̔ԍ��B


//////////////////////////////////////////////////////////////////////////



// �֐��̃v���g�^�C�v�錾
// cgf_win.cpp
void PRT(char *fmt, ...);			// �ς̃��X�g������printf��p�֐�
void PassWindowsSystem(void);		// Windows�ֈꎞ�I�ɐ����n���BUser ����̎w���ɂ��v�l��~�B
void LoadSetSGF(int fSgf);			// SGF�����[�h���ĔՖʂ��\������
void SaveSGF(void);					// SGF�������o��
void move_tesuu(int n);				// �C�ӂ̎萔�̋ǖʂɔՖʂ��ړ�������
int ThinkingDLL(int endgame_type);	// �v�l���[�`��
int count_endgame_score(void);		// �I�Ǐ�����������ŁA�n�̐��݂̂𐔂���B(���n �| ���n)

// cgf.cpp
void reconstruct_ban(void);		// �Ֆʂ̏��݂̂���
void hyouji_clipboard(void);	// �N���b�v�{�[�h�ɔՖʂ��R�s�[
void hyouji(void);				// �e�L�X�g�ł̔Ֆʕ\��	
void debug(void);				// �G���[����
void print_tejun(void);			// ���܂ł̒T���菇��\��
void set_tejun_kifu(void);		// ���܂ł̎菇�������ɃZ�b�g�B�k���悤�ɁB
void hyouji_saizen(void);		// �őP����菇��\������
void set_saizen_kifu(void);
void init_ban(void);

// cgf_main.cpp
int make_move(int z, int col);	// ���i�߂�Bz ... ���W�Acol ... �΂̐F

int count_total_time(void);				// �݌v�̎v�l���Ԃ��v�Z���Ȃ��������B���݂̎�Ԃ̗݌v�v�l���Ԃ�Ԃ�
int set_kifu_time(int z,int col,int t);	// �����Ɏ����������ŋǖʂ�i�߂ĉ�ʂ��X�V 
double GetSpendTime(clock_t ct1);		// �o�ߎ��Ԃ�b�ŕԂ�
int GetTurnCol(void);					// ���݂̎�Ԃ̐F��Ԃ��i���ݎ萔�Ə��肪���Ԃ��ǂ����Ŕ��f����j
int IsBlackTurn(void);					// ���̎�Ԃ��H


