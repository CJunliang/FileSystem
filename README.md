# FileSystem
## 使用方法
编译  `make`  
运行  `./fileOS`  
清理文件  `make clean`
##  实现命令
创建目录mkdir `mkdir <dirName1> <dirName2> ...`  
删除目录rmdir  `rmdir <dirName1> <dirName2> ...`  
修改名称rename  `rename <sourceFileName> <targetFileName>`   
创建文件touch   `touch <fileName1> <fileName2> ...`  
删除文件rm   `rm <fileName1> <fileName2> ...`  
读文件read   `read <fileName>`    
写文件write    `write <option> <fileName>`, `<option>`包括重写`-c`和追加`-a`   
查看文件系统目录结构ls    `ls`  
改变当前目录cd    `cd <dirName>`