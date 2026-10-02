# 上层 DMA SDK 偏移与布局：游戏更新维护指南

> 本文是给后续 AI 直接执行的工程维护指令。目标是：游戏更新后，只通过 VT USB 探针重新获取并验证上层 DMA 所需的 SDK 偏移、结构布局和指针链，再按依赖顺序同步到正式源码。本文不负责通用明文入口更新；入口变化应同时读取同目录的 `通用明文坐标全量捕获_游戏更新维护指南.md`。

## 1. 硬性约束

1. 只处理项目目录：`C:\Users\Administrator\Desktop\双机内存全套_交付版`。
2. **禁止使用上层 DMA 软件定位偏移、扫描内存、试探结构或验证候选。** 所有取证和候选验证必须通过独立 VT USB 探针完成。
3. 探针运行前必须关闭 `oi.exe`。禁止上层 DMA 与 USB 探针同时运行。
4. 每次测试前后都必须清理 Hook，并确认 `clean=yes`、VT Hook 表为 `0`。
5. 运行时证据优先级：实时 USB 读取/捕获 > 当前内存代码与对象布局 > 当前 PE/模块状态 > 新 SDK dump > 旧源码/旧日志/注释。
6. 旧偏移只能作为候选起点，不能直接写回。每个新偏移必须有当前版本的对象地址、原始字节/值、类型解释和动态验证证据。
7. 一次只更新一个依赖层或一个字段组。上游链未通过时，禁止继续验证下游字段。
8. 禁止为了“看起来能画”而同时修改偏移、解密、分类、平滑、投影和缓存。一次只改变一个变量。
9. 不修改 VT 驱动核心、EPT 双视图、USB 协议、通用明文捕获节奏和昨晚的绘制优化，除非用户明确下达独立任务。
10. 禁止恢复已放弃的玩家/物资低频双 Hook。SDK 偏移更新仍使用当前通用明文全量捕获链。
11. 探针没有实现某个本文命令时，必须先在 `UsbItemProbe.cpp` 实现对应的 **USB-only、只读优先** 功能；不得假装命令已经执行。
12. 任一动态探针必须短时、单候选、可清理。连续 2 至 3 次失败且没有新证据时停止动态测试，返回只读分析。

## 2. 工程位置与权威文件

```text
项目根目录：
C:\Users\Administrator\Desktop\双机内存全套_交付版

上层源码：
C:\Users\Administrator\Desktop\双机内存全套_交付版\副机源码\dma - 双机内存版

SDK 偏移主表：
Source\Common\Offsets.h

偏移注册层：
Source\Common\Offset.cpp
Source\Common\Offset.h

结构定义：
Source\Common\SDK.h
Source\Common\Constant.h
Source\Common\Data.h

关键读取链：
Source\Hack\GlobalDecrypt.h
Source\Hack\UPTRDecrypt.cpp
Source\Hack\GName.cpp
Source\Hack\Process.cpp
Source\Hack\Actor.cpp
Source\Hack\Player.cpp
Source\Hack\Item.cpp
Source\Hack\Container.cpp
Source\Hack\Radar.cpp
Source\Hack\AimBot.cpp
Source\Hack\PhysXMirror.cpp

通用明文捕获：
Source\Hack\EptCoordCapture.cpp

USB 探针：
USB链路测试\UsbItemProbe.cpp
USB链路测试\UsbItemProbe.exe

最终成品：
副机成品\上层DMA\oi.exe
```

`Offsets.h` 是偏移数值主表，但不是唯一来源。以下内容也可能包含布局常量，更新时必须全局审计：

- `Offset.cpp` 是否注册了对应键。
- `SDK.h` 的结构字段顺序、大小和 `offsetof`。
- 各 Hack 源文件中的裸 `0x...` 常量。
- `GlobalDecrypt.h` 的 slot 语义和解密算法。
- `EptCoordCapture.cpp`、`Player.cpp`、`Item.cpp` 的明文 source 偏移。
- 骨骼步长、TArray/FString/FName/FText 等容器布局。

## 3. 当前基线，仅作候选

下列数据来自当前工程，只能用于差异比较。游戏更新后必须由 USB 探针重新证明。

### 3.1 全局与 UObject

```text
UWorld RVA：             0x1FB98C88
Engine RVA：             0x1FB94948
GLocalPlayer RVA：       0x20834170
GNames RVA：             0x206CBE00
GObjects RVA：           0x206F0928
ObjID / UObject_Name：   0x1C
UObject_Number：         0x20
UObject_Index：          0x24
UObject_Class：          0x08
UObject_Outer：          0x10
UObject_Size：           0x18
UObject_Inherit：        0x50
```

当前 `GlobalDecrypt` 同时兼容两种 RVA 语义：RVA 直接指向 plain slot，或 RVA 指向 wrapper 基址后再取 `+0x1008`。加密 slot 与明文 slot 当前相差 `0x2000`，旧算法为交换 byte1/byte6 后 XOR `0x37`。这些都必须随版本复验。

### 3.2 世界与本机链

```text
UWorld.OwningGameInstance： 0x190
GameInstance.LocalPlayers： 0x38
LocalPlayer.PlayerController：0x30
UWorld.PersistentLevel：    0xF8
UWorld.Levels：             0x158
ULevel.Actor：              0x98
UWorld.GameState：          0x140
PlayerController.AcknowledgedPawn：0x3F0
PlayerController.PlayerCameraManager：0x408
```

### 3.3 场景组件、相机与骨骼

```text
Actor.RootComponent：       0x180
Actor.Mesh：                0x3D0
SceneComponent velocity：   0x190
ComponentToWorld：          0x210
ComponentLocation：         0x220
ComponentRotation：         0x230
PlayerCameraManager.CameraCachePrivate：0x2F7A0
Camera POV：                CameraCachePrivate + 0x10
Mesh.BoneArray：            0x740
Mesh.SkeletalMesh：         0x708
骨骼元素步长：              0x30
骨骼 transform 起始：       BoneArray + index*0x30 + 0x10
```

### 3.4 玩家与物资的关键第二来源

这些值目前有部分直接写在业务源码中，不能只修改 `Offsets.h`：

