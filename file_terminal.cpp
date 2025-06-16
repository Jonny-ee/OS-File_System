#include "file_terminal.h"
#include <__filesystem/path.h>
#include <utility>
void File_Terminal::command() {

    while(true) {
        std::string cmd,parameter;
        for(int i=0;i<path_name.size();i++) {
            std::cout<<path_name[i];
        }
        std::cout<<":";
        std::cin >> cmd;
        std::getline(std::cin, parameter);  // 读取整行（包括回车）
        if(parameter==" "||parameter=="\n")
            parameter="";
        else
            parameter.erase(0,1);
        
        if(cmd=="ls") // 子文件list
            ls(parameter);
        else if(cmd=="cd") // 切换目录,..可以切换上一级
            cd(parameter);
        else if(cmd=="mkdir")// 创建目录,先检查有没有同类型同名
            mkdir(parameter);
        else if(cmd=="rm")  // 删除文件或文档
            fm.fm_delete(inode,parameter.c_str());
        else if(cmd=="cat") // 查看文件内容
            cat(parameter);
        else if(cmd=="vi") // 编辑文件，如果没有该文件就创建
            vi(parameter);
        else if(cmd=="reset") // 格式化该系统
            reset();
        else if(cmd=="exit") {
            save_to_file();
            break;
        }
    }
}

