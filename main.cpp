#include <iostream>
#include <chrono>
#include"file_terminal.h"
using namespace std;
int main()
{
    system("clear");
    std::cout<<"开发者：2356215 郑功灿"<<std::endl;
    std::cout<<"联系我：2356215@tongji.edu.cn"<<std::endl<<std::endl;
    std::cout<<"欢迎进入Inode文件管理系统"<<std::endl;
    std::cout<<"本系统通过模拟Linux的inode机制，旨在理解文件系统的底层管理原理，省略了部分复杂实现以突出核心概念。"<<std::endl;
    std::cout<<"本项目实现了："<<std::endl;
    std::cout<<"1. cd--用于改变当前工作目录"<<std::endl;
    std::cout<<"2. ls--用于显示指定工作目录下之内容"<<std::endl;
    std::cout<<"3. mkdir--用于在创建目录"<<std::endl;
    std::cout<<"4. rm--用于删除目录或文件"<<std::endl;
    std::cout<<"5. cat--用于查看文件内容"<<std::endl;
    std::cout<<"6. vi--用于创建新文件或修改文件内容"<<std::endl<<std::endl;
    std::cout<<"现在你可以点击任意按键开始该系统"<<std::endl;
    getchar();
    system("clear");
    File_Terminal ft;
    system("clear");
    std::cout<<"感谢使用Inode文件管理系统"<<std::endl<<std::endl;
    std::cout<<"开发者：2356215 郑功灿"<<std::endl;
    std::cout<<"联系我：2356215@tongji.edu.cn"<<std::endl;
    return 0;
}
