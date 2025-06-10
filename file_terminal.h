#ifndef FILE_TERMINAL_H
#define FILE_TERMINAL_H
#include <sstream>
#include <ncurses.h>
#include <fstream>
#include "file_manager.h"
class File_Terminal {
private:
    uint32_t inode; //目前所在文件夹的inode
    File_Manager fm;
    std::vector<std::string> path_name;//存储路径
    std::vector<uint32_t> path_inode;
    void save_to_file();
    void load_from_file();
public:
    File_Terminal();
    void command(); //终端
    void cd(const std::string& name); // 改变当前路径
    void cat(const std::string& name); // 查看文件内容
    void vi(const std::string& name); // 查看创建文件
    void mkdir(const std::string& name); // 创建文件夹
    uint32_t write_to_file(uint32_t new_file,const std::string& text);
};



#endif //FILE_TERMINAL_H
