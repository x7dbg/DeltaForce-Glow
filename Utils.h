// #pragma once 头文件保护符，防止头文件在同一个 cpp 重复 include 导致编译报错
#pragma once

// 宏定义 IsBadPtr(_x_)，将 Mem::ValidPtr 封装成宏，取反指针，判断是否为坏指针
#define IsBadPtr(_x_) (Mem::ValidPtr((void*)(_x_)))

// Mem 命名空间：封装内存相关工具函数，全部放在这个命名空间里，防止和其他函数命名冲突
namespace Mem
{
    // ValidPtr：验证指针合法性，PVOID 就是 void* Windows 类型
    bool ValidPtr(PVOID Ptr)
    {
        // 将验证指针强转为 64 位无符号整数，得到指针的内存地址值
        auto v1 = (ULONG64)Ptr;

        // 返回判断值，判断逻辑：
        // v1 < 0x1000000         地址小于 16MB，一般是无效指针（空指针附近）
        // v1 > 0x7FFFFFF00000    地址大于用户态上限，进入内核地址范围，用户态无法访问
        // v1 % sizeof(uint64_t)  指针没有按 8 字节对齐；
        // 满足任一条件返回 true，说明该指针是【非法指针】
        return (bool)(v1 < 0x1000000 || v1 > 0x7FFFFFF00000 || v1 % sizeof(uint64_t));
    }

    // IsBadReadPtr_：模拟 Windows 系统 API IsBadReadPtr，判断一块内存是否【不可读取】
    // lp：要检查的内存起始地址，ucb：要检查的内存字节大小
    BOOL IsBadReadPtr_(void* lp, UINT_PTR ucb)
    {
        // 如果 ValidPtr 返回 true 说明指针本身异常，直接返回 TRUE，该内存不可读
        if (ValidPtr(lp))
            return TRUE;

        // MEMORY_BASIC_INFORMATION 是 Windows 结构体，描述一页内存的属性（权限、状态、大小等）
        MEMORY_BASIC_INFORMATION mbi;

        // VirtualQuery Windows API，查询指定地址的内存页信息，填充到 mbi 结构中
        // 返回 0 表示查询失败，地址无效
        if (VirtualQuery(lp, &mbi, sizeof(mbi)) == 0)
        {
            // 查询失败，判断指针无效，内存不能读
            return TRUE;
        }

        // 判断内存页保护属性
        // PAGE_NOACCESS：完全禁止访问；PAGE_EXECUTE：只能执行，不能读取数据
        if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE)
        {
            // 当前内存页没有读取权限，返回 TRUE，非法指针
            return TRUE;
        }

        // MEM_FREE：内存未分配；MEM_RESERVE：内存已经预留但还没有实际提交，没有实际内存
        if (mbi.State == MEM_FREE || mbi.State == MEM_RESERVE)
        {
            // 内存没有实际分配使用，指针无效
            return TRUE;
        }

        // RegionSize 是当前内存块的实际字节大小
        // 如果要读取的字节 ucb 大于实际内存的大小，说明越界了
        if (ucb > mbi.RegionSize)
        {
            // 要读取的范围超出内存实际大小，判断无效
            return TRUE;
        }

