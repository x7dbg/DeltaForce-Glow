// #pragma once 头文件保护符，防止头文件被重复包含导致编译报错
#pragma once

// Windows 系统相关 API 头文件，HMODULE、CreateThread、VirtualQuery 等全部 Windows 底层 API 都依赖这个
#include <windows.h>

// C++ 标准输入输出库，cout、printf 等控制台输出
#include <iostream>

// iomanip 格式化输出库，用来打印十六进制、对齐等
#include <iomanip>

// cstdlib 是 C 标准工具库，内存分配、程序退出等函数
#include <cstdlib>

// unordered_map 是哈希表容器，存储键值对，这里存储名字->地址的映射
#include <unordered_map> 

// string 是 C++ std::string 字符串库
#include <string>

// utility 是工具库，std::pair 键值对结构
#include <utility>  

// mutex 是互斥锁，多线程并发时防止多线程同时读写导致崩溃
#include <mutex>

// thread 是 C++ std::thread 线程库
#include <thread>

// vector 是动态数组容器，存储实体列表、敌人列表等
#include <vector>

// format 是 C++20 格式化字符串 std::format，用来拼接打印字符串
#include <format>

//#define DEBUG_ENABLE   // 取消此行注释，就可以开启控制台输出，注释掉就关闭调试
#ifdef DEBUG_ENABLE
// 当定义了 DEBUG_ENABLE 时展开 DeBug 宏
// __VA_ARGS__ 是可变参数宏，可以传入任意参数打印，类似 printf
#define DeBug(...)                                                      \
        do {                                                            \
            // GetConsoleWindow() 获取当前进程控制台窗口句柄，如果为 NULL 说明没有控制台
            if (GetConsoleWindow() == NULL) {                           \
                // AllocConsole() Windows API，为 DLL 分配创建一个新的控制台窗口
                AllocConsole();                                         \
                // freopen_s 将 C 语言标准输入 stdin 重定向绑定到控制台输入，CONIN$
                freopen_s((FILE**)stdin, "CONIN$", "r", stdin);         \
                // freopen_s 将 C 语言标准输出 stdout 重定向绑定到控制台输出，CONOUT$
                freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);      \
            }                                                           \
            // printf 将传入的格式化数据打印到控制台
            printf(__VA_ARGS__);                                        \
        } while (0)
#else
// 如果没有定义 DEBUG_ENABLE，DeBug(xxx) 直接被替换为 ((void)0) 空语句，什么都不执行，调试代码完全失效
#define DeBug(...) ((void)0)
#endif

// 下面是本项目自己写的全局自定义头文件，全局统一包含在 Includes.h 里，其他文件只需要 #include "Includes.h" 即可
#include "Enum.h"      // 本项目的常用枚举定义
#include "Utils.h"     // 封装的一些 Mem 内存库、Utils 工具的头文件
#include "Hooks.h"     // Hook 功能相关代码，用于挂钩函数功能
#include "Offsets.h"   // UE 偏移量头文件，全部注释管理偏移量命名空间
#include "Engine.h"    // UE 引擎封装层，封装 GEngine、UWorld 等引擎相关类
#include "Game.h"      // 游戏业务逻辑层，游戏专属逻辑
#include "Entity.h"    // 实体封装层，角色 Actor 的读取、封装
#include "Hack.h"      // C_Hack 的头文件，DllMain 中调用到的 C_Hack::Init2 函数在这个头文件
#include "Hidedll.h"   // HideDll 的头文件，DllMain 中用到的 DLL RemoveLDR/RemoveMAP/RemovePEH 功能在这个头文件