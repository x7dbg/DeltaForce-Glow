// #pragma once 头文件保护符，防止重复包含
#pragma once

// VTableHook 虚表 Hook 功能类，用于劫持 C++ 类的虚函数实现 Hook
// 原理：C++ 类的虚函数的开头有一个虚表指针（*vptr），指向虚函数表数组；
// 拷贝一份原始虚表，修改这个复制的虚表中的函数指针，将目标 vptr 指向这个复制的虚表，达到 Hook 目的
class VTableHook
{
public:
    // Initialize：初始化 Hook 对象
    // pTarget：目标类的实例的地址（对象实例指针，不是类类型指针）
    void Initialize(void* pTarget)
    {
        // 将目标对象指针保存到成员变量 Target
        Target = pTarget;

        // *(void***)pTarget 获取对象头部的虚表指针 vptr
        // oVTable = original VTable，保存原始虚表数组地址
        oVTable = *(void***)pTarget;

        // 调用 CalcVTableSize 函数计算虚表中一共有多少个函数指针
        uint32_t Size = CalcVTableSize();

        // 在堆上分配一块内存，用来保存我们自己的虚表，大小为 Size 个 void* 指针
        VTable = new void* [Size];

        // memcpy 将原始虚表 oVTable 完全拷贝到新建的复制虚表 VTable
        memcpy(VTable, oVTable, Size * sizeof(void*));

        // 修改目标对象头部的虚表指针 vptr，使其指向我们复制出来的新虚表 VTable
        // 此后调用虚函数时，就会走这个复制的虚表，而不是原始的虚表
        *(void***)pTarget = VTable;
    }

    // Bind：Hook 指定下标的虚函数
    // Index：虚函数在虚表中的序号，Function：我们自己写的挂钩函数地址
    void Bind(uint32_t Index, void* Function)
    {
        // 将虚表对应位置替换为我们的函数地址，完成劫持
        VTable[Index] = Function;
    }

    // UnBind：取消某个函数的 Hook，恢复为原始函数
    void UnBind(uint32_t Index)
    {
        // 将复制虚表该位置重新赋值为原始虚表的函数指针
        VTable[Index] = oVTable[Index];
    }

    // UnAllBind：完全取消 Hook，恢复原始状态并释放堆分配的复制虚表内存
    void UnAllBind()
    {
        // delete[] 释放 new[] 分配的复制虚表的内存，防止内存泄漏
        delete[] VTable;

        // 将目标对象的虚表指针指回最开始的原始虚表 oVTable，完全恢复原始状态
        *(void***)Target = oVTable;
    }

    // 模板函数 GetOriginal：获取原始未 Hook 的函数地址
    // T 是函数指针类型，Index 是虚表数组的原始位置的函数指针
    template <typename T>
    T GetOriginal(uint32_t Index)
    {
        // 从原始虚表 oVTable 取出函数指针，强转为 T 类型返回，用来调用原始函数
        return (T)oVTable[Index];
    }

private:
    // CalcVTableSize：计算虚表有多少个函数指针
    uint32_t CalcVTableSize()
    {
        uint32_t i = 0;
        // 循环遍历原始虚表数组，直到遇到 NULL 指针为止（虚表末尾以 NULL 作为结束标记）
        while (oVTable[i] != NULL)
        {
            i++;
        }
        // 返回虚表总函数指针数量
        return i;
    }

    void* Target = NULL;    // 保存被 Hook 的目标对象实例地址
    void** VTable = NULL;   // 我们自己创建可修改的复制虚表（void** 指针数组）
    void** oVTable = NULL;  // original VTable，保存原始虚表地址
};

// inline namespace Hook 内联命名空间，里面的变量相当于可以直接 Hook::RenderHook 访问
inline namespace Hook
{ 
    // 全局 VTableHook 实例 RenderHook，专门用来 Hook 渲染相关虚函数（UE 游戏用来 Hook 渲染函数）
    VTableHook RenderHook;
}