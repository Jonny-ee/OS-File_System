#ifndef FILE_TERMINAL_H
#define FILE_TERMINAL_H
#include <sstream>
#include <ncurses.h>
#include "file_manager.h"
class File_Terminal {
private:
    uint32_t inode; //目前所在文件夹的inode
    File_Manager fm;
    std::vector<std::string> path_name;//存储路径
    std::vector<uint32_t> path_inode;
public:
    File_Terminal() {
        inode=ROOT; // 默认初始所在位置是ROOT文件夹
        path_name.emplace_back("\\");
        path_inode.emplace_back(ROOT);
        command();
    }
    void command(); //终端
    void cd(const std::string& name); // 改变当前路径
    void cat(const std::string& name); // 查看文件内容
    void vi(const std::string& name); // 查看创建文件
    void mkdir(const std::string& name); // 创建文件夹
    void write_to_file(uint32_t new_file,const std::string& text);

};



#endif //FILE_TERMINAL_H