```text
玩家明文 source：          rootComponent + 0x240
物资明文 source：          pickupComponent + 0x154
SceneComponent 相对位置：  component + 0x168
Item.PickupMeshComponent：  actor + 0x1D98
Item.DelayMeshComponent：   actor + 0x1D50
Player Root fallback：      actor + 0x3E0
```

## 4. 更新总顺序与依赖门

必须严格按以下顺序验证。任一阶段未通过，不得进入下一阶段。

```text
0. 干净 USB 基线
1. 进程、CR3、模块和 PE 布局
2. 全局锚点与全局指针解密
3. UObject/FName/GNames 基础布局
4. UWorld、GameInstance、LocalPlayer、Controller 世界链
5. Levels、ActorArray 与 Actor 分类
6. 相机、投影和本机 Pawn
7. RootComponent、Transform、相对坐标和速度
8. 玩家 PlayerState、血量、队伍、Mesh、骨骼
9. 物资、容器、ItemInfo、ItemRow 和价格品质
10. 武器、雷达、交互、PhysX 等可选功能
11. 通用明文 source 归属复验
12. 同步源码、编译、部署和完整回归
```

依赖门示例：

- GNames 未通过，禁止根据名字判断 Actor 类型。
- UWorld 未通过，禁止扫描 Levels 或玩家。
- LocalPlayer/Controller 未通过，禁止判断相机偏移。
- ComponentToWorld 未通过，禁止修改平滑或投影掩盖错误。
- Pickup 组件未通过，禁止调整物资 source 映射。

## 5. 阶段 0：建立干净 USB 基线

```powershell
$probe = 'C:\Users\Administrator\Desktop\双机内存全套_交付版\副机源码\dma - 双机内存版\USB链路测试\UsbItemProbe.exe'
& $probe --unhide-all
& $probe --list-processes
```

必须记录：

```text
游戏进程名与 PID
CR3
PEB
模块基址
映像大小
各 PE section 的 RVA、大小和权限
VT Hook 表数量
```

基线通过条件：目标进程稳定、模块头有效、CR3 可用、随机抽取的模块页可重复读取、Hook 表为 `0`。

## 6. 阶段 1：进程、模块与读取可信度

1. 通过 USB VT 进程枚举定位当前进程，不使用上层保存的旧 PID。
2. 通过 `PEB + 0x10` 或当前系统结构解析模块基址，并用 DOS/NT headers 验证。
3. 对同一只读地址连续读取 3 次，确认数据稳定；场景变化字段除外。
4. 验证 USB 虚拟读取使用当前 PID/CR3，禁止复用上局或上次启动的状态。
5. 记录模块是否热更新、重映射或存在多个同名模块。

如果这一层失败，停止。不得把读失败产生的零值当作偏移变化。

## 7. 阶段 2：全局锚点与解密布局

按以下顺序验证：

1. `GNames`：名称池指针/基址、chunk 表和 name entry 可解释。
2. `GObjects`：对象数组头、元素数量和对象指针范围合理。
3. `Engine`：能继续解析 `GameViewport -> World`。
4. `UWorld`：能继续解析 GameState、Levels、OwningGameInstance。
5. `GLocalPlayer`：能继续解析有效 PlayerController。

对 `UWorld/Engine/GLocalPlayer` 必须分别读取并记录：

```text
Base + 候选 RVA 的原值
候选是否直接是 plain slot
wrapper + 0x1008 的值（若适用）
对应 encrypted slot 的值
解密算法与解密结果
最终指针指向对象的结构证据
```

不能只用“像用户态指针”作为通过条件。UWorld 至少要能解析合理 Levels/GameState；Engine 至少要能解析 GameViewport；LocalPlayer 至少要能解析 PlayerController。

若 slot 间距、字节交换或 XOR 常量变化，先在 USB 探针中实现并验证新算法，再更新 `GlobalDecrypt.h`。禁止在上层运行中试算法。

## 8. 阶段 3：UObject、FName 与名称池

必须验证：

```text
UObject.Class
UObject.Outer
UObject.Name / ObjID
UObject.InternalIndex
FNamePool chunk 计算
Name entry header
ANSI/Wide 标志
长度编码
名称解密算法
```

验证方法：

1. 从已知 UWorld、Level、PlayerController 等不同类型对象取样，不少于 5 个对象。
2. 对每个候选字段读取并解析，要求 Class/Outer 链形成合理对象关系。
3. ObjID 必须能在当前 GNames 中解析为稳定、可打印名称。
4. 同一个 ObjID 多次解析结果一致；不同对象不能大量返回同一错误名称。
5. ANSI 与 Wide 名称各找至少一个样本。

若名称长度、chunk 位数、entry 步长或 XOR 规则变化，必须同步 `GName.cpp`，不能只改 `GNames` RVA。

## 9. 阶段 4：世界链和本机链

按顺序验证以下链路，每一跳都记录 `base + offset -> raw -> decoded`：

```text
UWorld
  -> OwningGameInstance
    -> LocalPlayers TArray
      -> LocalPlayers[0]
        -> PlayerController
          -> AcknowledgedPawn
          -> PlayerCameraManager

UWorld
  -> GameState
  -> PersistentLevel
  -> Levels TArray
```

TArray 通过条件：

- data 是合理指针。
- count >= 0 且不超过业务上限。
- capacity >= count 且不过分异常。
- 抽样元素能继续解析为预期对象。

指针字段如果带高位 handler/type 元数据，必须同时记录 raw 值与低 48 位结果，并验证当前 `UPTRDecrypt::Indicator` 是否仍适用。禁止默认所有指针永远只需截低 48 位。

## 10. 阶段 5：Levels、ActorArray 与分类

1. 从 `UWorld.Levels` 取得多个 Level。
2. 验证每个 Level 的 Actor TArray。
3. 抽样至少 30 个 Actor 指针，记录 ObjID、名称、Class、Outer、RootComponent。
4. 统计有效率；不能只凭 1 个对象认定偏移正确。
5. 至少覆盖玩家、物资、容器、普通场景 Actor 四类。
6. 换地图后重复，排除只适配单个 Level 布局。

