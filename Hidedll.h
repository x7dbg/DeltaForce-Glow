// 头文件保护符，防止重复包含
#pragma once
// Winternl.h 包含 Nt 内核结构体，PEB、TEB、LDR_DATA_TABLE_ENTRY 等定义
#include <Winternl.h>

// HideDll 类：DLL 内存隐藏类，将本 DLL 从进程模块列表、PE 头、内存映射中抹除
class HideDll
{
public:
    // 构造函数，传入当前 DLL 模块句柄并保存到成员变量
    HideDll(HMODULE hModule)
    {
        m_hModule = hModule;
    }

    // RemoveMAP：抹除内存映射，使用 Nt 原生 API 重新映射 DLL 内存
    // 效果：让 VirtualQuery 等内存扫描工具看不到这个 DLL 的模块映射，但内存中依然存在
    BOOL RemoveMAP()
    {
        // GetModuleLen() 获取 DLL 镜像总大小 SizeOfImage
        auto Len = GetModuleLen();
        // 当前 DLL 模块基地址
        auto DllHandle = m_hModule;

        // 从 ntdll 获取 ZwUnmapViewOfSection 函数地址，卸载内存映射的 NtAPI
        auto ZwUnmapViewOfSection = GetProcAddress(LoadLibraryA(("ntdll.dll")), ("ZwUnmapViewOfSection"));
        if (!ZwUnmapViewOfSection) return false;

        // ZwAllocateVirtualMemory 是 NtAPI，用于分配内存
        auto ZwAllocateVirtualMemory = GetProcAddress(LoadLibraryA(("ntdll.dll")), ("ZwAllocateVirtualMemory"));
        if (!ZwAllocateVirtualMemory) return false;

        auto DllLen = Len;
        // 在堆上开辟一块缓存，用来备份 DLL 全部数据
        auto DllAddr = new BYTE[Len];

        // shellcode 是 64 位机器码，运行时执行取消映射、重新映射的逻辑
        // 因为直接调用 ZwUnmapViewOfSection 会导致 DLL 卸载后无法继续执行
        // 所以把逻辑写到 shellcode 中，shellcode 放在 DLL 镜像内存中，卸载映射后还可以继续执行
        BYTE shellcode[256] =
        {
            0x4C, 0x89, 0x4C, 0x24, 0x20, 0x4C, 0x89, 0x44, 0x24, 0x18, 0x48, 0x89, 0x54, 0x24, 0x10, 0x48,
            0x89, 0x4C, 0x24, 0x08, 0x56, 0x57, 0x48, 0x83, 0xEC, 0x38, 0x48, 0x8B, 0x54, 0x24, 0x58, 0x48,
            0x8B, 0x4C, 0x24, 0x50, 0xFF, 0x54, 0x24, 0x70, 0x48, 0x85, 0xC0, 0x74, 0x04, 0x32, 0xC0, 0xEB,
            0x75, 0x48, 0xC7, 0x44, 0x24, 0x28, 0x40, 0x00, 0x00, 0x00, 0x48, 0xC7, 0x44, 0x24, 0x20, 0x00,
            0x20, 0x00, 0x00, 0x4C, 0x8D, 0x4C, 0x24, 0x60, 0x45, 0x33, 0xC0, 0x48, 0x8D, 0x54, 0x24, 0x58,
            0x48, 0x8B, 0x4C, 0x24, 0x50, 0xFF, 0x54, 0x24, 0x78, 0x48, 0x85, 0xC0, 0x74, 0x04, 0x32, 0xC0,
            0xEB, 0x44, 0x48, 0xC7, 0x44, 0x24, 0x28, 0x40, 0x00, 0x00, 0x00, 0x48, 0xC7, 0x44, 0x24, 0x20,
            0x00, 0x10, 0x00, 0x00, 0x4C, 0x8D, 0x4C, 0x24, 0x60, 0x45, 0x33, 0xC0, 0x48, 0x8D, 0x54, 0x24,
            0x58, 0x48, 0x8B, 0x4C, 0x24, 0x50, 0xFF, 0x54, 0x24, 0x78, 0x48, 0x85, 0xC0, 0x74, 0x04, 0x32,
            0xC0, 0xEB, 0x13, 0x48, 0x8B, 0x7C, 0x24, 0x58, 0x48, 0x8B, 0x74, 0x24, 0x68, 0x48, 0x8B, 0x4C,
            0x24, 0x60, 0xF3, 0xA4, 0xB0, 0x01, 0x48, 0x83, 0xC4, 0x38, 0x5F, 0x5E, 0xC3
        };

        // 将 DLL 镜像内存完整拷贝备份到堆内存 DllAddr
        memcpy(DllAddr, DllHandle, Len);

        DWORD OldProtect = 0;
        // 修改 shellcode 所在栈内存权限为 PAGE_EXECUTE_READ(64)，栈默认不可执行，必须开启才能执行机器码
        VirtualProtect(&shellcode, sizeof(shellcode), 64, &OldProtect);

        // 强制将 shellcode 转成函数指针并调用
        // 参数说明：-1 当前进程，DllHandle 原始地址，Len 大小，DllAddr 备份数据地址，两个 NtAPI 函数地址
        auto result = reinterpret_cast<BOOL(__cdecl*)(INT64, HMODULE, DWORD64, PVOID64, FARPROC, FARPROC)>(&shellcode)(-1, DllHandle, Len, DllAddr, ZwUnmapViewOfSection, ZwAllocateVirtualMemory);

        // 执行完毕，恢复栈内存原来的保护权限
        VirtualProtect(&shellcode, sizeof(shellcode), OldProtect, &OldProtect);

        // 释放堆上备份的内存
        delete[] DllAddr;
        return result;
    }

