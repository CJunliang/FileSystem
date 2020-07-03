//
// Created by liang on 2020/6/11.
//
#include <iostream>
#include <vector>
#include <cstring>
#include "FileOperate.h"

using namespace std;

vector<string> commandLine;
vector<string> pathClip;

vector<string> split(const string &str, const string &delimiter) {
    vector<string> res;
    if (str.empty()) return res;
    //先将要切割的字符串从string类型转换为char*类型
    char *strs = new char[str.length() + 1]; //不要忘了
    strcpy(strs, str.c_str());
    char *d = new char[delimiter.length() + 1];
    strcpy(d, delimiter.c_str());
    char *p = strtok(strs, d);
    while (p) {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(nullptr, d);
    }
    return res;
}

unsigned int checkName(Inode *node,char *name) {
    int size = sizeof(FileIndex);
//    Inode *node = &inode[curInodeIndex];
    int fileNum = node->size / size;
    int usedBlocks = node->size / BLOCKSIZE + 1;
    if (!fileNum)
        return 0;
    for (int i = 0; i < usedBlocks; i++) {
        auto *fileIndex = (FileIndex *) getBlockAddr(node->dirBlock[i]);
        if (i == usedBlocks - 1) {
            for (int j = 0; j < fileNum % superBlock->fileIndexPerBlock; j++)
                if (!strcmp(name, fileIndex[j].fileName))
                    return fileIndex[j].inode;
        } else {
            for (int j = 0; j < superBlock->fileIndexPerBlock; j++)
                if (!strcmp(name, fileIndex[j].fileName))
                    return fileIndex[j].inode;
        }
    }
    return 0;
}

FileIndex *getFileIndex(char *name) {
    int size = sizeof(FileIndex);
    Inode *node = &inode[curInodeIndex];
    int fileNum = node->size / size;
    int usedBlocks = node->size / BLOCKSIZE + 1;
    for (int i = 0; i < usedBlocks; i++) {
        FileIndex *fileIndex = (FileIndex *) getBlockAddr(node->dirBlock[i]);
        if (i == usedBlocks - 1) {
            for (int j = 0; j < fileNum % superBlock->fileIndexPerBlock; j++)
                if (!strcmp(name, fileIndex[j].fileName))
                    return &fileIndex[j];
        } else {
            for (int j = 0; j < superBlock->fileIndexPerBlock; j++)
                if (!strcmp(name, fileIndex[j].fileName))
                    return &fileIndex[j];
        }
    }
    return nullptr;
}

void addFatherFolder(Inode *node) {
    int size = sizeof(FileIndex);
    node->dirBlock[0] = blockAlloc();
    auto *fileIndex = (FileIndex *) getBlockAddr(node->dirBlock[0]);
    strcpy(fileIndex[0].fileName, "..");
    fileIndex[0].inode = curInodeIndex;
    node->size += size;
    node->ctime = node->mtime = time(nullptr);
}

void createNewFile(Inode *node, char *name, bool attribute) {
    int size = sizeof(FileIndex);
    int fileNum = node->size / size;
    int usedBlocks = node->size / BLOCKSIZE;
    if (fileNum % superBlock->fileIndexPerBlock == 0)
        node->dirBlock[usedBlocks] = blockAlloc();
    unsigned int index = fileNum % superBlock->fileIndexPerBlock;
    auto *fileIndex = (FileIndex *) getBlockAddr(node->dirBlock[usedBlocks]);
    strcpy(fileIndex[index].fileName, name);
    fileIndex[index].inode = inodeAlloc();
    initInode(fileIndex[index].inode, attribute);
    node->size += size;
    node->ctime = node->mtime = time(nullptr);
    if (attribute) {
        addFatherFolder(&inode[fileIndex[index].inode]);
    }
}

void fileInit() {
    allocMemory();
    path = "/";
}

void pwd() {
    cout << path << endl;
}

void exit() {}

void changePath(Inode *startInode) {
    Inode *node=startInode;
    unsigned int index;
    for (int i = 0; i < pathClip.size(); i++) {
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        index=checkName(node,name);
        if(!index){
            printf("cd: No such file or directory!\n");
            return;
        }
    }
}

void cd() {
    if (commandLine.size() > 2) {
        printf("cd: too much arguments!\n");
        return;
    }
//    char *jPath = new char(commandLine[1].length() + 1);
//    strcpy(jPath, commandLine[1].c_str());
    pathClip = split(commandLine[1], "/+");
    if (commandLine[1][0] == '/') {
        printf("d");
    } else {

    }
    pathClip.clear();
}

