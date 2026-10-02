# DeltaForce-Glow

三角洲行动（Delta Force）游戏内部 DLL 注入式辅助工具，基于虚幻引擎（UE）VTable Hook 技术实现，通过修改游戏轮廓效果系统实现敌方透视高亮功能。

## 项目概述

这是一个针对三角洲行动（Delta Force）游戏的 Windows DLL 项目，通过注入游戏进程并 Hook 渲染管线，实现对敌方角色（包括 AI 和玩家）的轮廓高亮显示。项目采用 C++ 开发，使用 UE 引擎内部机制和内存操作技术。

### 工作原理

本工具通过以下技术路径实现功能：

1. **DLL 注入**：将编译后的 DLL 注入到游戏进程中
2. **隐藏自身**：通过移除 LDR 链表、擦除 PE 头、重映射内存三重技术隐藏 DLL
3. **VTable Hook**：劫持 `UGameViewportClient::PostRender` 函数（虚表索引 95）
4. **实体遍历**：每帧调用 `GetAllActorsOfClass` 获取所有角色
5. **轮廓应用**：通过游戏原生的 `UGPOutLineEffectComponent::PlayOutLineEffect` 施加高亮效果

### 技术特点

- **纯内部实现**：直接读取游戏内存，无需外部驱动或 DMA 硬件
- **利用游戏原生系统**：不绘制任何覆盖层，完全使用游戏内置轮廓系统
- **动态偏移解析**：通过 UE UObject 反射系统动态查找成员偏移，减少版本更新维护成本
- **多模式支持**：自动识别游戏模式并适配队伍/阵营判断逻辑

## 核心功能

### 1. 轮廓高亮系统（Glow ESP）

通过修改角色的 `UGPOutLineEffectComponent` 组件，对敌方施加不同的轮廓效果：

| 目标类型 | 轮廓效果 | 枚举值 | 视觉效果 |
|---------|---------|--------|---------|
| AI 敌人 | `OutLineType_DyingShowTeammateCanRescueSelf` | 128 | 红色高亮（可救援提示） |
| 玩家敌人 | `OutLineType_ProxSensor` | 1 | 黄色高亮（传感器探测） |
| 队友 | 不施加 | - | 无效果 |
| 自己 | 不施加 | - | 无效果 |

**优势**：
- 使用游戏原生渲染管线，难以被反作弊检测
- 轮廓效果穿透墙壁，实现透视功能
- 自动过滤已死亡角色和队友

### 2. 反检测保护

采用三重 DLL 隐藏技术：

#### 2.1 LDR 链表移除（`HideDll::RemoveLDR`）

从 PEB（进程环境块）的三个模块列表中移除自身 DLL 节点：

```
InLoadOrderModuleList      - 按加载顺序排列
InMemoryOrderModuleList    - 按内存地址排列
InInitializationOrderModuleList - 按初始化顺序排列
```

**效果**：`EnumProcessModules`、`CreateToolhelp32Snapshot` 等 API 无法枚举到该 DLL

#### 2.2 PE 头擦除（`HideDll::RemovePEH`）

将 DLL 内存前 0x1000 字节全部清零：

```
偏移 0x000-0x001: MZ 签名 (4D 5A) → 清零
偏移 0x03C-0x03F: e_lfanew → 清零
偏移 0x000-0xFFF: 整个 PE 头 → 清零
```

**效果**：内存扫描工具无法识别为有效 PE 模块

#### 2.3 内存映射伪装（`HideDll::RemoveMAP`）

使用 Nt API 和 Shellcode 技术：

1. 将 DLL 内存完整备份到堆内存
2. 调用 `ZwUnmapViewOfSection` 卸载原始映射
3. 调用 `ZwAllocateVirtualMemory` 重新分配内存
4. 将备份数据复制回新映射区域

**效果**：`VirtualQuery` 无法检测到原始 DLL 内存映射

### 3. 动态偏移解析

通过 UE 引擎的 UObject 反射系统自动查找成员偏移：

```cpp
// FindOffset 工作原理
1. 使用 StaticFindObject 查找 UStruct（如 "Engine.Actor"）
2. 遍历 ChildProperties 链表获取所有成员属性
3. 比对成员名称（如 "RootComponent"）
4. 返回匹配的 Offset 值
```

**维护成本**：
- 游戏小更新：无需修改任何偏移
- 游戏大更新：仅需更新 4 个关键 RVA

## 项目结构

