#include "file.h"

Super_Block::Super_Block(const json &j) {
    num_inode_table = NUM_INODES;//8个表来存储inode，一个表可以写入32个inode，8个表则一共可以写入256条
    num_block = NUM_BLOCKS; //数据块一共256个
    free_inode = j["free_inode"]; //初始时，空闲inode号有256个
    free_data_block = j["free_data_block"]; //初始时，空闲数据块有256个
    inode_table_size = INODE_SIZE; //一个inode占128B
    data_block_size = BLOCK_SIZE; //数据块4KB
}
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
bool Super_Block::use_inode_table() {
    if (free_inode == 0)
        return false;
    free_inode.fetch_sub(1);
    return true;
}
void Super_Block::release_data_block() {
    free_data_block.fetch_add(1);
}
void Super_Block::release_inode_table() {
    free_inode.fetch_add(1);
}
int Super_Block::get_free_data_block() const {
    return free_data_block.load();
}
int Super_Block::get_free_inode() const {
    return free_inode.load();
}
json Super_Block::save_super_block() {
    return{{"free_inode",free_inode.load()},{"free_data_block",free_data_block.load()}};
}
Bitmap::Bitmap(const json &j,const std::string& n) {
    std::string bit = j[n];
    bit_map = std::bitset<NUM_BLOCKS>(bit);
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
[[nodiscard]] std::string Bitmap::save_bitmap() const {
    return bit_map.to_string();
}
void Bitmap::reset_bitmap() {
    bit_map.reset();
}
Inode::Inode(const json& j) {
    mode = j["mode"];
    size = j["size"];
    uid  = j["uid"];
    create_time = j["create_time"];
    modification_time= j["modification_time"];
    indirect_ptr = j["indirect_ptr"];
    double_indirect_ptr = j["double_indirect_ptr"];
    for(int i=0;i<DIRECT_PTR_COUNT;i++) {
        direct_ptrs[i]=j["direct_ptrs"][i];
    }
}
Inode::Inode() {
    mode = 0;
    size = 0;
    uid  = 0;
    create_time = 0;
    modification_time= 0;
    indirect_ptr = 0;
    double_indirect_ptr = 0;
    for(int i=0;i<DIRECT_PTR_COUNT;i++) {
        direct_ptrs[i]=0;
    }
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
void Inode::update_size(uint32_t &s,bool is_file) {
    if(is_file)
        size = s;
    else
        size +=s;;
    update_modification_time();
}
[[nodiscard]] uint32_t Inode::get_size() const {
    return size;
}

void Inode::update_direct_ptr(int block_num) {
    for(int i = 0; i < DIRECT_PTR_COUNT; i++) {
        if(direct_ptrs[i] == 0) {
            direct_ptrs[i] = block_num;
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
            break;
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
[[nodiscard]] std::string Inode::get_mode(bool be_string) const {
    // 提取低12位
    uint16_t low12 = mode & 0xFFF;

    // 生成9位权限字符串
    std::string result(9, '-');

    // 用户权限
    result[0] = (low12 & PERM_USER_READ)   ? 'r' : '-';
    result[1] = (low12 & PERM_USER_WRITE)  ? 'w' : '-';
    result[2] = (low12 & PERM_USER_EXEC)   ? 'x' : '-';

    // 组权限
    result[3] = (low12 & PERM_GROUP_READ)  ? 'r' : '-';
    result[4] = (low12 & PERM_GROUP_WRITE) ? 'w' : '-';
    result[5] = (low12 & PERM_GROUP_EXEC)  ? 'x' : '-';

    // 其他权限
    result[6] = (low12 & PERM_OTH_READ)    ? 'r' : '-';
    result[7] = (low12 & PERM_OTH_WRITE)   ? 'w' : '-';
    result[8] = (low12 & PERM_OTH_EXEC)    ? 'x' : '-';

    return result;
}
void Inode::reset_inode() {
    mode=0;
    size=0;
    uid=0;
    create_time=0;
    modification_time=0;
    direct_ptrs={};
    indirect_ptr=0;
    double_indirect_ptr=0;
}
json Inode::save_inode() {
    return {{"mode", mode}, {"size", size}, {"uid", uid},
        {"create_time",create_time},{"modification_time",modification_time},
        {"direct_ptrs",direct_ptrs},{"indirect_ptr",indirect_ptr},
        {"double_indirect_ptr",double_indirect_ptr}};
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
Data_Block::Data_Block(const json &j,int pos){
    std::string key = "block_" + std::to_string(pos);
    std::string encoded = j[key].get<std::string>();

    // 解码Base64字符串
    std::string decoded = base64_decode(encoded);

    // 验证解码后长度是否正确
    if (decoded.length() != 4096) {
        // 处理错误：数据长度不匹配
        std::memset(block, 0, 4096);  // 初始化为全0
        return;
    }

    // 复制解码后的数据到block数组
    std::memcpy(block, decoded.data(), 4096);
}
Data_Block::Data_Block() {
    strcpy(block,"");
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
uint32_t Data_Block::get_inode(const char* target_name) {
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
// 返回名字数组
[[nodiscard]] std::vector<std::string> Data_Block::list_entries() {
    std::vector<std::string> result;
    for (int i = 0; i < BLOCK_SIZE; i += sizeof(Directory_Entry)) {
        auto* entry = reinterpret_cast<Directory_Entry*>(block + i);
        if (entry->get_inode() != 0) {
            result.emplace_back( entry->get_name());
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

std::string Data_Block::save_data_block(){
    std::string str(block, 4096);
    return base64_encode(str, true);
}