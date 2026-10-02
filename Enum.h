#pragma once

enum class BlueprintType : int32_t
{
    GameplayStatics,                // 0 GameplayStatics
    KismetTextLibrary,              // 1 KismetTextLibrary
    KismetStringLibrary,            // 2 KismetStringLibrary
    GameplayBlueprintHelper,        // 3 GameplayBlueprintHelper
    GPScalabilityBlueprintTools,    // 4 GPScalabilityBlueprintTools
    KismetSystemLibrary,            // 5 KismetSystemLibrary
    BlueprintFunctionLibrary,       // 6 BlueprintFunctionLibrary
    Max                             // 7 最大值边界
};

/**
 * @brief EDFMGamePlayMode 三角洲行动对局模式枚举，占 1 字节 (uint8_t)
 */
enum class EDFMGamePlayMode : uint8_t  // Size 9
{
    None = 0,
    GamePlayMode_SOL = 1,           // 决战模式/大战场 沙漠行动 SOL 模式
    GamePlayMode_Raid = 2,          // 全面行动 / 烽火撤离（PVE）
    GamePlayMode_IrisDiscovery = 3, // 烽火地带
    GamePlayMode_Conquest = 4,      // 占点模式
    GamePlayMode_Breakthrough = 5,  // 突破模式（攻防）
    GamePlayMode_SafeHouse = 6,     // 安全屋/营地安全区
    GamePlayMode_Intro = 7,         // 开场动画 CG
    EDFMGamePlayMode_MAX = 8
};

/**
 * @brief EOutLineEffectType 角色轮廓效果类型（位掩码）
 * 游戏原生的轮廓系统，使用位运算组合效果
 */
enum class EOutLineEffectType : int32_t
{
    OutLineType_None = 0,
    OutLineType_ProxSensor = 1,                    // 传感器探测高亮
    OutLineType_ArrowMark = 2,                      // 箭头标记目标
    OutLineType_MedicGun = 4,                       // 医疗枪瞄准提示
    OutLineType_MedicGunHit = 8,                    // 医疗枪命中
    OutLineType_SupportEffect = 16,                 // 支援特效高亮
    OutLineType_DyingLockEnemy = 32,                // 倒地锁定敌人
    OutLineType_Breakthrough_Defender = 64,         // 突破模式防守方
    OutLineType_DyingShowTeammateCanRescueSelf = 128,// 倒地：可被队友救援提示
    OutLineType_DyingShowCampMedicCanRescueSelf = 256,
    OutLineType_Locking = 512,                      // 锁定进行中
    OutLineType_LockComplete = 1024,                // 锁定完成
    OutLineType_LockFire = 2048,                    // 锁定开火
    OutLineType_OnlyMarker = 4096,                  // 仅标记，无实际高亮
    OutLineType_SOL_OB_Teammate = 8192,             // SOL 决战 OB 队友
    OutLineType_SOL_OB_Enemy = 16384,               // SOL 决战 OB 敌人
    OutLineType_SOL_OB_AI = 32768,                  // SOL 决战 OB AI
    OutLineType_SOL_OB_FreeCamera = 65536,
    OutLineType_Breakthrough_OB_Attacker = 131072,  // 突破模式 OB 进攻方
    OutLineType_OBorReplay = 262144,                // OB 观战/回放模式
    OutLineType_MAX = 262145
};

/**
 * @brief EMainFlowState 主流程状态，标识大厅、加载、对局
 */
enum class EMainFlowState : uint8_t
{
    Default = 0,
    Lobby = 1,          // 大厅界面
    Loading = 2,        // 加载界面
    SafeHouse = 3,      // 安全屋/基地
    InGame = 4,         // 在对局中
    LobbyBHD = 5,       // 备战大厅
    EMainFlowState_MAX = 6
};