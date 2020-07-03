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
    fileInit();
    bool flag = true;
    CommandEnum command;
    string line;
    while (flag) {
        printf(">>>");
        getline(cin, line);
        commandLine = split(line, " ");
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
/*            case OPEN:
                open();
                break;*/
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