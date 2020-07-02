//
// Created by liang on 2020/6/11.
//
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "FileManager.h"
#include "FileOperate.h"

using namespace std;

CommandEnum lookUp(const string &str) {
    for (int i = 0; i < MAXCOMMAND; i++)
        if (!str.compare(Command[i].str))
            return Command[i].command;
    return ERROR;
}

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

int main() {
    fileInit();
    bool flag = true;
    CommandEnum command;
    string line;
    while (flag) {
        getline(cin, line);
        vec = split(line, " ");
        command = lookUp(vec[0]);
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
            case OPEN:
                open();
                break;
            case READ:
                read();
                break;
            case WRITE:
                write();
                break;
            case ERROR:
            default:
                printf("\033[31mNo such command!\033[0m\n");
                break;
        }
        vec.clear();
    }
}