Actor 数量突然为 0 时，先检查 Levels/ActorArray/TArray，不要先改分类字符串或实体过滤器。

## 11. 阶段 6：相机与投影

相机必须在世界链通过后验证：

```text
PlayerController -> PlayerCameraManager
PlayerCameraManager -> CameraCachePrivate
CameraCacheEntry -> FMinimalViewInfo
Location / EncHandler / Rotation / FOV
```

当前 `FMinimalViewInfo` 布局候选：

```text
Location：   +0x00，FVector
EncHandler： +0x0C，4 字节
Rotation：   +0x10，FRotator
FOV：        +0x1C，float
```

验证动作：站立不动转动视角、改变俯仰、移动位置、开镜改变 FOV。每次只改变一个变量并用 USB 连续读取，确认对应字段变化，其他字段保持合理。

相机通过条件：

- Location 与当前本机位置同一数量级并连续变化。
- Rotation yaw/pitch 与单一转动动作对应。
- FOV 有限且在合理范围，开镜时按预期变化。
- WorldToScreen 对屏幕中心前方目标投影合理。

投影失败时先确认相机结构、矩阵和屏幕尺寸；禁止修改玩家坐标平滑器。

## 12. 阶段 7：场景组件与坐标布局

分别验证以下字段：

```text
Actor.RootComponent
Actor.Mesh
SceneComponent.RelativeLocation（当前裸偏移候选 0x168）
ComponentVelocity
ComponentToWorld
ComponentLocation
ComponentRotation
```

至少取 3 个静止对象和 3 个移动对象。验证：

- RootComponent 指向可解释的 SceneComponent。
- RelativeLocation/ComponentToWorld translation 与场景位置同一数量级。
- 移动物体坐标连续变化，静止物体稳定。
- Velocity 方向与位移差分方向一致。
- Transform 的 scale/rotation/translation 均为有限值。

如果 `ComponentToWorld` 与 `+0x168` 同时失效，先证明 SceneComponent 布局变化，不要把某个偶然出现的 FVector 当成新坐标。

## 13. 阶段 8：玩家布局

按依赖顺序验证：

1. Pawn：RootComponent、Mesh、PlayerState。
2. PlayerState：名字、Uin、TeamId、Camp、死亡状态、HeroID。
3. HealthComp -> HealthSet：Health、MaxHealth、护甲与濒死血量。
4. Mesh：ComponentToWorld、BoneArray、SkeletalMesh、AnimScriptInstance。
5. 骨骼：数组指针、元素步长、局部 transform 与世界 transform。
6. 武器：CacheCurWeapon、WeaponID、弹药、WeaponMesh。

动态验证动作：

- 本机或可控目标移动、蹲起、死亡/复活。
- 队伍/阵营用多名玩家交叉验证。
- 受伤与治疗验证 Health/MaxHealth。
- 切换武器与开火验证 WeaponID、弹药和 firing mode。
- 骨骼至少验证头、躯干、手脚若干点，不能只看一个 bone。

骨骼验证必须同时证明 `BoneArray` 指针、元素步长、transform 起始位置和 ComponentToWorld。骨架错位时禁止只改 bone ID 表掩盖布局错误。

## 14. 阶段 9：物资与容器布局

物资链按顺序验证：

```text
Actor
  -> PickupMeshComponent / DelayMeshComponent / RootComponent
  -> PickupItemInfo
    -> ItemID
    -> ItemRow
      -> Quality
      -> InitialGuidePrice
```

容器另行验证：

```text
容器 Actor -> RootComponent
容器状态：bIsOpened / OpeningPlayerId / RepItemArrayItems / CurrentLootingPlayers
```

至少使用以下现场状态：地面物资、拾取前后、不同品质/价格物资、未开启容器、已开启容器。只改变一个状态并比较 USB 快照。

必须区分：

- 白色容器不是地面物资。
- PickupMeshComponent 与 DelayMeshComponent 不是玩家 RootComponent。
- 物资 source 必须精确归属于当前 pickup component。
- ItemRow/价格正确不代表坐标组件正确，二者要分开验证。

出现物资聚堆或跟随玩家时，优先检查组件偏移、source `+offset` 和 exact-source lookup，禁止调整屏幕投影掩盖错误。

## 15. 字段候选的 USB 验证方法

### 15.1 指针字段

候选必须满足：

1. raw 值可重复读取。
2. 解码后是合理用户态地址。
3. 指向页面可读。
4. 指向对象能通过 Class/Name/Outer 或下游字段证明类型。
5. 不少于 3 个对象样本通过。

### 15.2 TArray/容器字段

同时验证 data/count/capacity，并抽样首、中、尾元素。只看到一个合理 count 不算通过。

### 15.3 数值、布尔与枚举

通过可控动作做前后快照：血量、弹药、FOV、开关门、拾取、死亡、队伍。一个动作只改变一个变量，记录旧值、新值和变化时间。

### 15.4 FVector/FTransform

要求有限值、合理范围、时间连续性和空间关联。三个随机 float 看似合理不足以证明坐标字段。

### 15.5 快照差分

对已确认对象基址读取有界窗口：

1. 状态 A 连续读取 2 至 3 次，排除自然波动字段。
2. 只执行一个可控动作进入状态 B。
3. 再读取同一窗口并计算变化位置。
4. 按字段类型、对齐、变化方向和下游语义筛选候选。
5. 换第二个对象复验同一 offset。

禁止无界扫描所有内存。窗口范围必须来自当前对象大小、SDK dump 或相邻已知字段。

## 16. USB 探针维护接口

当前 `UsbItemProbe.exe` 已有进程枚举、绝对地址导出、Hook 清理和通用入口验证能力。SDK 更新需要的结构化命令若尚未实现，应先添加到 `UsbItemProbe.cpp`。建议接口：

```text
--sdk-baseline
  输出 PID/CR3/PEB/base/PE sections/Hook 表。

--sdk-read <address> <size> <output.bin>
  有界只读导出，等价能力可复用 --dump-absolute。

--sdk-chain <profile>
  按候选偏移读取 world/local-player/actor/player/item/camera 链，并输出每一跳 raw/decoded/value。

--sdk-snapshot <address> <size> <output.bin>
  保存对象窗口，用于状态 A/B 差分。

--sdk-validate <profile>
  对一组候选偏移执行类型、范围、TArray、名称和下游语义检查，不安装 Hook。
```