void File_Terminal::cd(const std::string& name) {
    if(name==".."&& inode!=ROOT) {
        path_inode.pop_back();
        inode=path_inode.back();
        path_name.pop_back();
        return;
    }
    if(name.empty())
        return;
    auto target =fm.find_inode(inode,name.c_str(),IS_DIRECTORY);
    if(target != inode) {
        inode=target;
        path_inode.emplace_back(inode);
        path_name.emplace_back(name+"/");
    }
}
uint32_t File_Terminal::write_to_file(uint32_t new_file,const std::string& text) {
    return fm.write_file(new_file,text);
}
void File_Terminal::cat(const std::string& name) {
    if(name.empty())
        return ;
    auto target =fm.find_inode(inode,name.c_str(),IS_FILE);
    if(target != inode) {
        std::cout<<fm.open_file(target)<<std::endl;
    }
}
void File_Terminal::vi(const std::string& name) {
    if(name.empty()) {
        std::cout << "错误：文件名不能为空" << std::endl;
        return;
    }

    auto target = fm.find_inode(inode, name.c_str(), IS_FILE);
    std::string text;
    
    // 如果没找到目标文件，就创建新的
    if(target == inode) {
        std::cout << "创建新文件: " << name << std::endl;
        std::cout << "请输入文件内容 (输入完成后按Enter):" << std::endl;
        std::getline(std::cin, text);
        auto new_file = fm.create_new(inode, 1, name.c_str(), IS_FILE);
        if(new_file != inode) {
            auto size=write_to_file(new_file, text);
            for(int i=0;i<path_inode.size();i++) {
                fm.update_directory_size(path_inode[i],size);
            }
            std::cout << "文件创建成功" << std::endl;
        } else {
            std::cout << "错误：无法创建文件" << std::endl;
        }
        return;
    }
    std::vector<std::string> lines;
    std::string current_line;
    bool modified = false;
    bool in_command_mode = false;
    std::string command_buffer;

    text = fm.open_file(target);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // 预载已有文本
    if (!text.empty()) {
        size_t pos = 0, next;
        while ((next = text.find('\n', pos)) != std::string::npos) {
            lines.push_back(text.substr(pos, next - pos));
            pos = next + 1;
        }
        if (pos < text.size()) {
            lines.push_back(text.substr(pos));
        }
    } else {
        lines.emplace_back();
    }

    int row = 0;
    int col = 0;
    int ch;

    while (true) {
        clear();
        printw("File name: %s\n", name.c_str());
        printw("Editing Mode Description：\n");
        printw("1. Enter the text content directly\n");
        printw("2. Use the arrow keys to move the cursor\n");
        printw("3. Use the Enter key to break lines\n");
        printw("4. Use the backspace key to delete characters\n");
        printw("5. Press the ESC key to enter the command mode\n");
        printw("6. Command mode input 'wq' save and exit, 'q' abandon modification and exit\n");
        printw("--------------------------------------------------\n");



        for (size_t i = 0; i < lines.size(); ++i) {
            mvprintw(i + 10, 0, "%s", lines[i].c_str());
        }

        if (in_command_mode) {
            mvprintw(LINES - 1, 0, "Enter q or wq:%s", command_buffer.c_str());
        }

        move(row + 10, col);
        refresh();

        ch = getch();

        if (in_command_mode) {
            if (ch == '\n' || ch == KEY_ENTER) {
                if (command_buffer == "q") {
                    modified = false;
                    break;
                } else if (command_buffer == "wq") {
                    modified = true;
                    break;
                }
                in_command_mode = false;
                command_buffer.clear();
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (!command_buffer.empty()) {
                    command_buffer.pop_back();
                }
            } else if (isprint(ch)) {
                command_buffer += ch;
            }
            continue;
        }

        if (ch == 27) { // ESC键
            in_command_mode = true;
            command_buffer.clear();
            continue;
        }

        if (ch == KEY_BACKSPACE || ch == 127) {
            if (col > 0) {
                lines[row].erase(col - 1, 1);
                col--;
                modified = true;
            } else if (row > 0) {
                col = lines[row - 1].size();
                lines[row - 1] += lines[row];
                lines.erase(lines.begin() + row);
                row--;
                modified = true;
            }
        } else if (ch == KEY_ENTER || ch == '\n') {
            std::string new_line = lines[row].substr(col);
            lines[row] = lines[row].substr(0, col);
            lines.insert(lines.begin() + row + 1, new_line);
            row++;
            col = 0;
            modified = true;
        } else if (ch == KEY_UP && row > 0) {
            row--;
            col = std::min(col, (int)lines[row].size());
        } else if (ch == KEY_DOWN && row < lines.size() - 1) {
            row++;
            col = std::min(col, (int)lines[row].size());
        } else if (ch == KEY_LEFT && col > 0) {
            col--;
        } else if (ch == KEY_RIGHT && col < lines[row].size()) {
            col++;
        } else if (isprint(ch)) {
            lines[row].insert(col, 1, (char)ch);
            col++;
            modified = true;
        }
    }

    endwin();

    // 保存或放弃修改
    if (modified) {
        text.clear();
        for (const auto& line : lines) {
            text += line + '\n';
        }
        auto size=write_to_file(target, text);
        for(int i=0;i<path_inode.size();i++) {
            fm.update_directory_size(path_inode[i],size);
        }
        std::cout << "文件已保存" << std::endl;
    } else {
        std::cout << "已放弃修改" << std::endl;
    }


}
void File_Terminal::mkdir(const std::string& name) {
    auto target =fm.find_inode(inode,name.c_str(),IS_DIRECTORY);
    if(target==inode)
        fm.create_new(inode,1,name.c_str(),IS_DIRECTORY);
    else
        std::cout<<"exsist such file or directory"<<std::endl;
}
void File_Terminal::ls(const std::string& name) {
    if(name == "-m")
        fm.list_directory(inode,true);
    else
        fm.list_directory(inode,false);
}
void File_Terminal::reset() {
    fm.init_file_manager();
}
void File_Terminal::save_to_file() {
    auto data = fm.save_data();
    std::string filePath = "filesystem.json";
    std::ofstream file(filePath);
    if (!file) {
        std::cerr << "文件创建失败！" << std::endl;
        return;
    }
    file << data.dump(4);
    file.close();
}

void File_Terminal::load_from_file() {
    std::ifstream file("filesystem.json");
    json data;
    try {
        file >> data;
        fm.load_data(data,true);
    } catch (const std::exception& e) {
        std::cerr << "无目标文件，正在初始化系统"<< std::endl;
        fm.load_data(data,false);
    }
    file.close();
}

File_Terminal::File_Terminal() {
    load_from_file();  // 在构造函数中加载数据
    inode = ROOT;
    path_name.emplace_back("root/");
    path_inode.emplace_back(ROOT);
    command();
}