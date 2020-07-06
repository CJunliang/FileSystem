//
// Created by liang on 2020/6/11.
//
#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include "FileOperate.h"

using namespace std;

vector<string> commandLine;
vector<string> pathClip;
sem_t mutex;

/*初始化磁盘和当前目录*/
void fileInit() {
    allocMemory();
    path = "/";
    sem_init(&mutex, 1, 1);
}

/*根据传入的string和分割符切成string类型的块存入到vector中*/
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

/*根据传入的inode和name检查该inode对应的目录项中是否存在和name
 * 相同名字的文件，如果有则返回该文件的inode编号，如果没有则返回-1*/
unsigned int checkName(Inode *node, char *name) {
    int size = sizeof(FileIndex);
    int fileNum = node->size / size;
    int usedBlocks = node->size / BLOCKSIZE + 1;
    if (!fileNum)
        return -1;
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
    return -1;
}

/*根据传入的inode从当前目录所在的inode中查找是否有同名的目录项，
 * 如果有则返回目录项FileIndex的地址，如果没有则返回null*/
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

/*为目录创建一个fileName为“..”，inode为上一层目录inode的
 * 目录项FileIndex，目的是为了在使用cd命令时能返回上一层*/
void addFatherFolder(Inode *node) {
    int size = sizeof(FileIndex);
    node->dirBlock[0] = blockAlloc();
    if (node->dirBlock[0] == -1)
        return;
    node->blockCount++;
    auto *fileIndex = (FileIndex *) getBlockAddr(node->dirBlock[0]);
    strcpy(fileIndex[0].fileName, "..");
    fileIndex[0].inode = curInodeIndex;
    node->size += size;
    node->ctime = node->mtime = time(nullptr);
}

/*根据传进来的inode节点、名字name和文件属性，根据该文件属性创建
 * 一个在该inode节点下的名字为name的目录或者文件*/
void createNewFile(Inode *node, char *name, bool attribute) {
    int size = sizeof(FileIndex);
    int fileNum = node->size / size;
    int usedBlocks = node->size / BLOCKSIZE;
    /*没有物理块或者申请的物理块已用完，申请新的物理块*/
    if (fileNum % superBlock->fileIndexPerBlock == 0) {
        node->dirBlock[usedBlocks] = blockAlloc();
        if (node->dirBlock[usedBlocks] == -1)
            return;
        node->blockCount++;
    }
    /*将对应的目录项FileIndex初始化，并为其分配inode*/
    unsigned int index = fileNum % superBlock->fileIndexPerBlock;
    auto *fileIndex = (FileIndex *) getBlockAddr(node->dirBlock[usedBlocks]);
    strcpy(fileIndex[index].fileName, name);
    fileIndex[index].inode = inodeAlloc();
    if (fileIndex[index].inode == -1)
        return;
    initInode(fileIndex[index].inode, attribute);
    /*修改inode的信息*/
    node->size += size;
    node->ctime = node->mtime = time(nullptr);
    /*如果是目录，需要为该目录创建一个指向inode的目录项*/
    if (attribute) {
        addFatherFolder(&inode[fileIndex[index].inode]);
    }
}

/*显示当前路径*/
void pwd() {
    cout << path << endl;
}

/*调用磁盘管理层的exitSys函数，用来退出系统*/
void exit() {
    exitSys();
}

/*根据开始目录的节点和路径以及被截断的路径，尝试改变目录*/
void changePath(Inode *startInode, string tempPath) {
    Inode *node = startInode;
    unsigned int index;
    /*逐项判断是否有该文件*/
    for (auto &i : pathClip) {
        char *name = new char(i.length() + 1);
        strcpy(name, i.c_str());
        index = checkName(node, name);
        /*没有该文件或目录，输出错误信息并返回*/
        if (index == (unsigned int) -1) {
            printf("cd: No such file or directory!\n");
            return;
        }
        node = &inode[index];
        /*该节点不是目录，输出错误信息并返回*/
        if (!node->attribute) {
            printf("cd: %s is not directory!\n", name);
            return;
        }
        /*返回上一层目录，首先找到最后一个出现“/”的地方然会截断*/
        if (strcmp(name, "..") == 0) {
            int last = tempPath.find_last_of('/');
            tempPath = tempPath.substr(0, last);
        } else {
            if (tempPath != "/")/*非根目录*/
                tempPath += "/";
            tempPath += name;
        }
    }
    /*改变当前目录的inode和路径*/
    curInodeIndex = index;
    path = tempPath;
    if (path == "")
        path = "/";
}