void mkdir() {
    for (int i = 1; i < commandLine.size(); i++) {
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        if (checkName(&inode[curInodeIndex],name)) {
            printf("mkdir: Name %s is used!\n", name);
            return;
        }
        Inode *node = &inode[curInodeIndex];
        createNewFile(node, name, true);
    }
}

void rmdir() {
    for (int i = 1; i < commandLine.size(); i++) {
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        FileIndex *fileIndex = getFileIndex(name);
        if (!fileIndex) {
            printf("rm: No such directory!\n");
            return;
        }
        unsigned int ino = fileIndex->inode;
        if (ino) {
            Inode *node = &inode[ino];
            if (node->attribute) {
                node->linkCount--;
                memset(fileIndex, 0, sizeof(FileIndex));
                int j = 0;
                while (node->dirBlock[j] != -1 && j < 12) {
                    printf("block %d free\n", node->dirBlock[j]);
                    blockFree(node->dirBlock[j++]);
                }
                inodeFree(ino);
                memset(node, 0, sizeof(Inode));
                inode[curInodeIndex].ctime = time(nullptr);
                inode[curInodeIndex].atime = time(nullptr);
                inode[curInodeIndex].mtime = time(nullptr);
            } else {
                printf("rmdir: failed to remove '%s': Not a directory\n", name);
            }
        } else {
            printf("rmdir: fail to remove '%s': No such directory!\n", name);
        }
    }
}

void rm() {
    for (int i = 1; i < commandLine.size(); i++) {
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        FileIndex *fileIndex = getFileIndex(name);
        if (!fileIndex) {
            printf("rm: No such file!\n");
            return;
        }
        unsigned int ino = fileIndex->inode;
        if (ino) {
            Inode *node = &inode[ino];
            if (!node->attribute) {
                node->linkCount--;
                memset(fileIndex, 0, sizeof(FileIndex));
                int j;
                while (node->dirBlock[j] != -1 && j < 12) {
                    printf("block %d free\n", node->dirBlock[j]);
                    blockFree(node->dirBlock[j++]);
                }
                inodeFree(ino);
                memset(node, 0, sizeof(Inode));
                inode[curInodeIndex].ctime = time(nullptr);
                inode[curInodeIndex].atime = time(nullptr);
                inode[curInodeIndex].mtime = time(nullptr);
            } else {
                printf("rm: failed to remove '%s': Not a file\n", name);
            }
        } else {
            printf("rm: fail to remove '%s': No such file!\n", name);
        }
    }
}

void touch() {
    for (int i = 1; i < commandLine.size(); i++) {
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        if (checkName(&inode[curInodeIndex],name)) {
            printf("touch: Name %s is used!\n", name);
            return;
        }
        Inode *node = &inode[curInodeIndex];
        createNewFile(node, name, true);
    }
}

void read() {}

void write() {}

void open() {}

void printInode(Inode *node, FileIndex *fileIndex) {
    if (!fileIndex->fileName[0])
        return;
    if (node->attribute) {
        printf(" dir  ");
        printf("\033[33m%6s  \033[0m", fileIndex->fileName);
    } else {
        printf(" file ");
        printf("%6s", fileIndex->fileName);
    }
    printf("%5u  ", node->linkCount);
    printf("%8lu  ", node->size);
    printf("%8u  ", fileIndex->inode);
    printf("-- %s", ctime((time_t *) &node->atime));
}

void ls() {
    int size = sizeof(FileIndex);
    Inode *node = &inode[curInodeIndex];
    int fileNum = node->size / size;
    printf("\033[32m type  %6s  links   size   inode   atime\033[0m\n", "name");
    int usedBlocks = node->size / BLOCKSIZE + 1;
    for (int i = 0; i < usedBlocks; i++) {
        auto *fileIndex = (FileIndex *) getBlockAddr(node->dirBlock[i]);
        if (i == usedBlocks - 1) {
            for (int j = 0; j < fileNum % superBlock->fileIndexPerBlock; j++) {
                Inode *tInode = &inode[fileIndex[j].inode];
                printInode(tInode, &fileIndex[j]);
            }
        } else {
            for (int j = 0; j < superBlock->fileIndexPerBlock; j++) {
                Inode *tInode = &inode[fileIndex[j].inode];
                printInode(tInode, &fileIndex[j]);
            }
        }
    }
}
