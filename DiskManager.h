//
// Created by liang on 2020/6/11.
//
#ifndef FILESYSTEM_DISKMANAGER_H
#define FILESYSTEM_DISKMANAGER_H

/*分别是系统大小，盘块大小和盘块数量*/
#include <semaphore.h>
#include <pthread.h>

#define KB 1024
#define MB 1024*1024
#define DISKSIZE 100*MB
#define BLOCKSIZE KB
#define BLOCKCOUNT DISKSIZE/BLOCKSIZE

struct ShareMemoryStruct {
    unsigned char shm[DISKSIZE];    /*磁盘的大小*/
    bool isInit;                    /*是否初始化*/
};

/*Inode节点的大小为128B，存储了文件的一些信息*/
typedef struct INODE {
    unsigned long size;     /*文件的大小*/
    unsigned long ctime;    /*inode上一次变动的时间*/
    unsigned long mtime;    /*文件内容上一次变动的时间*/
    unsigned long atime;    /*文件上一次打开的时间*/
    sem_t writeMutex;         /*写信号量*/
    sem_t readMutex;          /*读信号量*/
    unsigned int readCount;  /*正在读的数量*/
    unsigned int linkCount; /*链接的数量*/
    unsigned int owner;     /*拥有者编号*/
    unsigned int blockCount;    /*使用逻辑块的数量*/
    unsigned int dirBlock[12];  /*直接块指针*/
    unsigned int inDirBlock;     /*间接块指针*/
    unsigned int dbInDirBlock;  /*双重间接地址*/
    unsigned int triInDirBlock; /*三重间接地址*/
    unsigned char fill[11];     /*用来填充inode，无实际作用*/
    bool attribute;    /*0文件，1文件夹*/
} Inode;

typedef struct SUPERBLOCK {
    unsigned short blockSize;       /*文件系统的数据块大小*/
    unsigned short inodeSize;       /*inode块的大小*/
    unsigned short inodeBlocks;     /*inode所占的块数*/
    unsigned int diskSize;        /*文件系统的大小*/
    unsigned int inodeAmt;          /*inode的数量*/
    unsigned int freeInodeAmt;      /*inode空闲的数量*/
    unsigned int blockAmt;          /*整个文件系统的数据块数*/
    unsigned int freeBlockAmt;      /*空闲块数目*/
    unsigned int blocksPerGroup;    /*每一个块组管理的逻辑块的数量*/
    unsigned int fileIndexPerBlock; /*每块所包含的文件索引个数*/
    unsigned int fileIndexSize;     /*文件索引的大小*/
    unsigned int rootInode;         /*根目录所在的inode*/
    unsigned int inode[3096];       /*inode节点的个数*/
    unsigned int group;             /*此时正在使用的组号（逻辑好）*/
    unsigned long mtime;            /*超级块被修改的时间*/
    unsigned long atime;            /*超级块创建时间*/
} SuperBlock;

/*512B */
typedef struct BOOT {
    unsigned short root;
    unsigned char *startBlock;
    char sysName[32];
    char sysInfo[32];
} Boot;

/*32B 记录文件名字以及文件的inode编号*/
typedef struct INDEX {
    char fileName[28];
    unsigned int inode;
} FileIndex;

/*1024B */
typedef struct GROUP {
    unsigned int blocks[255];   /*块号数组*/
    unsigned short count;       /*空闲盘块*/
    unsigned short max;         /*最多空闲盘块*/
} Group;

/*共享内存的区域*/
extern ShareMemoryStruct *shmSt;
/*共享内存的初始地址*/
extern unsigned char *baseAddr;
/*超级块的空间地址*/
extern SuperBlock *superBlock;
/*inode开始的地址*/
extern Inode *inode;
/*此时正在使用的group*/
extern Group *group;
/*当前目录的inode编号*/
extern unsigned int curInodeIndex;
/*是否显示inode和block的分配和回收信息*/
extern bool track;

/*创建共享内存*/
void allocMemory();

/*初始化*/
void diskInit();

/*获取block地址*/
void *getBlockAddr(unsigned int num);

/*分配inode*/
unsigned int inodeAlloc();

/*回收inode*/
void inodeFree(unsigned int num);

/*分配block*/
unsigned int blockAlloc();

/*回收block*/
void blockFree(unsigned int num);

void load();

/*初始化inode*/
void initInode(unsigned int index, bool attribute);

/*退出系统，结束共享内存在本进程的挂载映像，删除共享内存区域*/
void exitSys();

#endif //FILESYSTEM_DISKMANAGER_H
