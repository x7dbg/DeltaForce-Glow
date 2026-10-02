// 包含自定义头文件 Includes.h，其中应包含了各种结构体、类定义（HideDll、C_Hack）、Windows API 头文件等
#include "Includes.h"

// DllMain 是 DLL 的入口函数，DLL 被加载/卸载时系统会自动调用这个函数
// APIENTRY 是 Windows 宏，规定了函数的调用约定
// hModule 是当前 DLL 的模块句柄，代表 DLL 在目标进程内存中的基地址
// ul_reason_for_call 是调用 DllMain 的原因，这里是进程加载、线程创建、线程退出、卸载等
// lpReserved 是保留参数，系统内部使用，一般不管
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    // 判断 DLL 被调用的事件类型
    switch (ul_reason_for_call)
    {
    // DLL_PROCESS_ATTACH：当前 DLL 被映射加载到目标进程的时候触发（最常用，DLL 注入就是走到这里）
    case DLL_PROCESS_ATTACH:
    {
        // DisableThreadLibraryCalls(hModule);
        // 关闭该 DLL 的线程通知，新线程创建/退出时不再通知 DllMain，避免 DllMain 频繁调用，减少开销
        DisableThreadLibraryCalls(hModule);

        // 实例化 HideDll 对象，传入当前 DLL 模块句柄，后面调用各种功能隐藏 DLL 自身
        HideDll DLL(hModule);

        // RemoveLDR(); 从进程 LDR 链表中移除 DLL 节点。LDR 链表是系统记录的加载模块，移除后工具无法检测到 DLL
        DLL.RemoveLDR();

        // RemoveMAP(); 移除内存映射标记，抹除 DLL 的内存映射信息，内存扫描工具不容易发现
        DLL.RemoveMAP();

        // RemovePEH(); 移除 PE 头，PE 头是 DLL 的文件头信息，抹除后在内存中看不出是一个标准 DLL 模块，无法解析/调试
        DLL.RemovePEH();

        // 被注释掉的代码：创建线程执行 C_Hack::Init 函数，传入当前 DLL 模块句柄
        // CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)C_Hack::Init, hModule, NULL, NULL);

        // CreateThread Windows API 创建一个新的线程，新线程执行 C_Hack::Init2 函数
        // 参数 1：线程安全属性，NULL 默认
        // 参数 2：线程栈大小，NULL 使用系统默认栈大小
        // 参数 3：线程入口函数，强制转换为 LPTHREAD_START_ROUTINE 线程函数指针，执行 C_Hack::Init2 函数，这是功能初始化的核心
        // 参数 4：传递给线程函数的参数，这里传 DLL 模块句柄进去
        // 参数 5：线程创建标志，NULL = 创建后立即运行
        // 参数 6：返回线程 ID，NULL 不需要获取 ID
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)C_Hack::Init2, hModule, NULL, NULL);

        // 结束 DLL_PROCESS_ATTACH 分支
        break;
    }
    // DLL_THREAD_ATTACH：目标进程新建一个线程的时候触发
    case DLL_THREAD_ATTACH:
    // DLL_THREAD_DETACH：目标进程的某个线程结束的时候触发
    case DLL_THREAD_DETACH:
    // DLL_PROCESS_DETACH：DLL 从进程卸载的时候触发，FreeLibrary 或进程关闭
    case DLL_PROCESS_DETACH:
        // 其他事件不处理，直接 break
        break;
    }
    // DllMain 返回 TRUE 表示系统 DLL 初始化成功，返回 FALSE 会导致 DLL 加载失败
    return TRUE;
}

//BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
//{
//	switch (ul_reason_for_call)
//	{
//	case DLL_PROCESS_ATTACH: {
//		DisableThreadLibraryCalls(hModule);
//		HideDll DLL(hModule);
//		//DLL.RemoveLDR();
//		//DLL.RemoveMAP();
//		//DLL.RemovePEH();
//		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)C_Hack::Init, hModule, NULL, NULL);
//		break;
//	}
//	case DLL_THREAD_ATTACH:
//	case DLL_THREAD_DETACH:
//	case DLL_PROCESS_DETACH:
//		break;
//	}
//	return TRUE;
//}