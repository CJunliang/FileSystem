//
// Created by liang on 2020/6/11.
//

#ifndef FILESYSTEM_FILEMANAGER_H
#define FILESYSTEM_FILEMANAGER_H

#include "FileOperate.h"

#define MAXCOMMAND 13

typedef enum {
    CD, MKDIR, RMDIR, TOUCH, RM, PWD, LS, EXIT, OPEN, READ, WRITE, CLOSE, ERROR, RENAME
} CommandEnum;

struct {
    std::string str;
    CommandEnum command;
} Command[MAXCOMMAND] = {{"cd",     CD},
                         {"ls",     LS},
                         {"mkdir",  MKDIR},
                         {"rmdir",  RMDIR},
                         {"touch",  TOUCH},
                         {"rm",     RM},
                         {"pwd",    PWD},
                         {"exit",   EXIT},
                         {"open",   OPEN},
                         {"read",   READ},
                         {"write",  WRITE},
                         {"close",  CLOSE},
                         {"rename", RENAME}};

CommandEnum lookUp(const std::string &str);

#endif //FILESYSTEM_FILEMANAGER_H
