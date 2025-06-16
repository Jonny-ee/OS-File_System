#ifndef FILE_H
#define FILE_H

#define BLOCK_SIZE 4096 // 一个数据块 4KB
#define INODE_SIZE 128 // 一个inode 128B
#define NUM_BLOCKS 256 // 数据块一共256个
#define NUM_INODES 256 // 空闲inode号有256个
#define NAME_LENGTH 252 // 名字长度
#define DIRECT_PTR_COUNT 12 // 直接指针的个数
#define ROOT 0
#define IS_FILE true
#define IS_DIRECTORY false

#include <atomic>  // 原子操作
#include <string>
#include <vector>
#include <bitset>
#include <array>
#include "base64.h"
//json的文件
#include "json.hpp"
using json = nlohmann::json;

// 文件类型（高位）
constexpr uint16_t FILE_TYPE_MASK   = 0xF000; // 高4位掩码
constexpr uint16_t TYPE_FILE        = 0x1000; // 普通文件
constexpr uint16_t TYPE_DIR         = 0x2000; // 文件目录

// 权限位（低 12 位）
constexpr uint16_t PERM_USER_READ   = 0x0100; // 用户读
constexpr uint16_t PERM_USER_WRITE  = 0x0080; // 用户写
constexpr uint16_t PERM_USER_EXEC   = 0x0040; // 用户执行

constexpr uint16_t PERM_GROUP_READ  = 0x0020; // 组读
constexpr uint16_t PERM_GROUP_WRITE = 0x0010; // 组写
constexpr uint16_t PERM_GROUP_EXEC  = 0x0008; // 组执行

constexpr uint16_t PERM_OTH_READ    = 0x0004; // 其他读
constexpr uint16_t PERM_OTH_WRITE   = 0x0002; // 其他写
constexpr uint16_t PERM_OTH_EXEC    = 0x0001; // 其他执行

//默认普通文件权限 rw-r--r--
#define DEFAULT_FILE_PERM (TYPE_FILE | PERM_USER_READ | PERM_USER_WRITE | PERM_GROUP_READ | PERM_OTH_READ)
//默认文件夹权限 rwxr-xr-x
#define DEFAULT_DIR_PERM (TYPE_DIR | PERM_USER_READ | PERM_USER_WRITE | PERM_USER_EXEC | PERM_GROUP_READ | PERM_GROUP_EXEC | PERM_OTH_READ | PERM_OTH_EXEC)


//超级块，占20B
struct Super_Block {
private:
    int num_inode_table; // 总inode数量
    int num_block; // 总数据块数量
    std::atomic<int>  free_inode; // 目前可用的inode数量
    std::atomic<int>  free_data_block; // 目前可用的数据块数量
    int inode_table_size; // inode大小
    int data_block_size; // 数据块大小
public:
    explicit Super_Block(const json &j); // 有本地文件构造方法
    Super_Block(); // 无参构造
    void init_sb(); // 如果有本地文件，则按照文件初始化
    int get_free_inode() const; // 获得目前可用的inode数量
    int get_free_data_block() const; // 获得目前可用的数据块数量
    bool use_data_block(); // 占用数据块，如果没有可用的数据块则返回false
    bool use_inode_table(); // 占用inode号，如果没有可用的数据块则返回false
    void release_data_block(); // 释放数据块
    void release_inode_table(); // 释放inode号
    json save_super_block(); // 保存超级块的信息
};

// 数据块和inode号正好都是256个，使用同一个类，占32B
struct Bitmap {
private:
    std::bitset<NUM_BLOCKS> bit_map; // 256位的比特集合
public:
    explicit Bitmap(const json &j,const std::string& n); // 有本地文件构造方法
    explicit Bitmap(); // 无参构造
    bool use_bitmap(int i_id); // 将目标bit置为1，如果已经是1则报错
    void release_bitmap(uint32_t i_id);// 释放该数据块
    [[nodiscard]] int find_free_bit() const;    //从2号开始遍历，找到空的序号，0和1为保留号和root
    void reset_bitmap(); // 重置bitmap
    [[nodiscard]] std::string save_bitmap() const; // 保存bitmap信息
};
static_assert(sizeof(Bitmap) == 32, "Bitmap size mismatch!");


