#pragma once

class C_Entity
{
public:
	/**
	* GetIsCamp：判断当前模式是否为阵营模式（突破模式）
	* @param DFMGamePlayerMode 当前对局模式
	* @return true=突破模式（使用阵营），false=普通队伍 ID 模式
	*/
	static bool GetIsCamp(EDFMGamePlayMode DFMGamePlayerMode)
	{
		bool bIsCamp = false;
		switch (DFMGamePlayerMode)
		{
		case EDFMGamePlayMode::None:
			break;
		case EDFMGamePlayMode::GamePlayMode_SOL:			// 决战模式
			bIsCamp = false;
			break;
		case EDFMGamePlayMode::GamePlayMode_Raid:			// 全面行动 PVE
			break;
		case EDFMGamePlayMode::GamePlayMode_IrisDiscovery:	// 烽火地带
			break;
		case EDFMGamePlayMode::GamePlayMode_Conquest:		// 全面征服
			bIsCamp = false;
			break;
		case EDFMGamePlayMode::GamePlayMode_Breakthrough:	// 突破攻坚模式，使用 Camp 阵营
			bIsCamp = true;
			break;
		case EDFMGamePlayMode::GamePlayMode_SafeHouse:
			break;
		case EDFMGamePlayMode::GamePlayMode_Intro:
			break;
		case EDFMGamePlayMode::EDFMGamePlayMode_MAX:
			break;
		default:
			break;
		}
		return bIsCamp;
	}

	/**
	* UpData：每帧执行核心逻辑
	* 1. 获取主流程状态，判断是否在对局
	* 2. 获取 GWorld、GameState、本地角色 MyCharacter
	* 3. 获取队伍/阵营 ID 存到 GameData::TeamIndex
	* 4. 遍历全部 ADFMCharacter 角色，过滤自己、队友、死亡
	* 5. 对敌人使用游戏原生轮廓组件强制开启轮廓高亮
	* 6. 释放 TArray 内存，防止内存泄漏
	*/
	static void UpData()
	{
		// 1. 通过蓝图工具类获取主流程状态（大厅/加载/对局）
		auto MainFlowState = GameData::BlueprintFunctionLibrary[(int)BlueprintType::GPScalabilityBlueprintTools]->GetMainFlowState();

		// 注意：这里逻辑小bug，SafeHouse 安全屋也判断 bIsInGame=true
		GameData::bIsInGame = MainFlowState == EMainFlowState::InGame || MainFlowState == EMainFlowState::SafeHouse;
		if (!GameData::bIsInGame) return;	// 不在对局直接退出本帧逻辑

		// 获取 GameViewportClient 接口对象，读取 UWorld
		auto GameViewportClient = Mem::Read<UGameViewportClient*>((uintptr_t)GameData::Engine + Offsets::GameViewport);
		if (IsBadPtr(GameViewportClient)) return;
		GameData::GWorld = Mem::Read<UWorld*>((uintptr_t)GameViewportClient + Offsets::World);
		if (IsBadPtr(GameData::GWorld)) return;

		// 通过蓝图工具类获取 GPGameState 游戏状态对象
		auto GameState = GameData::BlueprintFunctionLibrary[(int)BlueprintType::GameplayBlueprintHelper]->GetGPGameState(GameData::GWorld);
		if (IsBadPtr(GameState)) return;

		// 获取当前游戏模式（突破/全面/决战等）
		auto DFMGamePlayerMode = Mem::Read<EDFMGamePlayMode>(GameState + Offsets::DFMGamePlayerMode);
		GameData::bIsCampMode = GetIsCamp(DFMGamePlayerMode);

		// 获取本地玩家角色指针 MyCharacter
		GameData::MyCharacter = GameData::BlueprintFunctionLibrary[(int)BlueprintType::GameplayBlueprintHelper]->GetLocalGPCharacter(GameData::GWorld);
		if (IsBadPtr(GameData::MyCharacter)) return;

		// 判断是否本地玩家，是才读取队伍信息
		if (GameData::BlueprintFunctionLibrary[(int)BlueprintType::GameplayBlueprintHelper]->IsLocalPlayer(GameData::MyCharacter))
		{
			auto HealthDataComponent = GameData::MyCharacter->GetHealthComp();
			if (IsBadPtr(HealthDataComponent)) return;
			if (HealthDataComponent->GetHealth() > 0 && !GameData::MyCharacter->IsDead())
			{
				auto TeamComp = GameData::MyCharacter->GetTeamComp();
				if (IsBadPtr(TeamComp)) return;
				// 模式区分：突破模式用 GetCamp 阵营，其他模式用 GetTeamID 队伍 ID
				if (GameData::bIsCampMode)
					GameData::TeamIndex = TeamComp->GetCamp();
				else
					GameData::TeamIndex = TeamComp->GetTeamID();
			}
		}

		// ---------------- 遍历全部角色 Actor ----------------
		TArray<AActor*> ACharacters = {};
		// 调用 GameplayStatics::GetAllActorsOfClass 获取全部 ADFMCharacter 角色
		GameData::BlueprintFunctionLibrary[(int)BlueprintType::GameplayStatics]->GetAllActorsOfClass(GameData::GWorld, ADFMCharacter::StaticClass(), &ACharacters);

		for (auto i = 0; i < ACharacters.Num(); i++)
		{
			const auto& pCharacter = ACharacters[i]->As<ADFMCharacter*>();
			// 过滤自己和已经死亡的角色
			if (pCharacter == GameData::MyCharacter || !pCharacter->IsAlive()) continue;

			auto TeamComp = pCharacter->GetTeamComp();
			if (IsBadPtr(TeamComp)) continue;
			// 根据模式获取目标的队伍/阵营 ID
			auto TeamIndex = GameData::bIsCampMode ? TeamComp->GetCamp() : TeamComp->GetTeamID();
			if (TeamIndex == GameData::TeamIndex) continue;	// 队友直接跳过，不处理队友

			// 获取目标的 UGPOutLineEffectComponent
			auto DFMOutLineEffectComponent = Mem::Read<UGPOutLineEffectComponent*>(
				reinterpret_cast<uintptr_t>(pCharacter) + Offsets::DFMOutLineEffectComponent);
			if (IsBadPtr(DFMOutLineEffectComponent)) continue;

			// 计算需要施加的轮廓效果
			auto desiredEffect = EOutLineEffectType::OutLineType_None;
			if (pCharacter->IsAI())
			{
				// AI 敌人：使用可救援提示
				desiredEffect = EOutLineEffectType::OutLineType_DyingShowTeammateCanRescueSelf;
			}
			else
			{
				// 玩家敌人：使用传感器 ProxSensor 高亮（黄色轮廓）
				desiredEffect = EOutLineEffectType::OutLineType_ProxSensor;
			}
			// 获取当前轮廓效果值，如果和目标不同才调用 PlayOutLineEffect，避免每帧重复调用
			auto currentEffect = Mem::Read<EOutLineEffectType>(
				reinterpret_cast<uintptr_t>(DFMOutLineEffectComponent) + 0x1D8);
			if (currentEffect != desiredEffect)
			{
				DFMOutLineEffectComponent->PlayOutLineEffect(desiredEffect);
			}
		}

		// 重要：必须释放 GetAllActorsOfClass 返回的 TArray 内存，防止 DLL 内存泄漏
		if (ACharacters.Data)
			Utils::FastCall(Offsets::FreeFunc, ACharacters.Data);
	}
};