    // RemovePEH：抹除 PE 头，PE 头是 DLL 最开头 0x1000 字节，包含 MZ 签名、NT 头等标识
    // 将 DLL 内存最开头 0~0xFFF 全部清零，在内存中无法识别为 PE 模块
    BOOL RemovePEH()
    {
        auto DllHandle = m_hModule;
        // 转为字节指针，遍历 DLL 内存
        unsigned char* ImageBase = reinterpret_cast<unsigned char*>(DllHandle);

        // IMAGE_DOS_HEADER DOS 头，DLL 最开头的 MZ 签名
        auto DosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(ImageBase);
        // 判断 MZ 签名是否合法
        if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return false;

        // 找到 NT 头位置，e_lfanew 为 NT 头偏移
        auto NTHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(ImageBase + DosHeader->e_lfanew);
        // 判断 NT 签名 PE\0\0
        if (NTHeader->Signature != IMAGE_NT_SIGNATURE)
            return false;

        // 循环将 DLL 内存前 0x1000 字节全部写 0，抹除 DOS 头、NT 头等标识
        for (int i = 0; i < 0x1000; i++)
            ImageBase[i] = 0;
        return true;
    }

    // RemoveLDR：从 PEB 的 LDR 链表中摘除 DLL 节点
    // Windows 进程 PEB 内部维护 3 个双向链表，记录全部已加载模块；EnumProcessModules 等工具读取的就是这个链表
    // 摘除节点后，工具就枚举不到这个 DLL 了
    BOOL RemoveLDR()
    {
        auto hModule = m_hModule;
        // NtCurrentTeb() 获取 TEB 线程环境块，TEB 里面有 PEB 进程环境块指针
        PBYTE pPeb = (PBYTE)NtCurrentTeb()->ProcessEnvironmentBlock;

        PPEB_LDR_DATA pLdr;
        PLDR_DATA_TABLE_ENTRY pLdrData;

        // 从 PEB 取出 LDR 数据，保存模块链表
        pLdr = (PPEB_LDR_DATA)(*(DWORD64*)(pPeb + offsetof(struct _PEB, Ldr)));
        // 获取第一个模块链表节点 InLoadOrderModuleList
        pLdrData = (PLDR_DATA_TABLE_ENTRY)pLdr->InLoadOrderModuleList.Flink;

        do
        {
            // 判断当前节点 DllBase 模块基址是否等于我们 DLL 的基址，找到自己的 DLL 节点
            if (pLdrData->DllBase == hModule)
            {
                // 双向链表摘除操作，前后节点全部连接
                // 1. InLoadOrderModuleList 按加载顺序的链表
                pLdrData->InLoadOrderLinks.Blink->Flink = pLdrData->InLoadOrderLinks.Flink;
                pLdrData->InLoadOrderLinks.Flink->Blink = pLdrData->InLoadOrderLinks.Blink;

                // 2. InMemoryOrderModuleList 按内存地址顺序的链表
                pLdrData->InMemoryOrderLinks.Blink->Flink = pLdrData->InMemoryOrderLinks.Flink;
                pLdrData->InMemoryOrderLinks.Flink->Blink = pLdrData->InMemoryOrderLinks.Blink;

                // 3. InInitializationOrderModuleList 按初始化顺序的链表
                pLdrData->InInitializationOrderLinks.Blink->Flink = pLdrData->InInitializationOrderLinks.Flink;
                pLdrData->InInitializationOrderLinks.Flink->Blink = pLdrData->InInitializationOrderLinks.Blink;

                return true;
            }
            // 移动到链表下一个节点
            pLdrData = (PLDR_DATA_TABLE_ENTRY)(pLdrData->InLoadOrderLinks.Flink);
        } while (pLdrData->DllBase); // 链表头节点 DllBase=0 时循环终止
        return false;
    }