实现要求：

- 参数错误直接退出并释放所有 USB/LeechCore 句柄。
- 输出必须包含候选偏移，不得只输出“成功”。
- 默认只读；确需动态 Hook 时使用独立命令并明确时限。
- 所有退出路径清理临时 Hook。
- 不把候选自动写入 `Offsets.h`。

## 17. 源码同步顺序

USB 证据全部通过后，按以下顺序修改：

1. `Offsets.h`：更新已证明的主表常量。
2. `Offset.cpp`：确认每个业务键已注册；新增字段必须新增注册，删除字段不得留下无意义使用点。
3. `SDK.h`：更新结构字段顺序、padding、大小和 `static_assert/offsetof`（适用时）。
4. `GlobalDecrypt.h`、`UPTRDecrypt.cpp`、`GName.cpp`：仅在对应算法已有 USB 证据变化时修改。
5. `Process.cpp`、`Actor.cpp`、`Player.cpp`、`Item.cpp` 等：移除或同步已确认的裸偏移。
6. `EptCoordCapture.cpp`：仅同步已证明变化的玩家/物资 source 归属；不改捕获节奏和平滑逻辑。
7. 全局搜索旧十六进制值和字段名，确认没有第二份旧常量。

强制搜索示例：

```powershell
$src = 'C:\Users\Administrator\Desktop\双机内存全套_交付版\副机源码\dma - 双机内存版'
rg -n '0x1D98|0x1D50|0x168|0x154|0x240|0x3E0|0x30' "$src\Source"
rg -n 'RootComponent|PickupMeshComponent|ComponentToWorld|BoneArray|CameraCachePrivate' "$src\Source"
```

搜索结果必须逐条分类：真实布局常量、结构大小/步长、无关数值、注释或废弃代码。禁止机械全局替换相同数字。

## 18. 分阶段回归矩阵

每完成一组修改，只验证对应层和其直接下游：

| 阶段 | 最小通过条件 |
|---|---|
| 全局锚点 | UWorld/Engine/GNames/GObjects/LocalPlayer 可解释 |
| 世界链 | GameState、Levels、Controller 连续有效 |
| Actor | 多 Level Actor 数合理、名称可解析 |
| 相机 | Location/Rotation/FOV 与动作对应、投影正确 |
| 玩家 | 多人位置、队伍、血量、骨骼正确 |
| 物资 | 多种物资独立位置、价格品质正确、不聚堆 |
| 容器 | 容器分类、位置和开关状态正确 |
| 通用明文 | 玩家/物资 exact source 命中且互不串位 |
| 场景切换 | 换图、重进局后重新解析，无旧指针残留 |

禁止第一轮就同时验证所有功能并根据最终画面猜哪一层错了。

## 19. 编译、部署与正式复验

只有 USB-only 布局验证完成后才能编译上层：

```powershell
$src = 'C:\Users\Administrator\Desktop\双机内存全套_交付版\副机源码\dma - 双机内存版'
$msbuild = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
& $msbuild "$src\oi.sln" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m /nologo
```

部署前：

1. 备份当前可运行成品并记录 SHA-256。
2. 确认 USB 探针已退出，Hook 表为 `0`。
3. 复制新产物到 `副机成品\上层DMA\oi.exe`。
4. 核对构建产物与交付成品 SHA-256 一致。
5. 只启动交付目录成品。

正式复验至少覆盖：

- 启动、进局、退局、重新进局。
- 两张不同地图。
- 单人、多人、玩家近距离聚集。
- 玩家移动、蹲起、死亡、换武器、开镜。
- 多种地面物资与拾取前后。
- 容器开启前后。
- 名称、队伍、血量、骨骼、相机、雷达和价格品质。
- 游戏 FPS、上层绘制流畅度、闪退和异常日志。
- 停止上层后的 Hook 清理。

若新版本失败，立即停止上层、执行清理、保留失败日志，并回退上一成品。禁止在失败版本上继续叠加无关优化。

## 20. 常见误判

### 全部指针为 0

先检查 PID/CR3/模块基址、USB 读取和全局解密；不要认定所有 SDK 偏移同时变化。

### Actor 数为 0

检查 UWorld、Levels、ULevel.Actor 和 TArray 布局；不要先改名称分类。

### Actor 有数量但名称全错

检查 GNames、ObjID、FNamePool chunk/header 和名称解密；不要改 ActorArray。

### 玩家位置错误但物资正常

检查玩家 RootComponent、Mesh、ComponentToWorld 和玩家 source；不要改通用入口。

### 物资聚堆或跟随玩家

检查 PickupMeshComponent、物资 source `+offset` 和 exact-source lookup；不要改投影或平滑器。

### 骨架散开或缩成一点

检查 BoneArray、元素步长、transform 起始和 ComponentToWorld；不要只改骨骼索引。

### 相机抖动或所有坐标偏屏

检查 CameraCachePrivate、FMinimalViewInfo、矩阵和屏幕参数；不要改玩家坐标捕获节奏。

### 换图后全部失效

检查 UWorld/CR3/模块重载和缓存清理；禁止继续使用上一局对象地址。

## 21. AI 完成条件与交付报告

后续 AI 不得只给新偏移列表。最终报告必须包含：

1. 当前 PID、CR3、PEB、模块基址、映像大小和 PE section。
2. 每个修改字段的旧值、新值、对象类型、样本地址和验证方法。
3. 全局指针的 plain/encrypted slot、算法和下游链证明。
4. UObject/GNames/ObjID 的交叉验证结果。
5. 世界链、相机、Actor、玩家、物资、容器各阶段通过情况。
6. USB 快照/导出/日志路径和命令。
7. 动态探针时长、命中情况、进程存活和最终 Hook 表。
8. 修改的源码文件及所有裸偏移审计结果。
9. 编译结果、构建产物 SHA-256、交付成品 SHA-256 和回退成品 SHA-256。
10. 换图、重进局、多人、FPS、闪退和卸载清理结果。
11. 尚未验证的字段必须明确标为“未验证”，不得用旧值冒充已更新。

