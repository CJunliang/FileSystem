//
// Created by liang on 2020/6/11.
//

#ifndef FILESYSTEM_FILEOPERATE_H
#define FILESYSTEM_FILEOPERATE_H

#include "DiskManager.h"

/*已打开的文件*/
typedef struct FILEOPEN {
    char fileName[28];  /*文件名字*/
    char op;            /*操作类型*/
    Inode *inode;       /*inode节点*/
    pid_t pid;          /*进程pid*/
} File;
/*文件*/
static std::vector<File> filesList;
/*当前路径*/
static std::string path;
/*存储命令参数*/
extern std::vector<std::string> commandLine;

/*初始化磁盘和当前目录*/
void fileInit();

/*显示当前路径*/
void pwd();

/*调用磁盘管理层的exitSys函数，用来退出系统。*/
void exit();
/*改变当前目录*/
void cd();

void mkdir();

void rmdir();

void rm();

void touch();

void read();

void write();

void ls();
/*根据传进来的源文件和目的名字改变源文件的名字为目的名字*/
void rename();

/*根据传进来的inode节点、名字name和文件属性，根据该文件属性
 * 创建一个在该inode节点下的名字为name的目录或者文件*/
void createNewFile(Inode *node, char *name, bool attribute);

/*根据传入的string和分割符切成string类型的块存入到vector中*/
std::vector<std::string> split(const std::string &str, const std::string &delimiter);

void open(char *name, char op, Inode *inode);

void close(char *name, Inode *inode);

void doAppendWrite(Inode *node);

void doReWrite(Inode *node);

#endif //FILESYSTEM_FILEOPERATE_H
