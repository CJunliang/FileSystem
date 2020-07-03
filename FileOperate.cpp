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
    node->blockCount++;
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
    if (fileNum % superBlock->fileIndexPerBlock == 0) {
        node->dirBlock[usedBlocks] = blockAlloc();
        node->blockCount++;
    }
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

void exit() {
    exitSys();
}

void changePath(Inode *startInode, string tempPath) {
    Inode *node = startInode;
    unsigned int index;
    for (auto &i : pathClip) {
        char *name = new char(i.length() + 1);
        strcpy(name, i.c_str());
        index = checkName(node, name);
        if (index == (unsigned int) -1) {
            printf("cd: No such file or directory!\n");
            return;
        }
        node = &inode[index];
        if (strcmp(name, "..") == 0) {
            int last = tempPath.find_last_of('/');
            tempPath = tempPath.substr(0, last);
        } else {
            if (path != "/")
                tempPath += "/";
            tempPath += name;
        }
    }
    curInodeIndex = index;
    path = tempPath;
    if (path == "")
        path = "/";
}

void cd() {
    if (commandLine.size() > 2) {
        printf("cd: too much arguments!\n");
        return;
    }
    pathClip = split(commandLine[1], "/+");
    if (commandLine[1][0] == '/') {
        changePath(&inode[superBlock->rootInode], "/");
    } else {
        changePath(&inode[curInodeIndex], path);
    }
    pathClip.clear();
}

void mkdir() {
    for (int i = 1; i < commandLine.size(); i++) {
        char *name = new char(commandLine[i].length() + 1);
        strcpy(name, commandLine[i].c_str());
        if (checkName(&inode[curInodeIndex], name) != (unsigned int) -1) {
            printf("mkdir: Name %s is used!\n", name);
            return;
        }
        Inode *node = &inode[curInodeIndex];
        createNewFile(node, name, true);
    }
}