只有从干净 USB 基线开始可重放全部关键链路，并且正式成品通过完整回归，才算 SDK 偏移与布局更新完成。

## 22. 2026-09-05 地图识别现场证据与更新范围

本节是 USB 只读结果，不代表上层源码已同步或模型已成功加载。第 3 节的旧基线不能覆盖本节实测值。

### 22.1 公共字段，不需要每张地图重新抓整套 SDK

- `DFMGameState::DFMGamePlayerMode`：旧 `0x6D1`，当前反射为 `0x6E1`，1 字节。
- `DFMGameState::MapConfig`：当前 `0x6E8`，大小 `0x90` 的内嵌结构，不是 UObject 指针。
- `MapConfig::LevelName`：结构内 `0x30`，FString 大小 `0x10`。
- 合计 `GameState + 0x718`，代替旧 `0x708`。大坝实测 `Dam_Iris_Level1`，按现有 `_Level` 截断规则得到 `Dam_Iris`，与 `Engine/Map/Dam_Iris.npak` 对应。
- 地图名称变化时单独维护别名映射，不要重新猜整套 SDK。公共字段先在一个地图跑通，再换一张地图和退出大厅复验；不要求所有地图逐张抓偏移。
- 模型文件存在不代表新赛季地形完全一致。偏移更新解决数据读取，无法自动更新旧 `.npak` 的几何内容。

### 22.2 状态与加载门控

- 大厅和对局的 UWorld 都是 `Iris_Entry`，该次进局 UWorld 地址也未改变。不能仅靠世界地址/根世界名判定换局或选图。
- 大厅 Pawn/GameState 为 `BP_DFMCharacter_SafeHouse_C` / `BP_GameState_IrisSafeHouse_C`；对局为 `BP_DFMCharacter_C` / `BP_GameState_PVPVE_C`。
- 大厅 Levels 已有 `107/204`，对局 `464`，所以源码的 `Levels > 16` 兜底会误判大厅。
- `MapConfig::bInGameLevel +0x8A` 在大坝和 Forrest 对局均为 1，中间大厅快照为 0；已完成这三种状态的对照，但不代表所有玩法均已覆盖，不能只凭单字节替换全部门控。
- 远端存在 `PhysX3_x64.dll`，但旧 `PhysXSDK=0x20EB78` 得到非对齐值 `0x785055413F2E`，不构成有效 PxScene 链。
- 当前 `PhysXMirror::Update` 在本地加载前依赖远端 PxScene、本机玩家和武器 ID。应分别处理公共地图字段与本地模型加载前置条件，不能将模型未加载直接归咎于缺文件。

### 22.3 证据与限制

- 权威日志：`副机源码\dma - 双机内存版\USB链路测试\evidence-match-20260905\map-04\usb_item_probe_diff.log`。
- 大厅对照：同一探针目录 `evidence-lobby-20260905\scene-03\usb_item_probe_diff.log`。
- 复验命令：`UsbItemProbe.exe --scene-map-details`，必须从新证据目录运行，避免覆盖工作目录下默认日志。
- 本轮未改正式上层、未写游戏内存、未安装 Hook，前后 Hook 表均为 0。
- 新快照辅助器确认 FProperty 的 ArrayDim/ElementSize 在 `+0x3C/+0x40`；早期 map-01/map-02 的字段大小注释已被 map-04 取代，旧通用辅助函数尚未一并修正。
- 地图偏移更新与全量坐标捕获是两件事。通用捕获也应使用公共链，不做每张地图各一套入口；当前尚无本轮全量明文成功证据。

### 22.4 大坝、大厅、第二张地图复验结果

用户最新要求为先复验地图、暂停坐标捕获。本轮只读快照已覆盖以下顺序：

| 证据子目录 | 状态 | Mode +0x6E1 | LevelName +0x718 | MapID | bInGameLevel |
| --- | --- | --- | --- | --- | --- |
| map-04 | 大坝对局 | 1 | Dam_Iris_Level1 | 2201 | 1 |
| map-recheck-01 | 大厅/下一张地图预载 | 6 | 空 | 0 | 0 |
| map-second-01 | Forrest 进局过渡 | 1 | Forrest_Level1 | 1901 | 1 |
| map-second-02 | Forrest 稳定对局 | 1 | Forrest_Level1 | 1901 | 1 |

- 上述目录均在 `副机源码\dma - 双机内存版\USB链路测试\evidence-match-20260905`，各自日志名为 `usb_item_probe_diff.log`。
- 第二张地图使用同一条 `GameState +0x6E8` 内嵌 MapConfig、结构内 `+0x30` FString 链，截断 `_Level` 后为 `Forrest`，对应 `Engine/Map/Forrest.npak`。已测两张地图不需要分别重抓偏移。
- 大厅快照 Levels 为 528，且已有 Forrest_Long3 流关卡；不能把预载关卡名称当作正在对局，也不能继续以 `Levels >16` 兜底。
- 过渡样本 GameState 已切为 PVPVE、地图名已是 Forrest，但 Pawn 仍为 SafeHouse，`bMatchRealStart +0x10A9=0`；稳定样本 Pawn 为 `BP_DFMCharacter_C`，该值变为 1。状态切换需要处理这一中间阶段，不能假定所有字段同时更新。
- 此次大厅到 Forrest 的 UWorld 均为 `0x8BA0E7EDC0`、名称仍为 `Iris_Entry`；GameState 从 `0x8CFA62BBC0` 切到 `0x8CC3E813C0`。地址只作为当次证据，禁止硬编码。
- 第二图稳定快照末尾 `worldReadable=1 rawWorldStable=yes gameStateStable=yes`，最终 Hook 数为 0。末尾复读不是原子快照保证。
- 地图字段和状态对照复验通过；正式上层尚未同步偏移/门控，未启动上层，不能宣称 `.npak` 已成功解析、创建 PhysX 场景或渲染。下一项地图工作是隔离坐标捕获后的加载链验证。

### 22.5 后续源码修正与本地加载器复验

本节更新 22.1 至 22.4 只读阶段的交接状态：用户保持 Forrest 对局并要求继续后，已修改源码侧地图读取和本地加载路径，但未替换交付成品，也未启动普通上层或坐标捕获。

