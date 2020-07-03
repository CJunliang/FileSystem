//
// Created by liang on 2020/6/11.
//
#include <cstdio>
#include <sys/shm.h>
#include <cstdlib>
#include <ctime>
#include "DiskManager.h"

/*共享内存的区域*/
ShareMemoryStruct *shmSt;
/*共享内存的初始地址*/
unsigned char *baseAddr;
/*超级块的空间地址*/
SuperBlock *superBlock;
/*inode开始的地址*/
Inode *inode;
/*此时正在使用的group*/
Group *group;
/*当前目录的inode编号*/
unsigned int curInodeIndex;

void allocMemory() {
    int shmId;
    void *shareMemory;

    /*获取key值*/
    key_t key = ftok("./DiskManager.cpp", 100);
    /*创建共享内存*/
    shmId = shmget(key, sizeof(ShareMemoryStruct), 0666 | IPC_CREAT);
    if (shmId < 0) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    /*将共享内存连接到当前进程的地址空间*/
    shareMemory = shmat(shmId, nullptr, 0);
    if (shareMemory < (void *) nullptr) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    /*设置共享内存并让使用程序次数加1*/
    shmSt = (ShareMemoryStruct *) shareMemory;
    if (!shmSt->isInit)
        diskInit();
    else load();
}

/*初始化inode*/
void initInode(unsigned int index, unsigned char attribute) {
    Inode *node = &inode[index];
    node->size = 0;
    node->linkCount = 0;
    node->attribute = attribute;
    node->ctime = time(nullptr);
    node->mtime = time(nullptr);
    for (unsigned int &i : node->dirBlock)
        i = -1;
}

/*获取block地址*/
void *getBlockAddr(unsigned int num) {
    return baseAddr + BLOCKSIZE * num;
}

/*分配inode*/
unsigned int inodeAlloc() {
    unsigned int index;
    if (superBlock->freeInodeAmt) {
        index = superBlock->inode[superBlock->inodeAmt - superBlock->freeInodeAmt];
        superBlock->freeInodeAmt--;
        return index;
    }
    printf("No Inode Left!\n");
    return -1;
}

/*回收inode*/
void inodeFree(unsigned int num) {
    superBlock->freeInodeAmt++;
    superBlock->inode[superBlock->inodeAmt - superBlock->freeInodeAmt] = num;
}

/*分配block*/
unsigned int blockAlloc() {
    unsigned int index;
    unsigned int size = superBlock->blocksPerGroup;
    if (group->count > 1) {
        index = group->blocks[size - group->count];
        group->count--;
        return index;
    }
    if (group->blocks[size - 1]) {
        superBlock->group = group->blocks[size - 1];
        group = (Group *) getBlockAddr(superBlock->group);
        index = group->blocks[0];
        group->count--;
        return index;
    }
    printf("No Space Left!\n");
    return -1;
}

/*回收block*/
void blockFree(unsigned int num) {
    unsigned int size = superBlock->blocksPerGroup;
    group->count++;
    group->blocks[size - group->count] = num;
    if (group->count == group->max) {
        superBlock->group -= size;
        group = (Group *) getBlockAddr(superBlock->group);
    }
}

/*初始化*/
void diskInit() {
    unsigned int i, j;

    baseAddr = reinterpret_cast<unsigned char *>(&shmSt->shm);
    printf("\033[33mLoading FileSystem...\033[0m\n");
    superBlock = (SuperBlock *) baseAddr;

    /*初始化超级块*/
    superBlock->atime = time(nullptr);
    superBlock->diskSize = DISKSIZE;
    superBlock->blockSize = BLOCKSIZE;
    superBlock->inodeSize = 128;
    superBlock->inodeBlocks = 387;
    superBlock->inodeAmt = 3096;
    superBlock->freeInodeAmt = 3096;
    superBlock->blockAmt = DISKSIZE / BLOCKSIZE;
    superBlock->freeBlockAmt = DISKSIZE / BLOCKSIZE;
    superBlock->blocksPerGroup = 255;
    superBlock->fileIndexSize = 32;
    superBlock->fileIndexPerBlock = BLOCKSIZE / superBlock->fileIndexSize;
    for (i = 0; i < superBlock->inodeAmt; i++)
        superBlock->inode[i] = i;

    /*初始化group*/
    unsigned int size = superBlock->blocksPerGroup;
    for (i = 400; i < superBlock->blockAmt; i += size) {
        group = (Group *) getBlockAddr(i);
        for (j = 0; j < size - 1; j++)
            group->blocks[j] = i + j;
        group->blocks[size - 1] = i + size;
        group->count = size;
        group->max = size;
    }
    group->blocks[size - 1] = 0;
    group->count = size - 1;
    group->max = size - 1;
    /*初始化为当前使用的组号*/
    group = (Group *) getBlockAddr(400);

    /*初始化根目录的inode*/
    superBlock->rootInode = inodeAlloc();
    inode = (Inode *) getBlockAddr(13);
    curInodeIndex = superBlock->rootInode;
    superBlock->mtime = time(nullptr);
    initInode(superBlock->rootInode, 1);
    printf("\033[32mFileSystem Created!\033[0m\n");
}

void load() {
    baseAddr = reinterpret_cast<unsigned char *>(&shmSt->shm);
    printf("\033[33mLoading FileSystem...\033[0m\n");
    superBlock = (SuperBlock *) baseAddr;
    group = (Group *) getBlockAddr(400);
    inode = (Inode *) getBlockAddr(13);
    curInodeIndex = superBlock->rootInode;
    printf("\033[32mFileSystem Loaded!\033[0m\n");
}