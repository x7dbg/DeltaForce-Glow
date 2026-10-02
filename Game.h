#pragma once

namespace GameData
{
    UWorld* GWorld = nullptr;                  // 当前 UWorld 世界对象指针
    UObject* Engine;                           // GEngine 全局引擎对象
    UBlueprintFunctionLibrary* BlueprintFunctionLibrary[10] = { nullptr }; // 预分配蓝图库，下标对应 BlueprintType 枚举
    AGPCharacter* MyCharacter = nullptr;       // 本地玩家控制角色指针
    bool bIsInGame = false;                    // 是否在对局游戏中，不在大厅
    bool bIsCampMode = false;                  // 是否在营队/阵营模式
    uint32_t TeamIndex = NULL;                 // 本地玩家队伍号，用于对比判断敌友，过滤队友 ESP
}