- `Source/Common/Offsets.h` 已同步 `DFMGamePlayerMode=0x6E1`、`LevelName=0x718`、`MapInGameLevel=0x772`。
- `Process.cpp` 改为有界 FString 读取、合法地图名和对局字段联合判断，移除 `Levels >16` 兜底；大厅/无进程时清除旧地图名，读取后复查 GameState。
- `PhysXMirror.cpp` 将远端 PxScene、玩家和武器 ID 检查移到远端同步路径。本地文件加载不依赖这些指针；空地图等待，不回退 SecretService；地图名/GameState/PID 改变时重载，失败后清理部分 Actor 并间隔 5 秒重试。
- 本地模式缺失模型时不自动落到旧远端 PhysX 链。退局后复用仍存活的 foundation/physics，避免重复创建底层 SDK。
- `PhysXCollect.cpp` 拒绝短文件头；加载时只替换 Actor，不随异步游戏状态销毁整个 scene；成功条件包含读取状态、Actor 数量和失败计数，不能只打印成功。
- 增加 `--verify-local-model <新日志路径> <地图名...>` 入口，在配置加载、USB 初始化和工作线程启动之前返回。`--lobby` 参数仅在该入口内模拟本地场景释放和重建，不操控游戏机。
- 初次离线加载后发现退出访问异常，调试栈定位为未初始化的 Memory 析构触发延迟 VMM 导入并访问已销毁的模块表。已初始化空句柄并跳过空句柄 Close；未改 USB 协议或驱动。

最终复验记录：

| 本地验证步骤 | 场景静态 Actor | 抽样射线命中 / 128 | 结果 |
| --- | --- | --- | --- |
| Forrest 加载 | 50243 | 125 | 通过 |
| 切换 Dam_Iris | 37933 | 128 | 通过 |
| 模拟大厅释放 | scene=null | 不适用 | 通过 |
| 重建并加载 Forrest | 50243 | 125 | 通过 |

- 完整流程退出码 0；缺失文件退出码 2；非法文件名退出码 64，均符合预期。
- 同时另用 USB 只读探针复读现场：仍为 Forrest_Level1，正常对局 Pawn，bMatchRealStart=1，world/GameState 复读稳定，最终 Hook 数 0。
- 证据目录：`副机源码\dma - 双机内存版\USB链路测试\evidence-model-20260905`。包含 `backup`、`live-map-01`、`debug-exit-01.log`、`build-final.log`、`final-replay-01` 和 `README.md`。
- 从源码根目录执行 `& '.\USB链路测试\evidence-model-20260905\Replay.ps1' -RunName 新名称` 重放。脚本串行调用诊断入口，拒绝覆盖原证据；各次日志和哈希写入新目录的 `manifest.json`。
- 源码构建 `x64/Release/oi.exe` SHA-256：`AC7C174A4C4B261F4F69D0E26425FB63BB53A7FB9BD0AE9EF077363AEC861E48`。
- 原源码构建已备份。交付 `副机成品/上层DMA/PYZ.exe` 未变，SHA-256 仍为 `10CEC3802BA9113DE741100E5340C2A5D0DED243983156FAAF0AD1B7B7FE20AA`。

边界：本次证明实际本地文件解析、cooking、场景 Actor、射线查询、切换和模拟退局重建，不是普通上层线程及可视化验收。游戏当前赛季地形与旧模型的几何一致性、其他模式、完整上层实机显示仍未验证。不要无参数启动测试构建：普通启动仍会开启坐标捕获，与当前暂停要求冲突。

### 22.6 普通上层地图线程实机测试未通过

用户再次进入对局要求实测，本次基线为 `Dam_Iris_Level1`，不是上一局 Forrest。直连 USB 探针读到 PID 85868、CR3 `0x10ACAD000`、UWorld `0x42BE95C940`、GameState `0x8C4E84C4F0`，角色已处于对局，Hook 数 0。

新增隔离诊断入口 `--verify-map-live <新日志路径>`，只运行原 Process 和 PhysXMirror 地图线程，不运行捕获、瞄准、模型保存或普通 UI 启动流程。实测得到两个独立阻塞：

1. 普通 VMM 路径出现 READ_PHYS 超时 121、2064384 字节 BATCH_READ 错误 995，随后 USB 请求管道错误 433。最终 UWorld、GameState、地图名和模型 Actor 均未取得，不能报告自动加载通过。失败后直连 USB 探针再次读取正常，同一游戏进程和地图仍存活，Hook 数为 0。两者 leechcore.dll 哈希相同，不是两个不同 DLL 版本造成的已证实差异；VMM 物理/批量路径的确切根因尚未定位。
2. 不连接游戏的查看器测试仍能加载大坝 37933 个 Actor，但 GLFW 初始化失败。当前 `build/link-stubs/glfw3.lib` 仅 4034 字节，反汇编确认 `glfwInit` 为 `xor eax,eax; ret`，是占位实现。当前库搜索顺序会选中它，因此不能显示；不是已证实的显卡不兼容，也没有有效渲染截图。

诊断入口初次失败后退出访问异常；已增加显式 VMM 提前关闭，避免已知的静态析构顺序问题，但本次崩溃没有独立栈取证，也未重复失败的线上读取，所以不宣称异常退出已复验修好。

- 证据：`副机源码\dma - 双机内存版\USB链路测试\evidence-live-map-20260905`，含前后直连快照、`live-01` 日志、`preview-01` 日志、备份及 README。
- 最终构建 `build-03.log` 成功；随后原离线回归三项全部通过，见 `evidence-model-20260905/after-live-diagnostic/manifest.json`。
- 当前源码构建 SHA-256 为 `436917C325E0B5D90200AE58D4B2ED3F0FAC429D8179445462B2B95B5617CE05`；前一构建已备份。交付 PYZ、驱动、DLL 和地图资源未改。
- 下一步先处理 VMM 物理/批量传输与真实 GLFW 链接，再回到自动加载和显示复验。坐标捕获保持暂停，不得把偏移或离线 Actor 验证通过扩大为完整上层通过。

