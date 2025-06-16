#ifndef FILE_TERMINAL_H
#define FILE_TERMINAL_H
#include <sstream>
#include <ncurses.h>
#include <fstream>
#include "file_manager.h"
class File_Terminal {
private:
    uint32_t inode; //目前所在文件夹的inode
    File_Manager fm; //
    std::vector<std::string> path_name;//存储路径-文件名
    std::vector<uint32_t> path_inode; //存储路径-inode
    void save_to_file(); // 保存文件信息
    void load_from_file(); //获取本地文件数据
public:
    File_Terminal(); //
    void command(); // 终端控制
    void cd(const std::string& name); // 改变当前路径
    void cat(const std::string& name); // 查看文件内容
    void vi(const std::string& name); // 查看创建文件
    void mkdir(const std::string& name); // 创建文件夹
    void ls(const std::string& name); // 展示文件夹下的各个文件名
    void reset(); //格式化系统
    uint32_t write_to_file(uint32_t new_file,const std::string& text); // 写入数据
};



#endif //FILE_TERMINAL_H