        // 全部校验通过，内存合法可以读取，返回 FALSE（不是坏指针）
        return FALSE;
    }

    // 模板函数 Read<T>：从内存读取数据，Address 是地址，读取 T 类型的数据并返回
    template <typename T>
    T Read(uintptr_t Address)
    {
        // uintptr_t 地址强转为 T 类型指针，直接解引用取值返回
        return *(T*)Address;
    }

    // 模板函数 Write<T>：向内存写入数据，将 Values 值写入到 dwAddress 内存地址
    template<typename T>
    void Write(uintptr_t dwAddress, T Values)
    {
        // 将地址转为 T 类型指针，直接赋值写入内存
        *(T*)dwAddress = Values;
    }

    // ReadBytes：读取原始字节，从 address 地址读取 size 个字节到 buffer 缓冲区
    bool ReadBytes(uintptr_t address, void* buffer, size_t size)
    {
        // 校验：源地址为空、目标缓冲区为空、读取长度为 0，直接返回 false 失败
        if (!address || !buffer || size == 0) return false;

        // 源地址转为 uint8_t* 字节指针，遍历字节不会越界，const 表示只读
        auto src = reinterpret_cast<const uint8_t*>(address);
        // 目标缓冲区转为字节指针
        auto dst = reinterpret_cast<uint8_t*>(buffer);

        // for 循环逐字节拷贝内存数据
        for (size_t i = 0; i < size; i++)
        {
            dst[i] = src[i];
        }
        // 拷贝完成返回 true
        return true;
    }

    // WriteBytes：向指定内存地址写入原始字节
    bool WriteBytes(uintptr_t address, void* buffer, size_t size)
    {
        // 参数合法性校验，任一参数不合法直接返回 false
        if (!address || !buffer || size == 0) return false;

        // src 是目标要修改的内存地址，dst 是数据源缓冲区
        auto src = reinterpret_cast<uint8_t*>(address);
        auto dst = reinterpret_cast<uint8_t*>(buffer);

        DWORD OldProtect = NULL; // 保存内存原来的页保护权限

        // VirtualProtect Windows API，修改内存页属性，将目标改为 PAGE_EXECUTE_READWRITE 可读写可执行
        // 修改成功后返回非 0，同时将旧权限保存到 OldProtect
        if (VirtualProtect(src, size, PAGE_EXECUTE_READWRITE, &OldProtect))
        {
            // 循环逐字节将 buffer 缓冲区数据写入目标内存
            for (size_t i = 0; i < size; i++)
            {
                src[i] = dst[i];
            }
            // 写入之后，将内存页权限恢复为修改之前的 OldProtect，避免破坏游戏内存保护
            VirtualProtect(src, size, OldProtect, &OldProtect);
        }
        return true;
    }

    // ABS：计算相对地址，PE 相对偏移转换为 RVA 指针（用于游戏模块内相对寻址计算）
    // address：基础地址，offset：相对偏移的位置，size：指令长度
    uintptr_t ABS(uintptr_t address, int offset, int size)
    {
        // 判断基础地址有效，不为 NULL
        if (address)
        {
            // address + offset 取出相对偏移值，Read<int> 读取 4 字节偏移量
            // address + size + 偏移值 = 最终绝对内存地址
            uintptr_t addrs = address + size + Read<int>(address + offset);
            return addrs;
        }
        // 地址为空返回 0(NULL)
        return NULL;
    }
}

// Utils 命名空间：通用工具函数库
namespace Utils
{
    // FastCall 模板：调用 __fastcall 调用约定的函数
    // Ret 是返回值类型，Args... 是可变参数列表，__forceinline 强制内联展开，提升性能
    template<typename Ret = void, typename... Args>
    __forceinline Ret FastCall(void* pFunc, Args... args)
    {
        // FuncType 定义 __fastcall 函数指针类型，返回值 Ret，参数 Args...
        typedef Ret(__fastcall* FuncType)(Args...);
        // 将 pFunc 转为函数指针，直接调用，传入参数 args，返回函数返回值
        return reinterpret_cast<FuncType>(pFunc)(args...);
    }

    // 重载版本 FastCall，传入 uint64_t 整数形式的函数地址，转为 void* 后调用上面的函数
    template<typename Ret = void, typename... Args>
    __forceinline Ret FastCall(uint64_t pFunc, Args... args)
    {
        typedef Ret(__fastcall* FuncType)(Args...);
        return FastCall<Ret>((void*)pFunc, args...);
    }

    // WCHAR2String：宽字符串 wstring(UTF16) 转换为窄字节 std::string(ANSI CP_ACP)
    std::string WCHAR2String(const std::wstring& wstrSrc)
    {
        // 如果源字符串为空，直接返回空字符串
        if (wstrSrc.empty()) return "";

        // WideCharToMultiByte Windows API，宽字符转多字节
        // CP_ACP 使用系统默认 ANSI 代码页，-1 自动处理字符串末尾 '\0' 结束符
        // 第一次调用传入 nullptr，只计算需要多少缓冲长度，不实际转换
        int nLen = WideCharToMultiByte(CP_ACP, 0,
            wstrSrc.c_str(), -1,
            nullptr, 0, nullptr, nullptr);

        // nLen<=0 转换失败，返回空
        if (nLen <= 0) return "";

        // 创建 string 对象，容量为 nLen-1，去掉末尾自动附带的 '\0' 占位
        std::string strDest(nLen - 1, '\0');

        // 再次调用 WideCharToMultiByte，真正执行转换，结果写入 strDest.data() 缓冲区
        WideCharToMultiByte(CP_ACP, 0,
            wstrSrc.c_str(), -1,
            strDest.data(), nLen, nullptr, nullptr);

        // 返回转换完成的普通字符串
        return strDest;
    }
}