```
DeltaForce-Glow/
│
├── main.cpp                    # DLL 入口点（DllMain）
│   ├── 初始化 DLL 隐藏
│   └── 创建 Hack 线程
│
├── Includes.h                  # 统一头文件包含
│   ├── Windows/C++ 标准库
│   ├── Debug 宏定义
│   └── 所有自定义头文件
│
├── Enum.h                      # 游戏枚举类型
│   ├── BlueprintType           # 蓝图函数库类型
│   ├── EDFMGamePlayMode        # 游戏模式枚举
│   ├── EOutLineEffectType      # 轮廓效果类型
│   └── EMainFlowState          # 主流程状态
│
├── Offsets.h                   # 内存偏移量命名空间
│   ├── StaticFindObject        # RVA - 对象查找函数
│   ├── GetName                 # RVA - 名称获取函数
│   ├── FreeFunc                # RVA - 内存释放函数
│   ├── GEngine                 # RVA - 全局引擎指针
│   └── 动态解析的成员偏移
│
├── Engine.h                    # UE 引擎数据结构封装
│   ├── TArray<T>               # UE 动态数组
│   ├── FVector / FVector2D     # 向量数学结构
│   ├── FName                   # 名称结构（Index+Number）
│   ├── UObject                 # 基类（IsA/ProcessEvent/FindObject）
│   ├── UStruct / UClass        # 结构/类对象
│   ├── UWorld                  # 世界对象
│   ├── UGameViewportClient     # 视口客户端
│   ├── AActor / APawn          # Actor 基类
│   ├── ADFMCharacter           # 游戏角色类
│   ├── UGPHealthDataComponent  # 血量组件
│   └── UGPOutLineEffectComponent # 轮廓效果组件
│
├── Entity.h                    # 实体管理逻辑
│   ├── GetIsCamp()             # 判断是否为阵营模式
│   └── UpData()                # 每帧更新核心逻辑
│
├── Game.h                      # 游戏全局数据命名空间
│   ├── GWorld                  # 当前世界指针
│   ├── Engine                  # GEngine 指针
│   ├── BlueprintFunctionLibrary # 蓝图函数库缓存
│   ├── MyCharacter             # 本地玩家角色
│   ├── bIsInGame               # 是否在局内
│   ├── bIsCampMode             # 是否阵营模式
│   └── TeamIndex               # 队伍/阵营 ID
│
├── Hack.h                      # C_Hack 核心类
│   ├── hkPostRender()          # Hook 后的渲染回调
│   ├── FindOffset()            # 动态偏移查找
│   ├── InitOffsets()           # 初始化所有偏移
│   ├── InitHooks()             # 初始化 VTable Hook
│   ├── Init()                  # 旧版初始化（HOME 键触发）
│   └── Init2()                 # 新版初始化（含热键轮询）
│
├── Hooks.h                     # VTable Hook 实现
│   ├── Initialize()            # 复制虚表并重定向 vptr
│   ├── Bind()                  # 替换指定索引函数
│   ├── UnBind()                # 恢复单个函数
│   ├── UnAllBind()             # 完全恢复
│   └── GetOriginal<T>()        # 获取原函数指针
│
├── Utils.h                     # 工具函数命名空间
│   ├── Mem::ValidPtr()         # 指针有效性验证
│   ├── Mem::Read<T>()          # 内存读取
│   ├── Mem::Write<T>()         # 内存写入
│   ├── Mem::ReadBytes()        # 字节块读取
│   ├── Mem::WriteBytes()       # 字节块写入（含页保护修改）
│   ├── Mem::ABS()              # RVA 转绝对地址
│   ├── Utils::FastCall<T>()    # __fastcall 调用封装
│   └── Utils::WCHAR2String()   # 宽字符转字符串
│
├── Hidedll.h                   # DLL 隐藏类
│   ├── RemoveLDR()             # 移除 LDR 链表节点
│   ├── RemovePEH()             # 擦除 PE 头部
│   ├── RemoveMAP()             # 重映射内存
│   └── GetModuleLen()          # 获取模块大小
│
├── 上层DMA_SDK偏移与布局_游戏更新维护指南.H
│   └── 游戏更新时的偏移维护指南
│
└── x64/                        # 编译输出目录
    └── Release/
        └── DeltaForce-Glow.dll # 最终 DLL 文件
```

## 技术架构

### 完整初始化流程

```
DllMain (DLL_PROCESS_ATTACH)
│
├── DisableThreadLibraryCalls()     // 禁用线程通知，减少 DllMain 调用
│
├── HideDll DLL(hModule)           // 创建隐藏对象
│   ├── DLL.RemoveLDR()            // ① 从 PEB LDR 链表移除节点
│   ├── DLL.RemoveMAP()            // ② 卸载并重新映射内存
│   └── DLL.RemovePEH()            // ③ 清零 PE 头部 0x1000 字节
│
└── CreateThread → C_Hack::Init2() // 创建 Hack 工作线程
    │
    ├── 定义变量
    │   ├── 原始字节数组 [5]        // 保存补丁位置的原始指令
    │   ├── 功能开关状态            // F1 切换状态
    │   └── 目标地址 0x14199E3A5   // 需要 Patch 的指令位置
    │
    ├── 等待 HOME 键按下
    │   └── GetAsyncKeyState(VK_HOME) 循环检测
    │
    ├── Beep(800, 500)             // 提示音表示初始化开始
    │
    ├── 读取目标地址原始 5 字节     // 保存以便恢复
    │
    ├── InitOffsets()              // 初始化所有偏移量
    │   ├── 设置 4 个关键 RVA
    │   ├── 动态查找成员偏移
    │   ├── 缓存蓝图函数库
    │   └── 获取 GEngine 指针
    │
    ├── InitHooks()                // 初始化 VTable Hook
    │   ├── 获取 GameViewportClient
    │   ├── RenderHook.Initialize()
    │   └── RenderHook.Bind(95, hkPostRender)
    │
    └── 主轮询循环 (10ms 间隔)
        └── 检测 F1 键
            ├── 开启：写入 Patch 字节 {0xF3, 0x0F, 0x10, 0x40, 0x3C}
            └── 关闭：恢复原始字节
```

### VTable Hook 机制

```cpp
// 1. 获取目标对象的虚表指针
void** oVTable = *(void***)pTarget;

// 2. 计算虚表大小（遍历直到 NULL）
uint32_t Size = CalcVTableSize();

// 3. 在堆上分配新内存并复制虚表
void** VTable = new void*[Size];
memcpy(VTable, oVTable, Size * sizeof(void*));

// 4. 重定向对象的 vptr 到新虚表
*(void***)pTarget = VTable;

// 5. 替换指定索引的函数指针
VTable[95] = hkPostRender;  // PostRender 被劫持

// 6. 调用原函数（在 Hook 函数中）
Utils::FastCall<void>(RenderHook.GetOriginal<void*>(95), GameViewport, Canvas);
```

### 每帧执行流程（hkPostRender → UpData）

```cpp
C_Entity::UpData() 每帧执行：
│
├── ① 获取主流程状态
│   └── GPScalabilityBlueprintTools::GetMainFlowState()
│       ├── Lobby (1)      → 不在局内，返回
│       ├── Loading (2)    → 不在局内，返回
│       ├── SafeHouse (3)  → 在局内，继续
│       └── InGame (4)     → 在局内，继续
│
├── ② 获取 UWorld
│   ├── GEngine + GameViewport 偏移 → GameViewportClient
│   └── GameViewportClient + World 偏移 → UWorld
│
├── ③ 获取 GameState
│   └── GameplayBlueprintHelper::GetGPGameState(GWorld)
│
├── ④ 识别游戏模式
│   ├── GameState + DFMGamePlayerMode 偏移 → 模式枚举
│   └── GetIsCamp() 判断是否使用阵营系统
│
├── ⑤ 获取本地玩家角色
│   └── GameplayBlueprintHelper::GetLocalGPCharacter(GWorld)
│
├── ⑥ 获取队伍/阵营 ID
│   ├── MyCharacter → GetTeamComp()
│   ├── 阵营模式 → GetCamp()
│   └── 非阵营模式 → GetTeamID()
│
├── ⑦ 遍历所有角色
│   ├── GameplayStatics::GetAllActorsOfClass(ADFMCharacter)
│   │
│   └── 对每个角色：
│       ├── 过滤自己 (pCharacter == MyCharacter)
│       ├── 过滤死亡角色 (!IsAlive())
│       ├── 获取队伍/阵营 ID
│       ├── 过滤队友 (TeamIndex == GameData::TeamIndex)
│       │
│       └── 对敌人应用轮廓：
│           ├── 获取 DFMOutLineEffectComponent
│           ├── 判断是否 AI (IsAI())
│           │   ├── AI → OutLineType_DyingShowTeammateCanRescueSelf (128)
│           │   └── 玩家 → OutLineType_ProxSensor (1)
│           ├── 读取当前效果 (偏移 0x1D8)
│           └── 如果不同 → PlayOutLineEffect(desiredEffect)
│
└── ⑧ 释放 TArray 内存
    └── Utils::FastCall(Offsets::FreeFunc, ACharacters.Data)
```

### 内存操作工具

```cpp
namespace Mem {
    // 指针验证：检查地址范围和 8 字节对齐
    bool ValidPtr(PVOID Ptr) {
        return v1 < 0x1000000 || v1 > 0x7FFFFFF00000 || v1 % 8;
    }

    // 模板读取：直接解指针
    template<typename T>
    T Read(uintptr_t Address) {
        return *(T*)Address;
    }

    // 模板写入：直接赋值
    template<typename T>
    void Write(uintptr_t Address, T Value) {
        *(T*)Address = Value;
    }

    // 字节写入：修改页保护后写入
    bool WriteBytes(uintptr_t address, void* buffer, size_t size) {
        VirtualProtect(src, size, PAGE_EXECUTE_READWRITE, &OldProtect);
        // 逐字节写入
        VirtualProtect(src, size, OldProtect, &OldProtect);  // 恢复保护
    }

    // RVA 转绝对地址：处理 x64 相对寻址
    uintptr_t ABS(uintptr_t address, int offset, int size) {
        return address + size + Read<int>(address + offset);
    }
}
```

## 编译说明

### 环境要求

| 项目 | 要求 |
|------|------|
| **编译器** | MSVC (Visual Studio 2019/2022) |
| **平台** | Windows x64 |
| **C++ 标准** | C++20（使用 std::format） |
| **项目类型** | 动态链接库 (.dll) |
| **字符集** | Unicode |
| **依赖项** | Windows SDK（无第三方库） |

### 编译步骤

1. 使用 Visual Studio 打开项目文件 `DeltaForce-Glow.vcxproj`
2. 选择配置：
   - **Debug | x64**：带调试输出，用于开发测试
   - **Release | x64**：优化版本，用于实际使用
3. 生成解决方案（Ctrl+Shift+B）
4. 输出文件位置：
   - Debug：`x64/Debug/DeltaForce-Glow.dll`
   - Release：`x64/Release/DeltaForce-Glow.dll`

### 项目配置详情

```xml
<!-- 关键编译配置 -->
<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>    <!-- /MD -->
<LanguageStandard>stdcpp20</LanguageStandard>        <!-- C++20 -->
<Optimization>MaxSpeed</Optimization>                <!-- /O2 (Release) -->
<CharacterSet>Unicode</CharacterSet>                 <!-- /D_UNICODE -->
<PlatformToolset>v142/v143</PlatformToolset>         <!-- VS2019/VS2022 -->
```

### 调试模式

