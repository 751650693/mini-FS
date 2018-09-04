#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

//�궨��
#define BLOCK_SIZE	4096						//��Ŵ�СΪ512B										һ�����̿�Ĵ�С512�ֽ�
#define INODE_SIZE	128						//inode�ڵ��СΪ128B��ע��sizeof(Inode)���ܳ�����ֵ
#define MAX_NAME_SIZE 28					//������ֳ��ȣ�����ҪС�������С

#define INODE_NUM	4096						//inode�ڵ���,���640���ļ�
#define BLOCK_NUM	262144					//�������10240 * 512B = 5120KB
#define BLOCKS_PER_GROUP	64				//���п��ջ��С��һ�����ж�ջ����ܴ���ٸ����̿��ַ

#define MODE_DIR	1					//Ŀ¼��ʶ
#define MODE_FILE	0					//�ļ���ʶ

#define FILESYSNAME	"MingOS.sys"			//��������ļ���		��ʼ����ʱ�򴴽���


//�ṹ������
//������
struct SuperBlock{
	unsigned short s_INODE_NUM;				//inode�ڵ�������� 65535			2�ֽ�
	unsigned int s_BLOCK_NUM;				//���̿��������� 4294967294		4�ֽ�

	unsigned short s_free_INODE_NUM;		//����inode�ڵ���
	unsigned int s_free_BLOCK_NUM;			//���д��̿���
	int s_free_addr;						//���п��ջָ��
	int s_free[BLOCKS_PER_GROUP];			//���п��ջ				����ջ������ǿ��п�ı�ţ�

	unsigned short s_BLOCK_SIZE;			//���̿��С
	unsigned short s_INODE_SIZE;			//inode��С
	unsigned short s_SUPERBLOCK_SIZE;		//�������С
	unsigned short s_blocks_per_group;		//ÿ blockgroup ��block����			

	//���̷ֲ�
	int s_Superblock_StartAddr;				//���̵ĸ���ַ����ʼ����
	int s_InodeBitmap_StartAddr;
	int s_BlockBitmap_StartAddr;
	int s_Inode_StartAddr;
	int s_Block_StartAddr;
};

//inode�ڵ�
struct Inode{
	unsigned short i_ino;					//inode��ʶ����ţ�
	unsigned short i_mode;					//�ļ�Ŀ¼�ı�־
	unsigned short i_cnt;					//���������ж����ļ���ָ�����inode
	char i_uname[20];						//�ļ������û�
	char i_gname[20];						//�ļ������û���
	unsigned int i_size;					//�ļ���С����λΪ�ֽڣ�B��
	time_t  i_ctime;						//inode��һ�α䶯��ʱ��
	time_t  i_mtime;						//�ļ�������һ�α䶯��ʱ��
	time_t  i_atime;						//�ļ���һ�δ򿪵�ʱ��
	int i_dirBlock[10];						//10��ֱ�ӿ顣10*512B = 5120B = 5KB		�����Ŀ¼���ݵ�block�ֽڵ�ַ�����б�ʾΪ-1��������ֽ�λ�ã����Լ��ҿ�ţ���������16���ṹ��DirItem
	int i_indirBlock_1;						//һ����ӿ顣512B/4 * 512B = 128 * 512B = 64KB
	//unsigned int i_indirBlock_2;			//������ӿ顣(512B/4)*(512B/4) * 512B = 128*128*512B = 8192KB = 8MB
	//unsigned int i_indirBlock_3;			//������ӿ顣(512B/4)*(512B/4)*(512B/4) * 512B = 128*128*128*512B = 1048576KB = 1024MB = 1G							��ӿ����˼��ָ����̿飬�����̿�������������̿�ĵ�ַ���൱�ڼ��Ѱַ
											//�ļ�ϵͳ̫С������ʡ�Զ�����������ӿ�
};