    // 析构函数，没有任何释放逻辑
    ~HideDll()
    {
    }

private:
    // GetModuleLen：从 PE 头中获取 DLL 镜像大小 SizeOfImage
    DWORD64 GetModuleLen()
    {
        auto hModule = m_hModule;
        auto pImage = (PBYTE)hModule;
        PIMAGE_DOS_HEADER pImageDosHeader;
        PIMAGE_NT_HEADERS pImageNtHeader;

        pImageDosHeader = (PIMAGE_DOS_HEADER)pImage;
        if (pImageDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return 0;

        pImageNtHeader = (PIMAGE_NT_HEADERS)&pImage[pImageDosHeader->e_lfanew];
        if (pImageNtHeader->Signature != IMAGE_NT_SIGNATURE)
            return 0;

        // 返回 OptionalHeader.SizeOfImage 是 DLL 镜像总共占用的内存大小
        return pImageNtHeader->OptionalHeader.SizeOfImage;
    }

    // 手动定义特定内核结构体，这些在 Winternl 没有完整定义，需要项目自己定义
    typedef struct _UNICODE_STRING
    {
        USHORT    Length;
        USHORT    MaximumLength;
        PWSTR     Buffer;
    } UNICODE_STRING, * PUNICODE_STRING;

    // PEB_LDR_DATA 是 PEB 中的模块链表结构体，包含三个模块双向链表
    typedef struct _PEB_LDR_DATA
    {
        ULONG                    Length;
        BOOLEAN                  Initialized;
        PVOID                    SsHandle;
        LIST_ENTRY               InLoadOrderModuleList;
        LIST_ENTRY               InMemoryOrderModuleList;
        LIST_ENTRY               InInitializationOrderModuleList;
    } PEB_LDR_DATA, * PPEB_LDR_DATA;

    // LDR_DATA_TABLE_ENTRY 是每一个加载模块对应的链表节点
    typedef struct _LDR_DATA_TABLE_ENTRY
    {
        LIST_ENTRY InLoadOrderLinks;
        LIST_ENTRY InMemoryOrderLinks;
        LIST_ENTRY InInitializationOrderLinks;
        PVOID DllBase;                 // DLL 模块基地址
        PVOID EntryPoint;              // DLL 入口地址
        ULONG SizeOfImage;             // 镜像大小
        UNICODE_STRING FullDllName;    // DLL 完整路径名
        UNICODE_STRING BaseDllName;    // DLL 文件名
        ULONG Flags;
        USHORT LoadCount;
        USHORT TlsIndex;
        union
        {
            LIST_ENTRY HashLinks;
            struct
            {
                PVOID SectionPointer;
                ULONG CheckSum;
            };
        };
        ULONG TimeDateStamp;
    } LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

    HMODULE m_hModule; // 当前 DLL 模块句柄/基地址
};