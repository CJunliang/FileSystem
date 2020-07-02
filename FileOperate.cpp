//
// Created by liang on 2020/6/11.
//
#include <iostream>
#include <vector>
#include "FileOperate.h"
#include "DiskManager.h"

using namespace std;

void fileInit() {
    allocMemory();
    printf("%d\n",shmSt->isInit);
    if (!shmSt->isInit)
        diskInit();
    else load();
    path = "/";
}

void pwd() {
}

void exit() {}

void cd() {}

void mkdir() {}

void rmdir() {}

void rm() {}

void touch() {}

void read() {}

void write() {}

void open() {}

void ls() {}
