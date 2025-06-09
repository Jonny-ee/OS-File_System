#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include <iostream>
#include <ostream>

#include "file.h"

#define ROOT 0
#define FILE true
#define DIRECTORY false
class File_Manager {
private:
    Super_Block sb; // 超级块
    Bitmap block_bitmap; // 256位的数据块位图
    Bitmap inode_bitmap; // 256位的inode位图
    Inode inode_table[NUM_INODES]{}; // 256个inode节点（简化逻辑）
    Data_Block data_blocks[NUM_BLOCKS]{}; // 256个数据块表

public:
    File_Manager();// 格式化文件管理系统
    void init_file_manager();
    // 创建文件夹，先为文件夹分配inode，再分配一个空闲块初始化为目录项，将空闲块与新文件夹的inode绑定，将新文件写入父文件夹的目录项中
    uint32_t create_new(uint32_t parent_inode,u_int32_t u,const char* n,bool type);
    void free_blocks(uint32_t inode_id);
    void fm_delete(uint32_t parent_inode, const char* n);// “”表示全部删除，文件夹还要负责级联删除
    void list_directory(uint32_t parent_inode);// 显示文件夹里的文件
    std::string open_file(uint32_t target_inode);// 打开文件，输出块内的消息
    uint32_t find_inode(uint32_t parent_inode,const char* n,bool type); // 改变当前文件
    void write_file(uint32_t inode_id,const std::string& content);
    uint32_t get_file_size(uint32_t inode_id);
};

#endif //FILE_MANAGER_H
