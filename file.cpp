#include "file.h"

Super_Block::Super_Block() {
    num_inode_table = NUM_INODES;//8个表来存储inode，一个表可以写入32个inode，8个表则一共可以写入256条
    num_block = NUM_BLOCKS; //数据块一共256个
    free_inode = NUM_INODES; //初始时，空闲inode号有256个
    free_data_block = NUM_BLOCKS; //初始时，空闲数据块有256个
    inode_table_size = INODE_SIZE; //一个inode占128B
    data_block_size = BLOCK_SIZE; //数据块4KB
}
void Super_Block::init_sb() {
    num_inode_table = NUM_INODES;//8个表来存储inode，一个表可以写入32个inode，8个表则一共可以写入256条
    num_block = NUM_BLOCKS; //数据块一共256个
    free_inode = NUM_INODES; //初始时，空闲inode号有256个
    free_data_block = NUM_BLOCKS; //初始时，空闲数据块有256个
    inode_table_size = INODE_SIZE; //一个inode占128B
    data_block_size = BLOCK_SIZE; //数据块4KB
}
bool Super_Block::use_data_block() {
    if (free_data_block == 0)
        return false;
    free_data_block.fetch_sub(1);
    return true;
}
void Super_Block::release_data_block() {
    free_data_block.fetch_add(1);
}
int Super_Block::get_free_data_block() const {
    return free_data_block.load();
}
int Super_Block::get_free_inode() const {
    return free_inode.load();
}

Bitmap::Bitmap() {
    bit_map.reset();
}
bool Bitmap::use_bitmap(int i_id) {
    if (i_id < 0 || i_id >= NUM_BLOCKS)
        return false;
    if (bit_map.test(i_id))
        return false;
    bit_map.set(i_id);
    return true;
}
void Bitmap::release_bitmap(uint32_t i_id) {
    bit_map.reset(i_id);
}
[[nodiscard]] int Bitmap::find_free_bit() const {
    for (int i = 1; i < NUM_BLOCKS; i++) {
        if (bit_map[i]==false)
            return i;
    }
    return -1;
}
Inode::Inode() {
    mode = 0;
    size = 0;
    uid  = 0;
    create_time = 0;
    modification_time=0;
    for (uint32_t & d_ptr : direct_ptrs)
        d_ptr = 0;
    indirect_ptr = double_indirect_ptr = 0;
}
void Inode::initial_inode(u_int32_t u,bool is_file) {
    uid = u;
    if(is_file)
        mode =DEFAULT_FILE_PERM;
    else
        mode = DEFAULT_DIR_PERM;
    modification_time = create_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}
[[nodiscard]] std::string Inode::get_time(const std::string& type) const {
    std::time_t t;
    if (type == "create")
        t = static_cast<std::time_t>(create_time);
    else if (type == "modify")
        t = static_cast<std::time_t>(modification_time);
    std::tm* tm_ptr = std::localtime(&t);  // 转换为本地时间结构体
    auto year=std::to_string(tm_ptr->tm_year + 1900);   // 年份是从1900年开始计算的
    auto mon=std::to_string(tm_ptr->tm_mon + 1);// 月份是0~11，需要+1
    auto mday=std::to_string(tm_ptr->tm_mday); // 天
    auto hour=std::to_string(tm_ptr->tm_hour); // 时
    auto min=std::to_string(tm_ptr->tm_min); // 分
    auto sec=std::to_string(tm_ptr->tm_sec); // 秒

    // 补零函数（如 3 → "03"）
    auto pad = [](const std::string& s) {
        return s.length() == 1 ? "0" + s : s;
    };
    // 构造时间字符串
    std::string time_str = year + "-" + pad(mon) + "-" + pad(mday) + " " +
                           pad(hour) + ":" + pad(min) + ":" + pad(sec);
    return time_str;
}
[[nodiscard]] uint64_t Inode::get_create_time() const {
    return create_time;
}
bool Inode::update_mode(std::string &m) {
    if (m.length() != 9) {
        throw std::invalid_argument("权限字符串长度必须为9位，如 rwxr-xr-x");
    }
    mode &= 0xF000; // 只清除低12位权限位，保留高位文件类型

    // 用户权限
    if (m[0] == 'r') mode |= PERM_USER_READ;
    if (m[1] == 'w') mode |= PERM_USER_WRITE;
    if (m[2] == 'x') mode |= PERM_USER_EXEC;

    // 组权限
    if (m[3] == 'r') mode |= PERM_GROUP_READ;
    if (m[4] == 'w') mode |= PERM_GROUP_WRITE;
    if (m[5] == 'x') mode |= PERM_GROUP_EXEC;

    // 其他权限
    if (m[6] == 'r') mode |= PERM_OTH_READ;
    if (m[7] == 'w') mode |= PERM_OTH_WRITE;
    if (m[8] == 'x') mode |= PERM_OTH_EXEC;

    update_modification_time();
    return true;
}

void Inode::update_modification_time() {
    modification_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}
void Inode::update_size(uint32_t &s) {
    size = s;
    update_modification_time();
}
[[nodiscard]] uint32_t Inode::get_size() const {
    return size;
}

