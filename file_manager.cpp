#include "file_manager.h"

void File_Manager::load_data(const json& j,bool file_exsist) {
    if(file_exsist) {
        sb=std::make_unique<Super_Block>(j.at("super_block"));
        block_bitmap=std::make_unique<Bitmap>(j,"block_bitmap");
        inode_bitmap=std::make_unique<Bitmap>(j,"inode_bitmap");
        for (int i = 0; i < NUM_INODES; ++i) {
            std::string key = "inode_" + std::to_string(i);
            inode_table[i]=std::make_unique<Inode>(j.at(key)); // 赋值到数组
        }
        for (int i = 0; i < NUM_BLOCKS; ++i) {
            data_blocks[i]=std::make_unique<Data_Block>(j,i); // 赋值到数组
        }
        return;
    }
    sb=std::make_unique<Super_Block>();
    block_bitmap=std::make_unique<Bitmap>();
    inode_bitmap=std::make_unique<Bitmap>();
    for (int i = 0; i < NUM_INODES; ++i) {
        inode_table[i]=std::make_unique<Inode>(); // 赋值到数组
        data_blocks[i]=std::make_unique<Data_Block>(); // 赋值到数组
    }
    inode_bitmap->use_bitmap(ROOT);// 初始时将0号和1号置为占用，0号inode为root
    inode_table[ROOT]->initial_inode(ROOT,IS_DIRECTORY); // 创建root的inode，0号用户为管理员，false表示为文件夹
    block_bitmap->use_bitmap(ROOT); // 将0号数据块给root使用
    data_blocks[ROOT]->init_directory_block(); //将0号数据块初始化成目录项
    inode_table[ROOT]->update_direct_ptr(ROOT); // 将0号数据块与0号inode绑定
    create_new(ROOT,ROOT,"usr",IS_DIRECTORY); // 创建初始的系统文件夹 usr
    create_new(ROOT,ROOT,"sys",IS_DIRECTORY); // 创建初始的系统文件夹 sys

}
void File_Manager::init_file_manager() {
    block_bitmap->reset_bitmap();
    inode_bitmap->reset_bitmap();
    for(int i=0;i<NUM_INODES;i++) {
        inode_table[i]->reset_inode();
        data_blocks[i]->reset_block();
    }
    sb->init_sb();

    inode_bitmap->use_bitmap(ROOT);// 初始时将0号和1号置为占用，0号inode为root
    inode_table[ROOT]->initial_inode(ROOT,IS_DIRECTORY); // 创建root的inode，0号用户为管理员，false表示为文件夹
    block_bitmap->use_bitmap(ROOT); // 将0号数据块给root使用
    data_blocks[ROOT]->init_directory_block(); //将0号数据块初始化成目录项
    inode_table[ROOT]->update_direct_ptr(ROOT); // 将0号数据块与0号inode绑定
    create_new(ROOT,ROOT,"usr",IS_DIRECTORY); // 创建初始的系统文件夹 usr
    create_new(ROOT,ROOT,"sys",IS_DIRECTORY); // 创建初始的系统文件夹 sys
}