// 定义inode 固定占128B
struct Inode {
private:
    uint16_t mode;                             // 文件类型 & 权限 高4位为文件
    uint32_t size;                             // 文件字节数
    uint32_t uid;                              // 所有者用户 ID
    uint64_t create_time;                      // 创建时间
    uint64_t modification_time;                // 最后一次修改时间
    std::array<int,12> direct_ptrs{};          // 直接指针（最多12个）
    uint32_t indirect_ptr;                     // 一级间接指针
    uint32_t double_indirect_ptr;              // 二级间接指针
    uint64_t padding[5]{};                     // 占位，未来拓展
public:
    explicit Inode(const json& j); // 有参构造
    explicit Inode(); // 无参构造
    void initial_inode(u_int32_t u,bool is_file); // 创建文件时，只用所有者id，并且自动获得当前时间和默认文件权限
    [[nodiscard]] std::string get_time(const std::string& type) const;// 返回年月日时分秒的string，格式为 "2014-12-01 14:12:60"，可以选择创建时间或者修改时间
    [[nodiscard]] uint64_t get_create_time() const; // 获取创建时的时间，用于排序
    bool update_mode(std::string &m);// 更新权限
    void update_modification_time(); // 更新修改时间
    void update_size(uint32_t &s,bool is_directory); // 更新文件大小
    [[nodiscard]] uint32_t get_size() const;// 获得当前文件大小
    void update_direct_ptr(int block_num); // 更新直接指针
    void update_indirect_ptr(int block_num); //更新一级间接指针
    void update_double_indirect_ptr(int block_num); // 更新二级间接指针
    [[nodiscard]] uint32_t get_direct_ptr(uint32_t ptr_num) const; // 获得直接指针所指向的数据块
    [[nodiscard]] uint32_t get_indirect_ptr() const; // 获得一级直接指针指向的index表
    [[nodiscard]] uint32_t get_double_indirect_ptr() const;// 获得二级直接指针指向的index表
    [[nodiscard]] bool get_type() const; // 获取该文件的类型
    [[nodiscard]] bool get_uid() const; //获取该文件的uid
    [[nodiscard]] uint16_t get_mode() const; // 返回文件权限-2字节整数
    [[nodiscard]] std::string get_mode(bool be_string) const; // 返回文件权限-string
    void reset_ptr(int ptr_type,int pos=0); // 更改指针
    void reset_inode(); // 重置inode
    json save_inode(); //保存inode节点，返回json文件
};
static_assert(sizeof(Inode) == INODE_SIZE, "Inode size mismatch!");


//文件目录 固定占256B，省略了目录项长度和文件名长度，采用默认长度的方式简化逻辑
struct Directory_Entry {
private:
    uint32_t inode=0;      // inode号
    char name[NAME_LENGTH]{};  // 文件名最大长度
public:
    explicit Directory_Entry()=default;
    void init_dir(uint32_t i,const char* n); // 初始化目录
    [[nodiscard]] uint32_t get_inode() const; // 获取inode
    [[nodiscard]] char* get_name(); // 获取名字
    void delete_file();// 删除文件就是将inode置为0，除此之外无法改变inode
    void update_name(char* n); // 重命名
};
static_assert(sizeof(Directory_Entry) == 256, "DirectoryEntry size mismatch!");


// 数据块 固定为4KB，可被用作数据块，索引块和目录块
struct Data_Block {
private:
    char block[BLOCK_SIZE]{};
public:
    Data_Block(const json& j,int pos ); // 有参构造
    Data_Block(); //无参构造
    void add_dir(uint32_t target_inode,const char* target_name); // 为目录块写入目录,当inode！=0的时候，才可以写入
    bool is_full_of_directory(); // 判断当前block是否已经填满目录
    bool is_full_of_index(); // 判断当前block是否已经填满index
    bool is_empty_of_directory(); // 判断当前目录block是否为空
    bool is_empty_of_index(); // 判断当前index块是否为空
    void add_index(uint32_t target_index); // 为索引块添加索引，当指针为0的时候，才可以写入
    uint32_t get_ptr_of_index(int num); // 从index表中通过位置获取指针
    uint32_t get_inode(const char* target_name); // 如果数据块为目录块，则提供名字查inode的服务
    uint32_t get_inode(uint32_t pos); // 或者通过位置来获取inode
    void delete_file(uint32_t pos); //删除指定位置的文件
    char* get_name(uint32_t pos); // 如果数据块为目录块，则提供查名服务
    void init_directory_block();// 初始化当前空闲块为目录项块，将所有inode都置为0
    void init_index_block();// 初始化当前空闲块为索引块，将所有指针改为null
    void reset_block(); // 初始化该数据块
    bool has_name(const char* target_name);// 判断是否已经有同名文件
    [[nodiscard]] std::vector<std::string> list_entries(); // 用于ls操作
    char* get_block(); // 读取该数据块
    std::string save_data_block(); // 保存数据块内的信息
};
static_assert(sizeof(Data_Block) == 4096, "Data_Block size mismatch!");

#endif //FILE_H