void Inode::update_direct_ptr(int block_num) {
    for(unsigned int & direct_ptr : direct_ptrs) {
        if(direct_ptr == 0) {
            direct_ptr = block_num;
            return;
        }
    }
}
void Inode::reset_ptr(int ptr_type,int pos) {
    switch(ptr_type) {
        case 0:
            direct_ptrs[pos]=0;
            break;
        case 1:
            indirect_ptr = 0;
            break;
        case 2:
            double_indirect_ptr = 0;
            break;
        default:
    }
}
void Inode::update_indirect_ptr(int block_num) {
    indirect_ptr = block_num;
}
void Inode::update_double_indirect_ptr(int block_num) {
    double_indirect_ptr = block_num;
}
[[nodiscard]] uint32_t Inode::get_direct_ptr(uint32_t ptr_num) const {
    return direct_ptrs[ptr_num];
}
[[nodiscard]] uint32_t Inode::get_indirect_ptr() const {
    return indirect_ptr;
}
[[nodiscard]] uint32_t Inode::get_double_indirect_ptr() const {
    return double_indirect_ptr;
}
[[nodiscard]] bool Inode::get_type() const {
    uint16_t type = mode & FILE_TYPE_MASK;
    if(type == TYPE_FILE)
        return true;
    return false;
}
[[nodiscard]] bool Inode::get_uid() const {
    return uid;
}
[[nodiscard]] uint16_t Inode::get_mode() const {
    return mode;
}
void Directory_Entry::init_dir(uint32_t i,const char* n) {
    inode = i;
    strcpy(name, n);
}
[[nodiscard]] uint32_t Directory_Entry::get_inode() const {
    return inode;
}
[[nodiscard]] char* Directory_Entry::get_name()  {
    return name;
}
void Directory_Entry::delete_file() {
    inode = 0;
}
void Directory_Entry::update_name(char* n) {
    strcpy(name, n);
}

void Data_Block::add_dir(uint32_t target_inode, const char* target_name) {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(Directory_Entry)) {
        auto* entry = reinterpret_cast<Directory_Entry*>(block + i);
        if (entry->get_inode()== 0) {
            entry->init_dir(target_inode, target_name);
            return;
        }
    }
}
void Data_Block::add_index(uint32_t target_index) {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(uint32_t)) {
        auto entry = reinterpret_cast<uint32_t*>(block + i);
        if(*entry==0) {
            *entry = target_index;
            return;
        }
    }
}
uint32_t Data_Block::get_inode(char* target_name) {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(Directory_Entry)) {
        auto* entry = reinterpret_cast<Directory_Entry*>(block + i);
        if (entry->get_inode() == 0)
            continue;  // 跳过已删除项
        if (strcmp(entry->get_name(), target_name) == 0) {
            return entry->get_inode();
        }
    }
    return 0;  // 没找到
}
uint32_t Data_Block::get_inode(uint32_t pos) {
    auto entry = reinterpret_cast<Directory_Entry*>(block + pos*sizeof(Directory_Entry));
    return entry->get_inode();  // 返回inode
}
char* Data_Block::get_name(uint32_t pos) {
    auto entry = reinterpret_cast<Directory_Entry*>(block + pos*sizeof(Directory_Entry));
    return entry->get_name();  // 返回名字
}
void Data_Block::delete_file(uint32_t pos) {
    auto entry = reinterpret_cast<Directory_Entry*>(block + pos*sizeof(Directory_Entry));
    return entry->delete_file(); // 删除文件
}
void Data_Block::init_directory_block() {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(Directory_Entry)) {
        auto* entry = reinterpret_cast<Directory_Entry*>(block + i);
        entry->delete_file();  // 设置 inode = 0
    }
}
void Data_Block::init_index_block() {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(uint32_t)) {
        auto entry = reinterpret_cast<uint32_t*>(block + i);
        *entry = 0;  // 设置 指针  = 0
    }
}
void Data_Block::reset_block() {
    memset(block, 0, BLOCK_SIZE);  // 使用memset完全清空块
}
bool Data_Block::has_name(const char* target_name) {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(Directory_Entry)) {
        auto* entry = reinterpret_cast<Directory_Entry*>(block + i);
        if (entry->get_inode() == 0) continue;
        if (strcmp(entry->get_name(), target_name) == 0)
            return true;
    }
    return false;
}
[[nodiscard]] std::string Data_Block::list_entries() {
    std::string result;
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(Directory_Entry)) {
        auto* entry = reinterpret_cast<Directory_Entry*>(block + i);
        if (entry->get_inode() != 0) {
            result += entry->get_name();
            result += " ";  // 添加空格分隔
        }
    }
    return result;
}
char* Data_Block::get_block() {
    return block;
}
bool Data_Block::is_full_of_directory() {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(Directory_Entry)) {
        auto* entry = reinterpret_cast<Directory_Entry*>(block + i);
        if (entry->get_inode() == 0)
            return false;
    }
    return true;
}
bool Data_Block::is_full_of_index() {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(uint32_t)) {
        auto* entry = reinterpret_cast<uint32_t*>(block + i);
        if(*entry==0)
            return false;
    }
    return true;
}
bool Data_Block::is_empty_of_directory() {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(Directory_Entry)) {
        auto* entry = reinterpret_cast<Directory_Entry*>(block + i);
        if (entry->get_inode() != 0)
            return false;
    }
    return true;
}
bool Data_Block::is_empty_of_index() {
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(uint32_t)) {
        auto* entry = reinterpret_cast<uint32_t*>(block + i);
        if(*entry!=0)
            return false;
    }
    return true;
}
uint32_t Data_Block::get_ptr_of_index(int num) {
    auto entry = reinterpret_cast<uint32_t*>(block+sizeof(uint32_t)*num);
    return *entry;
}