### 22.7 GLFW 占位库修复及双图离线显示

用户报告回到大厅后，本轮没有连接游戏内存，也未安装捕获 Hook。保留原占位库，新增官方 GLFW 3.4 源码到 `third_party/glfw-3.4`，以 MSVC x64 Release、静态 CRT 编译。项目显式优先使用真实头文件/库，并加缺库报错，防止静默链接占位库。

实际结果：

| 地图 | PhysX Actor | 查看器实例 | 非背景像素 | OpenGL 错误 | 退出码 |
| --- | --- | --- | --- | --- | --- |
| Dam_Iris | 37933 | 37929 | 251616 | 0 | 0 |
| Forrest | 50243 | 50154 | 156020 | 0 | 0 |

1280x800 帧缓冲 PNG 已保存并目视确认地形和模型非空、取景可见。查看器保留原有几何转换/缩放过滤，实例数与输入 Actor 并不完全一致；本结果不保证每个 Actor 都显示，也不保证模型变换和当前赛季地形完全一致。

- 证据目录：`副机源码\dma - 双机内存版\USB链路测试\evidence-glfw-20260905`，含 `preview-01.log.png`、`preview-forrest.log.png`、日志、备份、README 和 `BuildGlfw.ps1`。
- `oi/oi.vcxproj` 改为真实库优先；原 `build/link-stubs/glfw3.lib` 和兼容头未删除，交付 PYZ、驱动和传输 DLL 未改。
- 当前源码构建 SHA-256：`FFF63EF6972C3C09591A590D22504A717F6BFE3F005F5ECAEE181B080E56DD41`。
- 新构建原有离线模型回归三项全部通过，见 `evidence-model-20260905/after-real-glfw/manifest.json`。
- GLFW 显示初始化阻塞已解决。VMM 物理/批量读取阻塞仍未解决，本轮仅检查源码线索，未重跑上次失败的实机读取；坐标捕获继续暂停。

### 22.8 收敛为小块 USB 地图路径，大厅实测通过

用户最新要求为“能正常初始化、识别游戏机对局地图并加载模型即可”，明确不需要大批量读取。已停止扩大 VMM 批读排查，不恢复坐标捕获。

地图专用入口现在只初始化 LeechCore USB，不调用 VMM 初始化、远程模块扫描、物理批读或远程 PhysX 集合读取。通过 `Source/Hack/UsbMapState.h` 小块读取目标 PEB/PE 头、UWorld、GameState、模式、对局标志和有界 FString；复读状态确认一致后，用原有 `MapLoadPolicy` 选择本地 `.npak`。地图采样与原 PhysX 本地加载器在同一个线程串行执行，地图原点也仅通过 VT 读取 12 字节。无须人工指定地图名。

实测 `usb-only-lobby-01`：

- USB 初始化成功，耗时 94 ms；日志为 `vmm=not-initialized`。
- PID 85868，UWorld `0x8CB719B700`，GameState `0x8B9F5B7780`，模式 6。
- 8 次有效采样、0 次失败，正确判定大厅，活动地图名为空，模型 Actor 为 0。
- 首尾 Hook 数为 0，USB 错误数为 0，`passed=1`，进程退出码 0；诊断已结束，无遗留运行任务。
- 最终构建 `upper-build-03.log` 成功；原有离线模型三项回归全部通过，见 `evidence-model-20260905/after-usb-map-only/manifest.json`。

复验命令在证据目录 `副机源码\dma - 双机内存版\USB链路测试\evidence-vmm-isolation-20260905` 执行，RunName 必须未使用：

```powershell
.\MapReplay.ps1 -State Lobby -RunName lobby-check-new
.\MapReplay.ps1 -State Match -RunName match-check-new
```

脚本只使用显式诊断参数。不得无参数启动 `oi.exe`，原普通入口仍会启动捕获等无关功能。Match 模式先采样并自动选图加载，成功后显示并截图；尚待用户进入大坝或长弓溪谷完成新版路径的对局实测，不能提前宣称自动模型加载和显示已通过。

前置 VMM 排查仅作归档：确认 PA 0 单读会等待约 10 秒；正常页与 PA 0 混合批读实际收到 6144 字节、客户端却等待 10240 字节，出现 995/433。`UsbPhysicalMap.h` 在副机本地映射中排除首个物理页后，这两个小复现消失，且两次独立 VMM 小字段复验通过。但原 Process/PhysX 组合仍遇到约 2 MB 批读失败，因此仅是局部缓解，不是完整 VMM 修复。地图专用路径已不再依赖它；后续不要自动继续该大批读方向。

当前源码 `oi.exe` SHA-256：`B75C571F70B97386CC439047B0AA90E3D654B23EA5E562A6575FD7008668A075`。交付 `PYZ.exe`、驱动、两份传输 DLL 和地图资源均未替换。备份、最小复现、失败记录、验证脚本和最终大厅日志均保存在上述证据目录。

### 22.9 小块 USB 对局自动识图、加载和显示通过

用户进入对局后，仅运行 `MapReplay.ps1 -State Match -RunName usb-only-match-01`，未输入地图名，未启动 VMM 扫描或坐标捕获。

- USB 初始化 110 ms，PID 85868，UWorld `0x8CB719B700`，GameState `0x8CB2372770`，模式 1、`inGame=1`，自动识别 `Dam_Iris`。
- 自动加载 `Engine/Map/Dam_Iris.npak`，Actor `37933/37933`，跳过 0、失败 0、`complete=1`，文件加载耗时 1788 ms。
- 原查看器生成 37929 个实例，保留其现有缩放过滤，4 个无效缩放未显示。帧缓冲为 1280x800，非背景像素 251616，OpenGL 错误 0；截图已目视确认地形和建筑模型可见，显示阶段累计 1441 帧。
- 有效状态采样 41 次、失败 0；首尾 Hook 数 0，最终目标 PID 仍为 85868，USB 错误数 0。
- `loaded=1 displayed=1 passed=1`，退出码 0，退出时本地场景已释放；没有诊断进程留在后台。
- 证据：`evidence-vmm-isolation-20260905/usb-only-match-01/` 内的 `map.log`、`stdout.log`、`map.log.png`、`manifest.json`。

