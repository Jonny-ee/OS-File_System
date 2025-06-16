#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include <iostream>
#include <ostream>
#include "file.h"


class File_Manager {
private:
    std::unique_ptr<Super_Block> sb; // 超级块
    std::unique_ptr<Bitmap> block_bitmap; // 256位的数据块位图
    std::unique_ptr<Bitmap> inode_bitmap; // 256位的inode位图
    std::unique_ptr<Inode> inode_table[NUM_INODES]; // 256个inode节点（简化逻辑）
    std::unique_ptr<Data_Block> data_blocks[NUM_BLOCKS]; // 256个数据块表
public:
    File_Manager()=default;
    void load_data(const json& j,bool file_exsist); // 载入初始化数据
    void init_file_manager(); // 初始化文件系统
    // 创建文件夹，先为文件夹分配inode，再分配一个空闲块初始化为目录项，将空闲块与新文件夹的inode绑定，将新文件写入父文件夹的目录项中
    uint32_t create_new(uint32_t parent_inode,u_int32_t u,const char* n,bool type);
    void free_blocks(uint32_t inode_id); // 释放文件所占用的全部数据块
    void fm_delete(uint32_t parent_inode, const char* n);// “”表示全部删除，文件夹还要负责级联删除
    void list_directory(uint32_t parent_inode,bool show_more);// 显示文件夹里的文件
    std::string open_file(uint32_t target_inode);// 打开文件，输出块内的消息
    uint32_t find_inode(uint32_t parent_inode,const char* n,bool type); // 改变当前文件
    uint32_t write_file(uint32_t inode_id,const std::string& content); // 将数据写入数据块
    uint32_t get_file_size(uint32_t inode_id); // 获取当前文件的大小
    void update_directory_size(uint32_t inode_id,uint32_t new_size);  //更新文件夹的大小
    json save_data(); // 保存文件系统的信息
};

#endif //FILE_MANAGER_H