void doRmdir(unsigned int blockNum) {
    auto *fileIndex = (FileIndex *) getBlockAddr(blockNum);
    for (int i = 0; i < superBlock->fileIndexPerBlock; i++) {
        if (fileIndex[i].fileName[0] != '\0') {
            unsigned int ino = fileIndex->inode;
            if (ino) {
                Inode *node = &inode[ino];
                memset(fileIndex, 0, sizeof(FileIndex));
                int j = 0;
                while (node->dirBlock[j] != -1 && j < 12) {
                    doRmdir(node->dirBlock[j]);
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
                    doRmdir(node->dirBlock[j]);
                    printf("block %d free\n", node->dirBlock[j]);
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
                    node->blockCount--;
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
        if (checkName(&inode[curInodeIndex], name) != (unsigned int) -1) {
            printf("touch: Name %s is used!\n", name);
            return;
        }
        Inode *node = &inode[curInodeIndex];
        createNewFile(node, name, false);
    }
}

void doRead(Inode *node) {
    int usedBlocks = node->size / BLOCKSIZE + 1;
    int offset = node->size % BLOCKSIZE;
    for (int i = 0; i < usedBlocks; i++) {
        char *addr = (char *) getBlockAddr(node->dirBlock[i]);
        if (i == usedBlocks - 1) {
            for (int j = 0; j < offset; j++)
                printf("%c", *(addr + j));
        } else {
            for (int j = 0; j < BLOCKSIZE; j++)
                printf("%c", *(addr + j));
        }
    }
    printf("#\n");
}

void read() {
    if (commandLine.size() > 2) {
        printf("read: too much arguments!\n");
        return;
    }
    if (commandLine.size() < 2) {
        printf("read: too less arguments!\n");
        return;
    }
    char *name = new char(commandLine[1].length() + 1);
    strcpy(name, commandLine[1].c_str());
    unsigned int ino = checkName(&inode[curInodeIndex], name);
    if (ino != (unsigned int) -1) {
        Inode *node = &inode[ino];
        if (!node->attribute) {
            open(name, 'r', node);
            doRead(node);
            close(name);
        } else {
            printf("read: %s is a directory!\n", name);
            return;
        }
    } else {
        printf("read: no such file or directory!\n");
        return;
    }
}

void doReWrite(Inode *node) {
    int j = 0;
    while (node->dirBlock[j] != -1 && j < 12) {
        printf("block %d free\n", node->dirBlock[j]);
        blockFree(node->dirBlock[j++]);
        node->blockCount--;
    }
    node->size = 0;
    doAppendWrite(node);
}

void doAppendWrite(Inode *node) {
    int usedBlocks = node->size / BLOCKSIZE;
    int offset = node->size % BLOCKSIZE;
    string str;
    getline(cin, str);
    int length = str.length();
    int complete = 0;
    char *line = new char(length + 1);
    strcpy(line, str.c_str());
    while (length) {
        if (offset == 0) {
            node->dirBlock[usedBlocks] = blockAlloc();
            node->blockCount++;
        }
        char *addr = (char *) getBlockAddr(node->dirBlock[usedBlocks]);
        if (length > (BLOCKSIZE - offset)) {
            for (int i = 0; i < (BLOCKSIZE - offset); i++)
                *(addr + offset + i) = *(line + complete + i);
            length -= BLOCKSIZE - offset;
            complete += BLOCKSIZE - offset;
            offset = 0;
            usedBlocks++;
        } else {
            for (int i = 0; i < length; i++)
                *(addr + i + offset) = *(line + i + complete);
            complete += length;
            length = 0;
        }
    }
    node->size = node->size + complete;
}

void write() {
    if (commandLine.size() > 3) {
        printf("write: too much arguments!\n");
        return;
    }
    if (commandLine.size() < 3) {
        printf("write: too less arguments!\n");
        return;
    }
    char *name = new char(commandLine[1].length() + 1);
    strcpy(name, commandLine[1].c_str());
    char *option = new char(commandLine[2].length() + 1);
    strcpy(option, commandLine[2].c_str());
    unsigned int ino = checkName(&inode[curInodeIndex], name);
    if (ino != (unsigned int) -1) {
        Inode *node = &inode[ino];
        if (!node->attribute) {
            /*追加*/
            if (strcmp(option, "-a") == 0) {
                open(name, 'a', node);
                doAppendWrite(node);
                close(name);
            } else if (strcmp(option, "-c") == 0) {/*重写*/
                open(name, 'c', node);
                doReWrite(node);
                close(name);
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

void open(char *name, char op, Inode *inode) {
    File file;
    strcpy(file.fileName, name);
    file.op = op;
    file.inode = inode;
    file.pid = getpid();
    filesList.push_back(file);
}

void close(char *name) {
    pid_t pid = getpid();
    vector<File>::iterator it;
    for (it = filesList.begin(); it < filesList.end(); it++) {
        if (strcmp(name, it->fileName) == 0 && it->pid == pid) {
            filesList.erase(it);
        }
    }
}

void rename() {
    if (commandLine.size() > 3) {
        printf("rename: too much arguments!\n");
        return;
    }
    if (commandLine.size() < 3) {
        printf("rename: too less arguments!\n");
        return;
    }
    char *name1 = new char(commandLine[1].length() + 1);
    strcpy(name1, commandLine[1].c_str());
    char *name2 = new char(commandLine[2].length() + 1);
    strcpy(name2, commandLine[2].c_str());
    FileIndex *fileIndex = getFileIndex(name1);
    if (fileIndex) {
        strcpy(fileIndex->fileName, name2);
    } else {
        printf("rename: no such file or directory!\n");
        return;
    }

}

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
//    printf("%s", ctime((time_t *) &node->atime));
//    printf("%s", asctime(localtime((time_t*) &node->atime)));
    if (node->attribute)
        printf("\033[33m%4s\033[0m", fileIndex->fileName);
    else
        printf("%4s", fileIndex->fileName);
    printf("\n");
}

void ls() {
    int size = sizeof(FileIndex);
    Inode *node = &inode[curInodeIndex];
    int fileNum = node->size / size;
    printf("\033[32mtype  size  inode  block  name\033[0m\n");
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