uint32_t File_Manager::create_new(uint32_t parent_inode,u_int32_t u,const char* n,bool type){
    // 分配inode
    auto inode = inode_bitmap->find_free_bit();
    if(inode==-1)
        return parent_inode;
    inode_bitmap->use_bitmap(inode);
    //初始化文件夹的inode节点
    inode_table[inode]->initial_inode(u,type);
    // 分配block，并且初始化
    auto block = block_bitmap->find_free_bit();
    if(block==-1)
        return parent_inode;
    block_bitmap->use_bitmap(block);// 分配block
    if(type==IS_DIRECTORY)
        data_blocks[block]->init_directory_block(); // 将该block设置为目录项block,如果不是文件夹就不用初始化

    // 将空闲块与新文件夹的inode绑定
    inode_table[inode]->update_direct_ptr(block);
    // 将新文件夹写入父文件夹的目录项中
    // 先遍历直接指针
    for(uint32_t i=0;i<DIRECT_PTR_COUNT;i++) {
        auto blk_id=inode_table[parent_inode]->get_direct_ptr(i); // 获得父文件夹指针指向的数据块序号
        // 如果先遍历到未被使用的指针，则绑定block，
        if(blk_id==0) {
            block = block_bitmap->find_free_bit();
            block_bitmap->use_bitmap(block);// 分配block
            data_blocks[block]->init_directory_block(); // 将该block设置为目录项block
            inode_table[parent_inode]->update_direct_ptr(block); // 将该block与父文件夹的直接指针绑定
            data_blocks[block]->add_dir(inode,n); // 将新文件添加进新的block中
            return inode;
        }
        else if(data_blocks[blk_id]->is_full_of_directory())
            continue;
        else {
            data_blocks[blk_id]->add_dir(inode,n); // 将新文件添加进新的block中
            return inode;
        }
    }
    // 如果1级指针都被写满，则遍历一级间接指针
    // 如果一级间接指针还没启用，则先设置索引块
    if(inode_table[parent_inode]->get_indirect_ptr()==0) {
        block = block_bitmap->find_free_bit();
        block_bitmap->use_bitmap(block);// 分配block
        data_blocks[block]->init_index_block(); // 将该block设置为index块
        inode_table[parent_inode]->update_indirect_ptr(block);// 将该block与父文件夹的一级间接指针绑定
    }
    auto index_id=inode_table[parent_inode]->get_indirect_ptr(); // 获取一级间接指针指向的index表
    //
    for(int i=0;i<BLOCK_SIZE / 4;i++) {
        auto blk_id= data_blocks[index_id]->get_ptr_of_index(i); // 获取index指针指向的数据块的id
        // 如果先遍历到index表中未被使用的指针，则将index表中未被使用的指针绑定block，
        if(blk_id==0) {
            block = block_bitmap->find_free_bit();
            block_bitmap->use_bitmap(block);// 分配block
            data_blocks[block]->init_directory_block(); // 将该block设置为目录项block
            data_blocks[index_id]->add_index(block); // 将该block与index表的绑定
            data_blocks[block]->add_dir(inode,n); // 将新文件添加进新的block中
            return inode;
        }
        else if(data_blocks[blk_id]->is_full_of_directory())
            continue;
        else {
            data_blocks[blk_id]->add_dir(inode,n); // 将新文件添加进新的block中
            return inode;
        }
    }
    // 如果一级间接指针指向的index表指向的数据块全被使用，则使用二级间接指针
    // 如果二级指针还未被启用，则初始化一个一级index表给二级间接指针
    if(inode_table[parent_inode]->get_double_indirect_ptr()==0) {
        block = block_bitmap->find_free_bit();
        block_bitmap->use_bitmap(block);// 分配block
        data_blocks[block]->init_index_block(); // 将该block设置为index块
        inode_table[parent_inode]->update_double_indirect_ptr(block);// 将该block与父文件夹的二级间接指针绑定
    }
    auto first_index_id=inode_table[parent_inode]->get_double_indirect_ptr(); // 获取二级间接指针指向的1级index块
    for(int i=0;i<BLOCK_SIZE / 4;i++) {
        auto second_index_id=data_blocks[first_index_id]->get_ptr_of_index(i); // 获取1级index表指向的2级index表id
        // 如果1级index表指向的2级index表id为0，则先设置2级index块
        if(second_index_id==0) {
            block = block_bitmap->find_free_bit();
            block_bitmap->use_bitmap(block);// 分配block
            data_blocks[block]->init_index_block(); // 将该block设置为index块
            data_blocks[first_index_id]->add_index(block); //将新的block与二级间接指针指向的index绑定
            second_index_id =block; //更新second_index_id
        }
        for(int j=0;j<BLOCK_SIZE/4;j++) {
            auto blk_id=data_blocks[second_index_id]->get_ptr_of_index(j); // 获取2级index表指向的数据块
            if(blk_id==0) {
                block = block_bitmap->find_free_bit();
                block_bitmap->use_bitmap(block);
                data_blocks[block]->init_directory_block();
                data_blocks[second_index_id]->add_index(block);
                data_blocks[block]->add_dir(inode,n); // 将新文件添加进新的block中
               return inode;
            }
            else if(data_blocks[blk_id]->is_full_of_directory())
                continue;
            else {
                data_blocks[blk_id]->add_dir(inode,n);
                return inode;
            }
        }
    }
    return parent_inode;
}