/*改变当前目录*/
void cd() {
    /*命令参数大于或者小于2输出对应的错误信息*/
    if (commandLine.size() > 2) {
        printf("cd: too much arguments!\n");
        return;
    }
    if (commandLine.size() < 2) {
        printf("cd: too less arguments!\n");
        return;
    }
    /*将路径使用'/'作为分割符分割*/
    pathClip = split(commandLine[1], "/+");
    /*如果是绝对路径*/
    if (commandLine[1][0] == '/')
        changePath(&inode[superBlock->rootInode], "/");
    else /*相对路径*/
        changePath(&inode[curInodeIndex], path);
    pathClip.clear();
}

/*根据传进来的名字创建目录，名字参数可以有多个*/
void mkdir() {
    for (int i = 1; i < commandLine.size(); i++) {
        /*获取名字*/
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        /*查找是否有该文件*/
        if (checkName(&inode[curInodeIndex], name) != (unsigned int) -1) {
            printf("mkdir: Name %s is used!\n", name);
            continue;
        }
        Inode *node = &inode[curInodeIndex];
        /*调用函数生成该目录*/
        createNewFile(node, name, true);
    }
}

/*根据传进的物理块逐个检查其目录项，如果目录项对应的inode是目录，
 * 则再调用doRmidr函数，如果是文件则清空对应的物理块*/
void doRmdir(unsigned int blockNum) {
    auto *fileIndex = (FileIndex *) getBlockAddr(blockNum);
    for (int i = 0; i < superBlock->fileIndexPerBlock; i++) {
        if (fileIndex[i].fileName[0] != '\0') {
            unsigned int ino = fileIndex->inode;
            if (ino) {
                Inode *node = &inode[ino];
                memset(fileIndex, 0, sizeof(FileIndex));
                int j = 0;
                if (!node->attribute)
                    while (node->dirBlock[j] != -1 && j < 12) {
                        doRmdir(node->dirBlock[j]);
                        printf("block %d free\n", node->dirBlock[j]);
                        blockFree(node->dirBlock[j++]);
                        node->blockCount--;
                    }
                else
                    while (node->dirBlock[j] != -1 && j < 12) {
                        printf("block %d free\n", node->dirBlock[j]);
                        blockFree(node->dirBlock[j++]);
                        node->blockCount--;
                    }
                inodeFree(ino);
                memset(node, 0, sizeof(Inode));
            }
        }
    }
}