//Ŀ¼��
struct DirItem{								//32�ֽڣ�һ�����̿��ܴ� 512/32=16��Ŀ¼��
	char itemName[MAX_NAME_SIZE];			//Ŀ¼�����ļ���				28�ֽ�
	int inodeAddr;							//Ŀ¼���Ӧ��inode�ڵ��ַ		4�ֽ�
};

//ȫ�ֱ�������
extern SuperBlock *superblock;
extern const int Inode_StartAddr;
extern const int Superblock_StartAddr;		//�������ƫ�Ƶ�ַ,������ռһ�����̿�
extern const int InodeBitmap_StartAddr;		//inodeλͼ ƫ�Ƶ�ַ��ռ�������̿飬����� 1024 ��inode��״̬ 	2*512�ֽڣ�λͼ��bool���ͣ�bool��һ���ֽ�
extern const int BlockBitmap_StartAddr;		//blockλͼ ƫ�Ƶ�ַ��ռ��ʮ�����̿飬����� 10240 �����̿飨5120KB����״̬
extern const int Inode_StartAddr;			//inode�ڵ��� ƫ�Ƶ�ַ��ռ INODE_NUM/(BLOCK_SIZE/INODE_SIZE) �����̿�
extern const int Block_StartAddr;			//block������ ƫ�Ƶ�ַ ��ռ INODE_NUM �����̿�
extern const int File_Max_Size;				//�����ļ�����С
extern const int Sum_Size;					//��������ļ���С


//ȫ�ֱ�������
extern int Root_Dir_Addr;					//��Ŀ¼inode��ַ
extern int Cur_Dir_Addr;					//��ǰĿ¼
extern char Cur_Dir_Name[310];				//��ǰĿ¼��
extern char Cur_Host_Name[110];				//��ǰ������
extern char Cur_User_Name[110];				//��ǰ��½�û���

extern bool isLogin;						//�Ƿ����û���½

extern FILE* fw;							//��������ļ� д�ļ�ָ��
extern FILE* fr;							//��������ļ� ���ļ�ָ��
extern SuperBlock *superblock;				//������ָ��
extern bool inode_bitmap[INODE_NUM];		//inodeλͼ     640�ֽ� ��Ӧ640��inode
extern bool block_bitmap[BLOCK_NUM];		//���̿�λͼ	10240�ֽ� ��Ӧ10240�����̿�

extern char buffer[10000000];				//10M������������������ļ�  ģ���ڴ���Ļ���


//��������
void Ready();													//��¼ϵͳǰ��׼������,ע��+��װ
bool Format();													//��ʽ��һ����������ļ�
bool Install();													//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
int	 balloc();													//���̿���亯��
bool bfree();													//���̿��ͷź���
int  ialloc();													//����i�ڵ�������
bool ifree();													//�ͷ�i���������
bool mkdir(int parinoAddr,char name[]);							//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
bool rmdir(int parinoAddr,char name[]);							//Ŀ¼ɾ������
bool create(int parinoAddr,char name[],char buf[]);				//�����ļ�����
bool del(int parinoAddr,char name[]);							//ɾ���ļ����� 
void ls(int parinoaddr);										//��ʾ��ǰĿ¼�µ������ļ����ļ���
void cd(int parinoaddr,char name[]);							//���뵱ǰĿ¼�µ�nameĿ¼
void writefile(Inode fileInode,int fileInodeAddr,char buf[]);	//��buf����д���ļ��Ĵ��̿�
void inUsername(char username[]);								//�����û���
void inPasswd(char passwd[]);									//��������
bool login();													//��½����
bool check(char username[],char passwd[]);						//�˶��û���������
void logout();													//�û�ע��
bool useradd(char username[]);									//�û�ע��
bool userdel(char username[]);									//�û�ɾ��
void touch(int parinoAddr,char name[],char buf[]);				//touch������ļ��������ַ�
void help();													//��ʾ���������嵥

void cmd(char str[]);											//�������������
