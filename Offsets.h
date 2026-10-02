// #pragma once 头文件保护符，防止被 #include 时重复包含导致编译报错
#pragma once

// Offsets 命名空间：专门存储游戏引擎的函数地址和成员偏移值（uint64_t 64 位地址/偏移）
// 这些值需要在内存中赋值，一般是运行时从游戏模块基址加上这些偏移值
namespace Offsets
{
    // StaticFindObject：UE 核心的查找函数，通过名字查找 UObject，最常用函数之一
    uint64_t StaticFindObject;

    // GetName：UObject 的成员函数，获取对象名称（FName）
    uint64_t GetName;

    // FreeFunc：内存释放函数，用来释放 TArray 等分配的内存
    uint64_t FreeFunc;

    // GEngine：全局 UE 引擎实例指针，整个游戏运行的全局对象，几乎所有数据从 GEngine 获取
    uint64_t GEngine;

    // GameViewport：游戏视口，渲染窗口、界面管理对象，从 GEngine 获取
    uint64_t GameViewport;

    // World：UWorld 对象，游戏世界实例，包含关卡、Actor 实体、玩家等全部游戏世界内容
    uint64_t World;

    // Levels：UWorld 下的关卡数组，存储当前加载的多个 Level 关卡
    uint64_t Levels;

    // GameState：游戏全局 GameState 对象，保存全局状态、模式等信息
    uint64_t GameState;

    // DFMGamePlayerMode：游戏自定义的游戏模式类，保存当前游戏进行中的模式，非 UE 原生
    uint64_t DFMGamePlayerMode;

    // PlayerController：玩家控制器，处理玩家输入、控制 Pawn 实体，每个玩家对应一个 PlayerController
    uint64_t PlayerController;

    // PlayerCameraManager：玩家相机管理器，控制相机位置、FOV 视角、镜头抖动等
    uint64_t PlayerCameraManager;

    // PlayerState：玩家状态类，存储玩家分数、ID、阵营等共享状态信息
    uint64_t PlayerState;

    // PlayerNamePrivate：PlayerState 内部私有成员偏移，存储玩家名字字符串
    uint64_t PlayerNamePrivate;

    // RootComponent：Actor 的根组件，决定 Actor 的坐标变换、位置、旋转等 Transform
    uint64_t RootComponent;

    // DFMOutLineEffectComponent：游戏自定义的轮廓效果组件，实现敌人/队友轮廓高亮透视功能
    uint64_t DFMOutLineEffectComponent;
}