void File_Manager::free_blocks(uint32_t inode_id) {
    // 释放直接指针
    for(int i = 0; i < DIRECT_PTR_COUNT; i++) {
        auto blk = inode_table[inode_id]->get_direct_ptr(i);
        if (blk) {
            data_blocks[blk]->reset_block();       // 重置该块
            block_bitmap->release_bitmap(blk);
            inode_table[inode_id]->reset_ptr(0,i);
        }
    }

    // 一级间接指针
    auto ind = inode_table[inode_id]->get_indirect_ptr();
    if (ind) {
        for (int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto blk = data_blocks[ind]->get_ptr_of_index(i);
            if (blk) {
                data_blocks[blk]->reset_block();       // 重置该块
                block_bitmap->release_bitmap(blk);
            }
        }
        data_blocks[ind]->reset_block();       // 重置该块
        block_bitmap->release_bitmap(ind);
    }
    inode_table[inode_id]->reset_ptr(1);
    // 二级间接指针
    auto doub = inode_table[inode_id]->get_double_indirect_ptr();
    if (doub) {
        for (int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto ind2 = data_blocks[doub]->get_ptr_of_index(i);
            if (ind2) {
                for (int j = 0; j < BLOCK_SIZE / 4; j++) {
                    auto blk = data_blocks[ind2]->get_ptr_of_index(j);
                    if (blk) {
                        data_blocks[blk]->reset_block();       // 重置该块
                        block_bitmap->release_bitmap(blk);
                    }
                }
                data_blocks[ind2]->reset_block();       // 重置该块
                block_bitmap->release_bitmap(ind2);
            }
        }
        data_blocks[doub]->reset_block();       // 重置该块
        block_bitmap->release_bitmap(doub);
    }
    inode_table[inode_id]->reset_ptr(2);
}

void File_Manager::fm_delete(uint32_t parent_inode, const char* n) {
    auto process_block = [&](uint32_t blk_id) {
        if (blk_id == 0)
            return;
        for(int j = 0; j < BLOCK_SIZE / 4; j++) {
            auto inode = data_blocks[blk_id]->get_inode(j);
            auto name = data_blocks[blk_id]->get_name(j);
            if(inode == 0)
                continue;
            if(strcmp(name, n) == 0 || strcmp(n, "") == 0) {
                if(inode_table[inode]->get_uid()==ROOT)
                    return;
                if(!inode_table[inode]->get_type())
                    fm_delete(inode, "");           // 递归删除
                free_blocks(inode);                // 释放块
                inode_bitmap->release_bitmap(inode);      // 释放 inode
                data_blocks[blk_id]->delete_file(j);      // 删除目录项
                --j;  // 防止跳过
            }
        }
        if(data_blocks[blk_id]->is_empty_of_directory()) {
            data_blocks[blk_id]->reset_block();       // 重置该块
            block_bitmap->release_bitmap(blk_id); // 释放空目录块
        }
    };

    // 处理直接指针
    for(int i = 0; i < DIRECT_PTR_COUNT; i++) {
        process_block(inode_table[parent_inode]->get_direct_ptr(i));
    }

    // 处理一级间接指针
    auto ind = inode_table[parent_inode]->get_indirect_ptr();
    if(ind) {
        for(int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto blk = data_blocks[ind]->get_ptr_of_index(i);
            process_block(blk);
        }
    }

    // 处理二级间接指针
    auto doub = inode_table[parent_inode]->get_double_indirect_ptr();
    if(doub) {
        for(int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto ind2 = data_blocks[doub]->get_ptr_of_index(i);
            if(ind2) {
                for(int j = 0; j < BLOCK_SIZE / 4; j++) {
                    auto blk = data_blocks[ind2]->get_ptr_of_index(j);
                    process_block(blk);
                }
            }
        }
    }
}

