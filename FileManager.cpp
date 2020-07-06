//
// Created by liang on 2020/6/11.
//
#include <iostream>
#include <string>
#include <vector>
#include "FileManager.h"

using namespace std;

CommandEnum lookUp(const string &str) {
    for (auto &i : Command)
        if (str == i.str)
            return i.command;
    return ERROR;
}

int main() {
    /*初始化*/
    fileInit();
    bool flag = true;
    CommandEnum command;
    string line;
    while (flag) {
        printf(">>> ");
        /*读取命令并切割*/
        getline(cin, line);
        commandLine = split(line, " ");
        /*寻找切割除的第一个字符串对应的命令*/
        command = lookUp(commandLine[0]);
        switch (command) {
            case EXIT:
                exit();
                flag = false;
                break;
            case CD:
                cd();
                break;
            case MKDIR:
                mkdir();
                break;
            case RMDIR:
                rmdir();
                break;
            case TOUCH:
                touch();
                break;
            case RM:
                rm();
                break;
            case PWD:
                pwd();
                break;
            case LS:
                ls();
                break;
            case READ:
                read();
                break;
            case WRITE:
                write();
                break;
            case RENAME:
                rename();
                break;
            case ERROR:
            default:
                printf("\033[31mNo such command!\033[0m\n");
                break;
        }
        commandLine.clear();
    }
}