void rmdir() {
    for (int i = 1; i < commandLine.size(); i++) {
        /*获取文件名字然后查找是否存在并返回目录项*/
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        FileIndex *fileIndex = getFileIndex(name);
        if (!fileIndex) {
            printf("rm: No such directory!\n");
            continue;
        }
        unsigned int ino = fileIndex->inode;
        if (ino) {
            Inode *node = &inode[ino];
            /*如果该inode是文件*/
            if (node->attribute) {
                node->linkCount--;
                memset(fileIndex, 0, sizeof(FileIndex));
                int j = 0;
                /*清空该目录项，释放并清空该inode所属的空间，更新当前的inode时间*/
                while (node->dirBlock[j] != -1 && j < 12) {
                    doRmdir(node->dirBlock[j]);
                    blockFree(node->dirBlock[j++]);
                    node->blockCount--;
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
        /*获取文件名字然后查找是否存在并返回目录项*/
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        FileIndex *fileIndex = getFileIndex(name);
        if (!fileIndex) {
            printf("rm: No such file!\n");
            continue;
        }
        unsigned int ino = fileIndex->inode;
        if (ino) {
            Inode *node = &inode[ino];
            /*如果该inode是文件*/
            if (!node->attribute) {
                node->linkCount--;
                /*清空该目录项，释放并清空该inode所属的空间，更新当前的inode时间*/
                memset(fileIndex, 0, sizeof(FileIndex));
                int j;
                while (node->dirBlock[j] != -1 && j < 12) {
                    blockFree(node->dirBlock[j++]);
                    node->blockCount--;
                }
                inodeFree(ino);
                memset(node, 0, sizeof(Inode));
                inode[curInodeIndex].ctime = time(nullptr);
                inode[curInodeIndex].atime = time(nullptr);
                inode[curInodeIndex].mtime = time(nullptr);
            } else {
                printf("rm: failed to remove '%s': Not a file\n", name);
                continue;
            }
        } else {
            printf("rm: fail to remove '%s': No such file!\n", name);
            continue;
        }
    }
}

void touch() {
    for (int i = 1; i < commandLine.size(); i++) {
        /*获取名字*/
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        /*查看是否已经使用*/
        if (checkName(&inode[curInodeIndex], name) != (unsigned int) -1) {
            printf("touch: Name %s is used!\n", name);
            continue;
        }
        Inode *node = &inode[curInodeIndex];
        /*调用函数生成该文件*/
        createNewFile(node, name, false);
    }
}

void doRead(Inode *node) {
    int usedBlocks = node->size / BLOCKSIZE + 1;
    int offset = node->size % BLOCKSIZE;
    /*逐个遍历使用的物理块*/
    for (int i = 0; i < usedBlocks; i++) {
        char *addr = (char *) getBlockAddr(node->dirBlock[i]);
        /*逐个字符获取对应的值*/
        if (i == usedBlocks - 1) {
            for (int j = 0; j < offset; j++)
                printf("%c", *(addr + j));
        } else {
            for (int j = 0; j < BLOCKSIZE; j++)
                printf("%c", *(addr + j));
        }
    }
    printf("\033[33m#\033[0m\n");
}

void read() {
    /*命令参数大于或者小于2输出对应的错误信息*/
    if (commandLine.size() > 2) {
        printf("read: too much arguments!\n");
        return;
    }
    if (commandLine.size() < 2) {
        printf("read: too less arguments!\n");
        return;
    }
    /*获取名字*/
    char *name = new char(commandLine[1].length() + 1);
    strcpy(name, commandLine[1].c_str());
    unsigned int ino = checkName(&inode[curInodeIndex], name);
    /*获取对应的inode编号和inode节点*/
    if (ino != (unsigned int) -1) {
        Inode *node = &inode[ino];
        /*非目录*/
        if (!node->attribute) {
            open(name, 'r', node);  //打开文件
            doRead(node);
            close(name, node);      //关闭文件
        } else {
            printf("read: %s is a directory!\n", name);
            return;
        }
    } else {
        printf("read: no such file or directory!\n");
        return;
    }
}

/*释放传入的inode的物理块，然后调用doAppendWrite函数*/
void doReWrite(Inode *node) {
    int j = 0;
    while (node->dirBlock[j] != -1 && j < 12) {
        blockFree(node->dirBlock[j]);
        node->dirBlock[j] = -1;
        node->blockCount--;
        j = j + 1;
    }
    node->size = 0;
    doAppendWrite(node);
}

/*将输入的字符串逐个赋值给物理块相应的位置*/
void doAppendWrite(Inode *node) {
    int usedBlocks = node->size / BLOCKSIZE;    /*已使用的物理块*/
    int offset = node->size % BLOCKSIZE;        /*偏移地址*/
    string str;                                 /*输入的字符串*/
    printf("Please inter text: ");
    getline(cin, str);
    int length = str.length();                  /*输入的字符长度*/
    int complete = 0;                           /*已写入的字符长度*/
    char *line = new char(length + 1);
    strcpy(line, str.c_str());
    while (length) {                            /*当长度为0时退出循环*/
        /*offset为0代表没有物理块可以放数据，需要申请物理块*/
        if (offset == 0) {
            node->dirBlock[usedBlocks] = blockAlloc();
            if (node->dirBlock[usedBlocks] == -1)
                return;
            node->blockCount++;
        }
        /*获取物理块地址*/
        char *addr = (char *) getBlockAddr(node->dirBlock[usedBlocks]);
        /*剩余长度大于物理块内剩余空间*/
        if (length > (BLOCKSIZE - offset)) {
            for (int i = 0; i < (BLOCKSIZE - offset); i++)
                *(addr + offset + i) = *(line + complete + i);
            length -= BLOCKSIZE - offset;
            complete += BLOCKSIZE - offset;
            offset = 0;
            usedBlocks++;
            /*剩余长度小于物理块内剩余空间*/
        } else {
            for (int i = 0; i < length; i++)
                *(addr + i + offset) = *(line + i + complete);
            complete += length;
            length = 0;
        }
    }
    /*改变size*/
    node->size = node->size + complete;
}

void write() {
    /*命令参数大于或者小于3输出对应的错误信息*/
    if (commandLine.size() > 3) {
        printf("write: too much arguments!\n");
        return;
    }
    if (commandLine.size() < 3) {
        printf("write: too less arguments!\n");
        return;
    }
    /*获取参数和文件名字，然后查找该名字是否存在*/
    char *option = new char(commandLine[1].length() + 1);
    strcpy(option, commandLine[1].c_str());
    char *name = new char(commandLine[2].length() + 1);
    strcpy(name, commandLine[2].c_str());
    unsigned int ino = checkName(&inode[curInodeIndex], name);
    /*获取对应的inode编号和地址*/
    if (ino != (unsigned int) -1) {
        Inode *node = &inode[ino];
        if (!node->attribute) {
            /*追加*/
            if (strcmp(option, "-a") == 0) {
                open(name, 'a', node);
                doAppendWrite(node);
                close(name, node);
                /*重写*/
            } else if (strcmp(option, "-c") == 0) {
                open(name, 'c', node);
                doReWrite(node);
                close(name, node);
            } else {
                printf("write: %s is error option!\n", option);
                return;
            }
        } else {
            printf("write: %s is a directory!\n", name);
            return;
        }
    } else {
        printf("write: no such file or directory!\n");
        return;
    }
}

/*将该文件加入到已打开的文件vector中和读取文件控制*/
void open(char *name, char op, Inode *node) {
    File file;
    strcpy(file.fileName, name);
    file.op = op;
    file.inode = node;
    file.pid = getpid();
    sem_wait(&mutex);
    switch (op) {
        /*读取文件*/
        case 'r':
            /*读者优先策略*/
            sem_wait(&node->readMutex);
            if (node->readCount == 0)
                sem_wait(&node->writeMutex);
            node->readCount++;
            sem_post(&node->readMutex);
            break;
            /*写文件，等待writeMutex信号量*/
        case 'a':
        case 'c':
            sem_wait(&node->writeMutex);
        default:
            break;
    }
    /*加入到已打开的文件结果*/
    filesList.push_back(file);
    sem_post(&mutex);
}

/*将该文件从已打开的文件vector中删除读取文件控制*/
void close(char *name, Inode *node) {
    pid_t pid = getpid();
    vector<File>::iterator it;
    sem_wait(&mutex);
    /*遍历打开的文件*/
    for (it = filesList.begin(); it < filesList.end(); it++) {
        if (strcmp(name, it->fileName) == 0 && it->pid == pid) {
            switch (it->op) {
                /*读*/
                case 'r':/*读者优先*/
                    sem_wait(&node->readMutex);
                    node->readCount--;
                    if (node->readCount == 0)
                        sem_post(&node->writeMutex);
                    sem_post(&node->readMutex);
                    /*写*/
                case 'a':
                case 'c':
                    sem_post(&node->writeMutex);
                default:
                    break;
            }
            filesList.erase(it);
        }
    }
    sem_post(&mutex);
}

/*根据传进来的源文件和目的名字改变源文件的名字为目的名字*/
void rename() {
    /*命令参数大于或者小于3输出对应的错误信息*/
    if (commandLine.size() > 3) {
        printf("rename: too much arguments!\n");
        return;
    }
    if (commandLine.size() < 3) {
        printf("rename: too less arguments!\n");
        return;
    }
    /*获取源文件的名字和目的名字*/
    char *name1 = new char(commandLine[1].length() + 1);
    strcpy(name1, commandLine[1].c_str());
    char *name2 = new char(commandLine[2].length() + 1);
    strcpy(name2, commandLine[2].c_str());
    /*获取原名字对应的目录项*/
    FileIndex *fileIndex = getFileIndex(name1);
    if (fileIndex) {
        strcpy(fileIndex->fileName, name2);
    } else {/*找不到该文件*/
        printf("rename: no such file or directory!\n");
        return;
    }

}

int fileCount;

/*输出inode节点和FileIndex节点的信息*/
void printInode(Inode *node, FileIndex *fileIndex) {
    if (!fileIndex->fileName[0])
        return;
    if (node->attribute)
        printf(" dir ");
    else
        printf("file ");
    printf("%4lu  ", node->size);
    printf("%4u  ", fileIndex->inode);
    printf("%4u  ", node->blockCount);
    if (node->attribute)
        printf("\033[33m%4s\033[0m", fileIndex->fileName);
    else
        printf("%4s", fileIndex->fileName);
    printf("\n");
    fileCount++;
}

void ls() {
    int size = sizeof(FileIndex);
    Inode *node = &inode[curInodeIndex];
    /*共有多少个目录项*/
    int fileNum = node->size / size;
    fileCount = 0;
    printf("\033[32mtype  size  inode  block  name\033[0m\n");
    /*使用了多少个物理块*/
    int usedBlocks = node->size / BLOCKSIZE + 1;
    for (int i = 0; i < usedBlocks; i++) {
        auto *fileIndex = (FileIndex *) getBlockAddr(node->dirBlock[i]);
        /*最后一个物理块*/
        if (i == usedBlocks - 1) {
            for (int j = 0; j < fileNum % superBlock->fileIndexPerBlock; j++) {
                Inode *tInode = &inode[fileIndex[j].inode];
                /*输出该节点和该目录项对应的信息*/
                printInode(tInode, &fileIndex[j]);
            }
        } else {
            for (int j = 0; j < superBlock->fileIndexPerBlock; j++) {
                Inode *tInode = &inode[fileIndex[j].inode];
                /*输出该节点和该目录项对应的信息*/
                printInode(tInode, &fileIndex[j]);
            }
        }
    }
    printf("Total: \033[32m%d\033[0m files\n", fileCount);
}