void File_Manager::list_directory(uint32_t parent_inode,bool show_more) {
    std :: string res;
    // 直接指针
    for(int i = 0; i < DIRECT_PTR_COUNT; i++) {
        auto blk = inode_table[parent_inode]->get_direct_ptr(i);
        if(blk == 0)
            continue;
        auto name=data_blocks[blk]->list_entries();
        while(show_more && !name.empty()) {
            auto inode = data_blocks[blk]->get_inode(name.back().c_str());
            std::string more_info;
            more_info += name.back()+":"+'\n';
            more_info += "  创建时间："+inode_table[inode]->get_time("create")+'\n';
            more_info += "  修改时间："+inode_table[inode]->get_time("modify")+'\n';
            more_info += "  文件大小："+std::to_string(inode_table[inode]->get_size()) +"B"+'\n';
            more_info += "  文件权限："+inode_table[inode]->get_mode(true)+'\n';
            name.pop_back();
            res+=more_info;
        }
        while(!show_more && !name.empty()) {
            res+=name.back()+" ";
            name.pop_back();
        }

    }
    // 处理一级间接指针
    auto ind = inode_table[parent_inode]->get_indirect_ptr();
    if(ind) {
        for(int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto blk = data_blocks[ind]->get_ptr_of_index(i);
            if(blk == 0)
                continue;
            auto name=data_blocks[blk]->list_entries();
            while(show_more && !name.empty()) {
                auto inode = data_blocks[blk]->get_inode(name.back().c_str());
                std::string more_info;
                more_info += name.back()+":"+'\n';
                more_info += "  创建时间："+inode_table[inode]->get_time("create")+'\n';
                more_info += "  修改时间："+inode_table[inode]->get_time("create")+'\n';
                more_info += "  文件大小："+std::to_string(inode_table[inode]->get_size()) +"B"+'\n';
                more_info += "  文件权限："+inode_table[inode]->get_mode(true)+'\n';
                name.pop_back();
                res+=more_info;
            }
            if(!show_more && !name.empty()) {
                res+=name.back()+" ";
                name.pop_back();
            }
        }
    }
    // 处理二级间接指针
    auto doub = inode_table[parent_inode]->get_double_indirect_ptr();
    if(doub) {
        for(int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto ind2 = data_blocks[doub]->get_ptr_of_index(i);
            if(ind2) {
                for(int j = 0; j < BLOCK_SIZE / 4; j++) {
                    auto blk = data_blocks[ind2]->get_ptr_of_index(j);
                    if(blk == 0)
                        continue;
                    auto name=data_blocks[blk]->list_entries();
                    while(show_more && !name.empty()) {
                        auto inode = data_blocks[blk]->get_inode(name.back().c_str());
                        std::string more_info;
                        more_info += name.back()+":"+'\n';
                        more_info += "  创建时间："+inode_table[inode]->get_time("create")+'\n';
                        more_info += "  修改时间："+inode_table[inode]->get_time("create")+'\n';
                        more_info += "  文件大小："+std::to_string(inode_table[inode]->get_size()) +"B"+'\n';
                        more_info += "  文件权限："+inode_table[inode]->get_mode(true)+'\n';
                        name.pop_back();
                        res+=more_info;
                    }
                    if(!show_more && !name.empty()) {
                        res+=name.back()+" ";
                        name.pop_back();
                    }
                }
            }
        }
    }
    std::cout<<res<<std::endl;
}
std::string File_Manager::open_file(uint32_t target_inode) {
    std :: string res;
    // 遍历直接指针
    for(int i = 0; i < DIRECT_PTR_COUNT; i++) {
        auto blk = inode_table[target_inode]->get_direct_ptr(i);
        if(blk == 0)
            continue;
        res+=data_blocks[blk]->get_block();
    }
    // 遍历一级间接指针
    auto ind = inode_table[target_inode]->get_indirect_ptr();
    if(ind) {
        for(int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto blk = data_blocks[ind]->get_ptr_of_index(i);
            if(blk == 0)
                continue;
            res+=data_blocks[blk]->get_block();
        }
    }
    //遍历二级间接指针
    auto doub = inode_table[target_inode]->get_double_indirect_ptr();
    if(doub) {
        for(int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto ind2 = data_blocks[doub]->get_ptr_of_index(i);
            if(ind2) {
                for(int j = 0; j < BLOCK_SIZE/4; j++) {
                    auto blk = data_blocks[ind2]->get_ptr_of_index(j);
                    if(blk == 0) continue;
                    res+=data_blocks[blk]->get_block();
                }
            }
        }
    }
    return res;
}