结论仅限当前大坝对局的地图专用入口：初始化、自动识别地图、选择本地模型、加载和显示已贯通。不是“全量坐标捕获通过”，不是旧完整 VMM 路径通过，也不是所有赛季几何/变换逐项正确的声明。换图和实际回大厅转换仍可做回归，无须为每张地图重新寻找一套地图名偏移。交付成品未替换，普通无参数入口仍不要启动。

### 22.10 持续窗口换图实测及读取失效边界

本节更新前两节对源码入口的描述：新源码默认入口已改为地图专用窗口，旧完整启动函数保留但不由当前 main 调用。捕获、自瞄、远程 PhysX 同步和 VMM 初始化均不启动。交付 `PYZ.exe` 尚未替换，不能把源码入口的变化套用到交付旧程序。

- 同一窗口 PID 6608：自动识别 `Dam_Iris`，加载 37933 个 Actor；用户回大厅后 `actors=0 instances=0 sceneNull=1`；再进入 `Forrest` 自动加载 50243 个 Actor，生成 50154 个渲染实例。地图名没有人工指定。
- Forrest 文件加载完整，耗时 2257 ms；模型查看器过滤 89 个无效缩放。原窗口被最小化，未保存换图后的帧缓冲，所以不能声称同进程换图画面也已目视复验。
- 后续在用户确认仍在对局内时，状态采样失效并显示等待游戏。停止该窗口时，累计有效 588 次、失败 51 次；场景释放、工作线程正常结束，但最终 Hook 查询失败，状态未知，不能写成 Hook 为零。持续读取失效原因尚未确认。
- 给 `UsbMapState.h` 增加失败阶段、耗时、累计计数和 10 秒状态日志，没有增加 USB 请求。诊断版使用独立名称 `oi-map-diagnostic.exe`，不覆盖锁定的旧 exe。
- 独立重新初始化短测 `stage-check-01`：20 秒内有效 53 次、失败 0，自动识别和显示 Forrest；帧缓冲 1280x800，非背景像素 156020，GL 错误 0，已目视确认模型非空。首尾 Hook 查询均成功且数量 0，退出码 0，场景释放。

结论：两张地图的自动识别和本地加载、实际大厅清理已得到运行证据，重新初始化后另一张地图的显示也通过。但持续会话读取失效仍未定位，不能据此宣称稳定性或自动恢复已修好。暂不更新交付成品，不恢复坐标捕获。下一步应保持当前地图，用阶段日志定位持续读取失败及连接恢复，而非逐图重新抓取偏移。

完整备份、构建记录、复验步骤及成功/失败日志位于 `副机源码\dma - 双机内存版\USB链路测试\evidence-map-runtime-20260905\README.md`。诊断 exe SHA-256 为 `68E66DF819EB331655E10413054E31344648F0C73B3A52B0DDA2EED3B152E2F0`。本轮短测已结束。

### 22.11 持续小量读取复验

2026-09-05 23:11:15 至 23:20:11，使用相同诊断 exe 和传输 DLL 在 Forrest 当前对局持续测试。用户要求“差不多就行了”后通过 stop.request 正常结束，实际约 536 秒，不是完整 10 分钟。

- USB 初始化 125 ms；目标 PID 85868、GameState `0x8c27efd8a0`、地图 Forrest 保持不变。
- 1598 次有效采样、0 次失败；模型只加载一次，没有反复清空重载。Actor 50243、渲染实例 50154，GL 错误 0。
- 首尾 Hook 查询均成功且数量 0，退出 `sceneNull=1 workerFailed=0`，进程退出码 0。
- 新诊断 exe 的离线模型三项回归均通过；第二地图窗口被单实例锁拒绝，退出码 67，未初始化第二条 USB 连接。
- 本轮未修改偏移、驱动、传输 DLL 或自动重连逻辑，未启动坐标捕获。此前失效没有复现，不应写成已定位或修复根因，也没有验证物理断线自动恢复。

测试已结束，无需继续逐张地图抓取。证据位于 `evidence-map-runtime-20260905/sustained-01/`、`offline-diagnostic-01/` 和 `singleton-01/`。交付程序仍未替换，下一步是整理地图专用交付入口，并对实际交付文件做一次启动验证。

### 22.12 交付版已更新并验证默认启动

2026-09-05 23:25，经用户明确要求，已将经过验证的地图专用二进制替换到 `副机成品\上层DMA\PYZ.exe`。新文件 SHA-256：`68E66DF819EB331655E10413054E31344648F0C73B3A52B0DDA2EED3B152E2F0`。本节取代前文“交付程序未替换”的当前状态描述。

只替换 EXE，未修改驱动、传输 DLL、配置或地图资源。原程序完整备份在 `副机源码\dma - 双机内存版\USB链路测试\evidence-map-runtime-20260905\delivery-01\PYZ.before-map-only.exe`，旧哈希为 `10CEC3802BA9113DE741100E5340C2A5D0DED243983156FAAF0AD1B7B7FE20AA`。旧备份仍是原完整启动入口，不要把它当地图专用版本直接运行。

实际交付 `PYZ.exe` 无参数启动复验：PID 26952，USB 初始化 125 ms；当时游戏机在大厅，识别 mode=6、GameState `0x8cc6848890`；35 次采样全部有效，模型和实例为零，场景为空，画面无旧模型残留。首尾 Hook 查询成功且数量 0，退出码 0，工作线程正常清理，测试窗口已结束。

日常使用：双击上述交付 `PYZ.exe` 即进入地图专用窗口，无需诊断参数；大厅不加载地图，进入对局后自动选择本地模型。坐标捕获、自瞄、远程 PhysX 同步、VMM 初始化仍关闭。交付文件名下本次实测为大厅；该 EXE 与此前两张地图的成功测试二进制完全相同。本轮没有再次要求用户进图，也不宣称旧持续读取故障根因或物理断线恢复已修复。

交付验证日志位于 `副机成品\上层DMA\Logs\Map\26952-526014890\`；更新清单和进程退出结果位于上述 `delivery-01`。需要回滚时，先关闭程序并核对旧备份哈希，再恢复原文件；不需要更换其他文件。

