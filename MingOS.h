#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

//宏定义
#define BLOCK_SIZE	4096						//块号大小为512B										一个磁盘块的大小512字节
#define INODE_SIZE	128						//inode节点大小为128B。注：sizeof(Inode)不能超过该值
#define MAX_NAME_SIZE 28					//最大名字长度，长度要小于这个大小

#define INODE_NUM	4096						//inode节点数,最多640个文件
#define BLOCK_NUM	262144					//块号数，10240 * 512B = 5120KB
#define BLOCKS_PER_GROUP	64				//空闲块堆栈大小，一个空闲堆栈最多能存多少个磁盘块地址

#define MODE_DIR	1					//目录标识
#define MODE_FILE	0					//文件标识

#define FILESYSNAME	"MingOS.sys"			//虚拟磁盘文件名		初始化的时候创建的


//结构体声明
//超级块
struct SuperBlock{
	unsigned short s_INODE_NUM;				//inode节点数，最多 65535			2字节
	unsigned int s_BLOCK_NUM;				//磁盘块块数，最多 4294967294		4字节

	unsigned short s_free_INODE_NUM;		//空闲inode节点数
	unsigned int s_free_BLOCK_NUM;			//空闲磁盘块数
	int s_free_addr;						//空闲块堆栈指针
	int s_free[BLOCKS_PER_GROUP];			//空闲块堆栈				空闲栈，存的是空闲块的标号？

	unsigned short s_BLOCK_SIZE;			//磁盘块大小
	unsigned short s_INODE_SIZE;			//inode大小
	unsigned short s_SUPERBLOCK_SIZE;		//超级块大小
	unsigned short s_blocks_per_group;		//每 blockgroup 的block数量			

	//磁盘分布
	int s_Superblock_StartAddr;				//磁盘的各地址的起始块标号
	int s_InodeBitmap_StartAddr;
	int s_BlockBitmap_StartAddr;
	int s_Inode_StartAddr;
	int s_Block_StartAddr;
};

//inode节点
struct Inode{
	unsigned short i_ino;					//inode标识（编号）
	unsigned short i_mode;					//文件目录的标志
	unsigned short i_cnt;					//链接数。有多少文件名指向这个inode
	char i_uname[20];						//文件所属用户
	char i_gname[20];						//文件所属用户组
	unsigned int i_size;					//文件大小，单位为字节（B）
	time_t  i_ctime;						//inode上一次变动的时间
	time_t  i_mtime;						//文件内容上一次变动的时间
	time_t  i_atime;						//文件上一次打开的时间
	int i_dirBlock[10];						//10个直接块。10*512B = 5120B = 5KB		存的是目录数据的block字节地址，空闲表示为-1，存的是字节位置，得自己找块号，里面存的是16个结构体DirItem
	int i_indirBlock_1;						//一级间接块。512B/4 * 512B = 128 * 512B = 64KB
	//unsigned int i_indirBlock_2;			//二级间接块。(512B/4)*(512B/4) * 512B = 128*128*512B = 8192KB = 8MB
	//unsigned int i_indirBlock_3;			//三级间接块。(512B/4)*(512B/4)*(512B/4) * 512B = 128*128*128*512B = 1048576KB = 1024MB = 1G							间接块的意思是指向磁盘块，但磁盘块里存的是另外磁盘块的地址，相当于间接寻址
											//文件系统太小，暂且省略二级、三级间接块
};

//目录项
struct DirItem{								//32字节，一个磁盘块能存 512/32=16个目录项
	char itemName[MAX_NAME_SIZE];			//目录或者文件名				28字节
	int inodeAddr;							//目录项对应的inode节点地址		4字节
};

//全局变量声明
extern SuperBlock *superblock;
extern const int Inode_StartAddr;
extern const int Superblock_StartAddr;		//超级块的偏移地址,超级块占一个磁盘块
extern const int InodeBitmap_StartAddr;		//inode位图 偏移地址，占两个磁盘块，最多监控 1024 个inode的状态 	2*512字节，位图是bool类型，bool是一个字节
extern const int BlockBitmap_StartAddr;		//block位图 偏移地址，占二十个磁盘块，最多监控 10240 个磁盘块（5120KB）的状态
extern const int Inode_StartAddr;			//inode节点区 偏移地址，占 INODE_NUM/(BLOCK_SIZE/INODE_SIZE) 个磁盘块
extern const int Block_StartAddr;			//block数据区 偏移地址 ，占 INODE_NUM 个磁盘块
extern const int File_Max_Size;				//单个文件最大大小
extern const int Sum_Size;					//虚拟磁盘文件大小


//全局变量声明
extern int Root_Dir_Addr;					//根目录inode地址
extern int Cur_Dir_Addr;					//当前目录
extern char Cur_Dir_Name[310];				//当前目录名
extern char Cur_Host_Name[110];				//当前主机名
extern char Cur_User_Name[110];				//当前登陆用户名

extern bool isLogin;						//是否有用户登陆

extern FILE* fw;							//虚拟磁盘文件 写文件指针
extern FILE* fr;							//虚拟磁盘文件 读文件指针
extern SuperBlock *superblock;				//超级块指针
extern bool inode_bitmap[INODE_NUM];		//inode位图     640字节 对应640个inode
extern bool block_bitmap[BLOCK_NUM];		//磁盘块位图	10240字节 对应10240个磁盘块

extern char buffer[10000000];				//10M，缓存整个虚拟磁盘文件  模拟内存里的缓存


//函数声明
void Ready();													//登录系统前的准备工作,注册+安装
bool Format();													//格式化一个虚拟磁盘文件
bool Install();													//安装文件系统，将虚拟磁盘文件中的关键信息如超级块读入到内存
int	 balloc();													//磁盘块分配函数
bool bfree();													//磁盘块释放函数
int  ialloc();													//分配i节点区函数
bool ifree();													//释放i结点区函数
bool mkdir(int parinoAddr,char name[]);							//目录创建函数。参数：上一层目录文件inode地址 ,要创建的目录名
bool rmdir(int parinoAddr,char name[]);							//目录删除函数
bool create(int parinoAddr,char name[],char buf[]);				//创建文件函数
bool del(int parinoAddr,char name[]);							//删除文件函数 
void ls(int parinoaddr);										//显示当前目录下的所有文件和文件夹
void cd(int parinoaddr,char name[]);							//进入当前目录下的name目录
void writefile(Inode fileInode,int fileInodeAddr,char buf[]);	//将buf内容写回文件的磁盘块
void inUsername(char username[]);								//输入用户名
void inPasswd(char passwd[]);									//输入密码
bool login();													//登陆界面
bool check(char username[],char passwd[]);						//核对用户名，密码
void logout();													//用户注销
bool useradd(char username[]);									//用户注册
bool userdel(char username[]);									//用户删除
void touch(int parinoAddr,char name[],char buf[]);				//touch命令创建文件，读入字符
void help();													//显示所有命令清单

void cmd(char str[]);											//处理输入的命令