uint32_t File_Manager::find_inode(uint32_t parent_inode,const char* n,bool type) {
    for(int i = 0; i < DIRECT_PTR_COUNT; i++) {
        auto blk_id=inode_table[parent_inode]->get_direct_ptr(i);
        if(blk_id == 0)
            continue;
        for(int j = 0; j < BLOCK_SIZE / 4; j++) {
            auto inode = data_blocks[blk_id]->get_inode(j);
            if(inode == 0)
                continue;
            auto name = data_blocks[blk_id]->get_name(j);
            if(strcmp(name, n) == 0&&inode_table[inode]->get_type()==type) {
                return inode;
            }
        }
    }
    // 处理一级间接指针
    auto ind = inode_table[parent_inode]->get_indirect_ptr();
    if(ind) {
        for(int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto blk_id = data_blocks[ind]->get_ptr_of_index(i);
            if(blk_id == 0)
                continue;
            for(int j = 0; j < BLOCK_SIZE / 4; j++) {
                auto inode = data_blocks[blk_id]->get_inode(j);
                if(inode == 0)
                    continue;
                auto name = data_blocks[blk_id]->get_name(j);
                if(strcmp(name, n) == 0&&inode_table[inode]->get_type()==type) {
                    return inode;
                }
            }
        }
    }
    // 处理二级间接指针
    auto doub = inode_table[parent_inode]->get_double_indirect_ptr();
    if(doub) {
        for(int i = 0; i < BLOCK_SIZE / 4; i++) {
            auto ind2 = data_blocks[doub]->get_ptr_of_index(i);
            if(ind2) {
                for(int j = 0; j < BLOCK_SIZE / 4; j++) {
                    auto blk_id = data_blocks[ind2]->get_ptr_of_index(j);
                    if(blk_id == 0)
                        continue;
                    for(int k = 0; k < BLOCK_SIZE / 4; k++) {
                        auto inode = data_blocks[blk_id]->get_inode(j);
                        if(inode == 0)
                            continue;
                        auto name = data_blocks[blk_id]->get_name(j);
                        if(strcmp(name, n) == 0&&inode_table[inode]->get_type()==type) {
                            return inode;
                        }
                    }
                }
            }
        }
    }
    return parent_inode;
}