在 [Includes.h](file:///c:/Users/Mr.Xiao/Desktop/新建文件夹/DeltaForce-Glow/Includes.h) 中取消注释可开启控制台输出：

```cpp
#define DEBUG_ENABLE   // 取消此行注释
```

开启后会：
- 自动分配控制台窗口
- 重定向 stdin/stdout 到控制台
- 输出所有偏移量查找结果
- 输出蓝图函数库地址

## 使用说明

### 快捷键

| 按键 | 功能 | 说明 |
|------|------|------|
| **HOME** | 启动 Hack 初始化 | 注入后首次按下触发，播放提示音表示开始 |
| **F1** | 切换轮廓高亮 开/关 | 切换 Patch 状态，开启后敌方显示高亮轮廓 |

### 注入方式

使用任意 DLL 注入工具将编译后的 `DeltaForce-Glow.dll` 注入到游戏进程：

1. 启动三角洲行动游戏
2. 使用注入工具（如 Process Hacker、Cheat Engine 等）
3. 选择游戏进程（通常为 `DeltaForceClient.exe` 或类似名称）
4. 注入 `x64/Release/DeltaForce-Glow.dll`
5. 进入游戏对局后按下 **HOME** 键初始化
6. 按 **F1** 键开启轮廓高亮

### 热键 Patch 机制

F1 键通过修改游戏代码实现功能开关：

```cpp
// 目标地址：0x140000000 + 0x199E3A5 = 0x14199E3A5
// 开启时写入：{ 0xF3, 0x0F, 0x10, 0x40, 0x3C }
// 对应汇编：movss xmm0, dword ptr [rax+0x3C]
// 关闭时：恢复原始 5 字节
```

## 偏移更新指南

### 需要更新的偏移

游戏大版本更新后，需要在 [Hack.h](file:///c:/Users/Mr.Xiao/Desktop/新建文件夹/DeltaForce-Glow/Hack.h) 的 `InitOffsets()` 中更新以下 4 个 RVA：

```cpp
// 这 4 个值需要使用 IDA Pro 或内存扫描工具重新获取
Offsets::StaticFindObject = 0x14AE8C310;  // UObject::StaticFindObject
Offsets::GetName = 0x14AB2CA30;           // FName::GetName
Offsets::FreeFunc = 0x14A9F0B90;          // 内存释放函数
Offsets::GEngine = 0x153A3D2A8;           // 全局 GEngine 指针
```

### 获取方法

1. 使用 IDA Pro 加载游戏主程序
2. 搜索函数名或特征码定位函数地址
3. 计算 RVA = 函数地址 - 模块基址 (0x140000000)
4. 或使用内存扫描工具在运行时读取

### 无需更新的偏移

以下偏移通过 `FindOffset()` 动态获取，游戏小更新无需修改：

```cpp
// 这些通过 UObject 反射系统自动查找
Offsets::GameViewport          // "Engine.Engine" -> "GameViewport"
Offsets::World                 // "Engine.GameViewportClient" -> "World"
Offsets::Levels                // "Engine.World" -> "Levels"
Offsets::GameState             // "Engine.World" -> "GameState"
Offsets::PlayerController      // "Engine.Player" -> "PlayerController"
Offsets::RootComponent         // "Engine.Actor" -> "RootComponent"
// ... 等等
```

## 游戏模式支持

| 模式 | 枚举值 | 队伍判断方式 | 说明 |
|------|--------|-------------|------|
| **None** | 0 | - | 未定义 |
| **决战（SOL）** | 1 | 队伍 ID | 大战场模式 |
| **全面行动** | 2 | 队伍 ID | PVE 模式 |
| **烽火地带** | 3 | 队伍 ID | 撤离模式 |
| **全面征服** | 4 | 队伍 ID | 占点模式 |
| **突破模式** | 5 | **阵营 ID** | 攻防模式，使用 Camp 系统 |
| **安全屋** | 6 | - | 基地/营地区域 |
| ** intro** | 7 | - | 开场动画 |

### 阵营模式说明

突破模式（Breakthrough）使用阵营系统而非队伍系统：

```cpp
// 阵营模式判断
bool GetIsCamp(EDFMGamePlayMode mode) {
    return mode == EDFMGamePlayMode::GamePlayMode_Breakthrough;
}

// 获取标识
if (bIsCampMode)
    TeamIndex = TeamComp->GetCamp();    // 获取阵营（攻方/守方）
else
    TeamIndex = TeamComp->GetTeamID();  // 获取队伍 ID
```

## 核心数据结构

### UObject 层次结构

```
UObject (基类)
├── VFTable              // 虚函数表指针
├── ClassPrivate         // UClass 对象
├── OuterPrivate         // 外部对象
├── InternalIndex        // 内部索引
└── NamePrivate          // FName 名称
    ├── Index            // 名称表索引
    └── Number           // 编号

UClass : UStruct : UField : UObject
├── SuperStruct          // 父结构
└── ChildProperties      // 成员属性链表

UWorld : UObject         // 世界对象，包含所有 Actor
UGameViewportClient : UObject  // 视口客户端，渲染入口
```

### 角色类层次

```
AActor : UObject
├── RootComponent        // 根组件（位置/旋转）
└── ...

APawn : AActor           // 可控制 Actor

ADFMCharacter : APawn    // 游戏角色类
├── DFMOutLineEffectComponent  // 轮廓效果组件
├── HealthDataComponent        // 血量组件
├── TeamComp                   // 队伍组件
└── PlayerState                // 玩家状态
```

### 轮廓效果枚举

```cpp
enum class EOutLineEffectType : int32_t {
    OutLineType_None = 0,
    OutLineType_ProxSensor = 1,                    // 传感器探测（黄色）
    OutLineType_ArrowMark = 2,                     // 箭头标记
    OutLineType_MedicGun = 4,                      // 医疗枪瞄准
    OutLineType_DyingLockEnemy = 32,               // 倒地锁定敌人
    OutLineType_Breakthrough_Defender = 64,        // 突破模式防守方
    OutLineType_DyingShowTeammateCanRescueSelf = 128, // 倒地救援提示（红色）
    OutLineType_Locking = 512,                     // 锁定中
    OutLineType_LockComplete = 1024,               // 锁定完成
    // ... 更多效果
};
```

## 常见问题

### Q: 注入后没有效果？

A: 检查以下几点：
1. 确认 DLL 已成功注入（使用 Process Explorer 查看模块列表，但隐藏后可能看不到）
2. 进入游戏对局后按下 **HOME** 键，听是否有提示音
3. 开启 `DEBUG_ENABLE` 查看控制台输出
4. 确认偏移量与当前游戏版本匹配

### Q: 游戏更新后如何获取新偏移？

A: 参考项目中的 `上层DMA_SDK偏移与布局_游戏更新维护指南.H` 文件，使用 VT USB 探测工具获取新的 RVA 值。

### Q: 如何添加新功能（如 AimBot）？

A: 在 `hkPostRender` 或 `UpData` 中添加逻辑：

```cpp
static void hkPostRender(void* GameViewport, UCanvas* Canvas)
{
    C_Entity::UpData();
    
    // 在这里添加新功能
    // C_AimBot::Update();
    // C_ESP::Render(Canvas);
    
    return Utils::FastCall<void>(RenderHook.GetOriginal<void*>(95), GameViewport, Canvas);
}
```

### Q: 为什么使用轮廓系统而不是绘制 ESP？

A: 优势包括：
- **隐蔽性**：使用游戏原生渲染，不创建额外窗口或覆盖层
- **性能**：不需要手动计算屏幕坐标和绘制线条
- **稳定性**：不修改游戏渲染管线，仅修改数据
- **穿透性**：轮廓效果天然穿透墙壁

### Q: TArray 内存泄漏问题？

A: `GetAllActorsOfClass` 返回的 TArray 必须手动释放：

```cpp
TArray<AActor*> ACharacters = {};
GameData::BlueprintFunctionLibrary[...]->GetAllActorsOfClass(..., &ACharacters);

// 使用完毕后必须释放
if (ACharacters.Data)
    Utils::FastCall(Offsets::FreeFunc, ACharacters.Data);
```

## 技术参考

### UE 引擎相关

- **虚幻引擎版本**：UE4/UE5（通过 GObjects/GNames 结构判断）
- **对象查找**：`StaticFindObject` / `FindObject`
- **事件处理**：`ProcessEvent`（虚表索引 0x44）
- **反射系统**：`UStruct::ChildProperties` 链表遍历

### Windows API 使用

| API | 用途 |
|-----|------|
| `CreateThread` | 创建工作线程 |
| `GetAsyncKeyState` | 键盘状态检测 |
| `VirtualProtect` | 修改内存页保护 |
| `VirtualQuery` | 查询内存区域信息 |
| `ReadProcessMemory` | 读取进程内存 |
| `WriteProcessMemory` | 写入进程内存 |
| `GetProcAddress` | 获取函数地址 |
| `LoadLibrary` | 加载 DLL |
| `AllocConsole` | 分配控制台 |
| `Beep` | 播放提示音 |

### Nt API 使用

| API | 用途 |
|-----|------|
| `ZwUnmapViewOfSection` | 卸载内存映射 |
| `ZwAllocateVirtualMemory` | 分配虚拟内存 |

## 依赖项

- **Windows SDK**：Windows API 头文件和库
- **Visual C++ Runtime**：C++ 标准库
- **无第三方库**：项目完全使用原生 API，无外部依赖

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0 | - | 初始版本，基础轮廓高亮功能 |

## 免责声明

⚠️ **重要声明**

本项目仅供学习和研究使用，旨在了解以下内容：
- Windows DLL 注入技术
- UE 引擎内部机制
- VTable Hook 原理
- 内存操作技术

**使用本软件进行游戏作弊可能：**
- 违反游戏服务条款
- 导致账号被封禁
- 影响其他玩家的游戏体验

请遵守游戏规则，合理使用软件。开发者不对任何滥用行为负责。

## 相关资源

- [上层DMA_SDK偏移与布局_游戏更新维护指南.H](file:///c:/Users/Mr.Xiao/Desktop/新建文件夹/DeltaForce-Glow/上层DMA_SDK偏移与布局_游戏更新维护指南.H) - 偏移维护指南
- [虚幻引擎官方文档](https://docs.unrealengine.com/)
- [Windows API 文档](https://docs.microsoft.com/en-us/windows/win32/api/)