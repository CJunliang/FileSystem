//
// Created by liang on 2020/6/11.
//

#ifndef FILESYSTEM_FILEOPERATE_H
#define FILESYSTEM_FILEOPERATE_H

#include "DiskManager.h"

typedef struct FILEOPEN {
    char fileName[28];
    char op;
    Inode *inode;
    pid_t pid;
} File;
/*文件*/
static std::vector<File> files;
/*当前路径*/
static std::string path;
/*存储命令参数*/
static std::vector<std::string> vec;

void fileInit();

void pwd();

void exit();

void cd();

void mkdir();

void rmdir();

void rm();

void touch();

void read();

void write();

void open();

void ls();

#endif //FILESYSTEM_FILEOPERATE_H