uint32_t File_Manager::write_file(uint32_t inode_id,const std::string& content) {
    // 检查是否是文件
    if(!inode_table[inode_id]->get_type()) {
        std::cout << "错误：不能向目录写入内容" << std::endl;
        return 0;
    }
    if(!(inode_table[inode_id]->get_mode()&PERM_USER_WRITE)) {
        std::cout << "错误：没有写入权限" << std::endl;
        return 0;
    }
    if(!(inode_table[inode_id]->get_mode()&PERM_USER_READ)) {
        std::cout << "错误：没有读取权限" << std::endl;
        return 0;
    }


    // 计算需要的块数
    size_t content_size = content.length();
    size_t blocks_needed = (content_size + BLOCK_SIZE - 1) / BLOCK_SIZE;  // 向上取整

    // 释放原有块
    free_blocks(inode_id);

    // 分配新块并写入内容
    size_t content_pos = 0;
    for(size_t i = 0; i < blocks_needed; i++) {
        // 分配新块
        auto block = block_bitmap->find_free_bit();
        if(block == -1) {
            std::cout << "错误：没有足够的空间" << std::endl;
            return 0;
        }
        block_bitmap->use_bitmap(block);

        // 计算当前块要写入的内容
        size_t block_size = std::min((size_t)BLOCK_SIZE, content_size - content_pos);
        std::string block_content = content.substr(content_pos, block_size);

        // 写入内容
        memcpy(data_blocks[block]->get_block(), block_content.c_str(), block_size);

        // 更新inode的指针
        if(i < DIRECT_PTR_COUNT) {
            inode_table[inode_id]->update_direct_ptr(block);
        } else if(i < DIRECT_PTR_COUNT + BLOCK_SIZE/4) {
            // 需要一级间接指针
            if(i == DIRECT_PTR_COUNT) {
                // 创建一级间接指针块
                auto index_block = block_bitmap->find_free_bit();
                if(index_block == -1) {
                    std::cout << "错误：没有足够的空间" << std::endl;
                    return 0;
                }
                block_bitmap->use_bitmap(index_block);
                data_blocks[index_block]->init_index_block();
                inode_table[inode_id]->update_indirect_ptr(index_block);
            }
            // 将数据块添加到一级间接指针
            data_blocks[inode_table[inode_id]->get_indirect_ptr()]->add_index(block);
        } else {
            // 需要二级间接指针
            if(i == DIRECT_PTR_COUNT + BLOCK_SIZE/4) {
                // 创建二级间接指针块
                auto index_block = block_bitmap->find_free_bit();
                if(index_block == -1) {
                    std::cout << "错误：没有足够的空间" << std::endl;
                    return 0;
                }
                block_bitmap->use_bitmap(index_block);
                data_blocks[index_block]->init_index_block();
                inode_table[inode_id]->update_double_indirect_ptr(index_block);
            }

            // 计算在一级间接指针中的位置
            size_t first_index = (i - DIRECT_PTR_COUNT - BLOCK_SIZE/4) / (BLOCK_SIZE/4);
            size_t second_index = (i - DIRECT_PTR_COUNT - BLOCK_SIZE/4) % (BLOCK_SIZE/4);

            // 如果需要新的一级间接指针块
            if(second_index == 0) {
                auto new_index_block = block_bitmap->find_free_bit();
                if(new_index_block == -1) {
                    std::cout << "错误：没有足够的空间" << std::endl;
                    return 0;
                }
                block_bitmap->use_bitmap(new_index_block);
                data_blocks[new_index_block]->init_index_block();
                data_blocks[inode_table[inode_id]->get_double_indirect_ptr()]->add_index(new_index_block);
            }

            // 将数据块添加到二级间接指针
            auto first_index_block = data_blocks[inode_table[inode_id]->get_double_indirect_ptr()]->get_ptr_of_index(first_index);
            data_blocks[first_index_block]->add_index(block);
        }

        content_pos += block_size;
    }

    // 更新inode的大小和修改时间
    uint32_t size = content_size;
    inode_table[inode_id]->update_size(size,IS_FILE);
    return size;
}

uint32_t File_Manager::get_file_size(uint32_t inode_id) {
    return inode_table[inode_id]->get_size();
}
json File_Manager::save_data() {
    json data;
    data["super_block"] = sb->save_super_block();
    data["block_bitmap"] = block_bitmap->save_bitmap();
    data["inode_bitmap"] = inode_bitmap->save_bitmap();
    for(int i = 0; i < NUM_INODES; i++) {
        auto inode_index= "inode_" + std::to_string(i);
        data[inode_index] = inode_table[i]->save_inode();
        auto block_index = "block_"+std::to_string(i);
        data[block_index]=data_blocks[i]->save_data_block();
    }
    return data;
}

void File_Manager::update_directory_size(uint32_t inode_id,uint32_t new_size) {
    inode_table[inode_id]->update_size(new_size,IS_DIRECTORY);
}