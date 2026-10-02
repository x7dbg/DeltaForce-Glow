#pragma once

class C_Hack
{
public:
	/**
	* hkPostRender - Hook 回调函数，劫持 UGameViewportClient::PostRender（虚表索引 95）
	* 每帧渲染时调用，用于 ESP 和其他视觉逻辑
	* @param GameViewport 视口客户端对象
	* @param Canvas 画布对象用于渲染
	*/
	static void hkPostRender(void* GameViewport, UCanvas* Canvas)
	{
		C_Entity::UpData();	// 更新所有实体，读取世界/关卡/角色信息
		// 使用 FastCall 调用原始游戏函数（64 位调用约定）
		return Utils::FastCall<void>(RenderHook.GetOriginal<void*>(95), GameViewport, Canvas);
	}

	/**
	* FindOffset - 运行时动态查找 UE 成员偏移
	* 无需使用 IDA 或硬编码偏移，直接从游戏内存 UStruct 读取
	* @param Class 完整类名字符串，例如 L"Engine.Actor"
	* @param varName 成员变量名称（char*）
	* @return 成员在对象中的偏移，未找到返回 0
	*/
	static int FindOffset(const wchar_t* Class, const char* varName)
	{
		// 通过 UObject 系统查找 UStruct
		auto CurrentObject = (UStruct*)UObject::FindObject(Class);
		if (IsBadPtr(CurrentObject)) return NULL;

		// 遍历 ChildProperties 查找成员属性
		for (auto Property = CurrentObject->ChildProperties; !IsBadPtr(Property); Property = Property->Next)
		{
			auto Offset = Property->Offset;
			// 验证偏移范围
			if (Offset > 0 && Offset < 0xFFFF)
			{
				// 比较名称，匹配目标成员
				if (strcmp(Property->Name.GetName().c_str(), varName) == 0)
				{
					return Offset;
				}
			}
		}
		return NULL;
	}

	/**
	* InitOffsets - 初始化所有全局偏移量
	* 注意：仅 StaticFindObject、GetName、FreeFunc、GEngine（4 个值）需要在游戏大版本更新时手动修改
	* 其他通过 FindOffset 动态查找，小更新无需修改
	*/
	static void InitOffsets()
	{
		// ---------------- 这 4 个 RVA 需要在游戏版本更新时手动更新 ----------------
		Offsets::StaticFindObject = 0x14AE8C310;
		DeBug(("StaticFindObject:0x%llX\n"), Offsets::StaticFindObject);
		Offsets::GetName = 0x14AB2CA30;
		DeBug(("GetName:0x%llX\n"), Offsets::GetName);
		Offsets::FreeFunc = 0x14A9F0B90;
		DeBug(("FreeFunc:0x%llX\n"), Offsets::FreeFunc);
		Offsets::GEngine = 0x153A3D2A8;
		DeBug(("GEngine:0x%llX\n"), Offsets::GEngine);

		// 动态查找成员偏移，小更新无需修改
		Offsets::GameViewport = FindOffset((L"Engine.Engine"), ("GameViewport"));
		DeBug(("GameViewport:0x%X\n"), Offsets::GameViewport);
		Offsets::World = FindOffset((L"Engine.GameViewportClient"), ("World"));
		DeBug(("World:0x%X\n"), Offsets::World);
		Offsets::Levels = FindOffset((L"Engine.World"), ("Levels"));
		DeBug(("Levels:0x%X\n"), Offsets::Levels);
		Offsets::GameState = FindOffset((L"Engine.World"), ("GameState"));
		DeBug(("GameState:0x%X\n"), Offsets::GameState);
		Offsets::DFMGamePlayerMode = FindOffset((L"DFMGameplay.DFMGameState"), ("DFMGamePlayerMode"));
		DeBug(("DFMGamePlayerMode:0x%X\n"), Offsets::DFMGamePlayerMode);
		Offsets::PlayerController = FindOffset((L"Engine.Player"), ("PlayerController"));
		DeBug(("PlayerController:0x%X\n"), Offsets::PlayerController);
		Offsets::PlayerCameraManager = FindOffset((L"Engine.PlayerController"), ("PlayerCameraManager"));
		DeBug(("PlayerCameraManager:0x%X\n"), Offsets::PlayerCameraManager);
		Offsets::PlayerState = FindOffset((L"Engine.Pawn"), ("PlayerState"));
		DeBug(("PlayerState:0x%X\n"), Offsets::PlayerState);
		Offsets::PlayerNamePrivate = FindOffset((L"Engine.PlayerState"), ("PlayerNamePrivate"));
		DeBug(("PlayerNamePrivate:0x%X\n"), Offsets::PlayerNamePrivate);
		Offsets::RootComponent = FindOffset((L"Engine.Actor"), ("RootComponent"));
		DeBug(("RootComponent:0x%X\n"), Offsets::RootComponent);

		Offsets::DFMOutLineEffectComponent = FindOffset((L"DFMGameplay.DFMCharacter"), ("DFMOutLineEffectComponent"));
		DeBug(("DFMOutLineEffectComponent:0x%X\n"), Offsets::DFMOutLineEffectComponent);

		// 缓存蓝图函数库对象，后续直接调用
		GameData::BlueprintFunctionLibrary[(int)BlueprintType::GameplayStatics] = (UBlueprintFunctionLibrary*)UObject::FindObject((L"Engine.GameplayStatics"));
		DeBug(("BlueprintFunctionLibrary->%llX\n"), GameData::BlueprintFunctionLibrary[(int)BlueprintType::GameplayStatics]);
		GameData::BlueprintFunctionLibrary[(int)BlueprintType::KismetTextLibrary] = (UBlueprintFunctionLibrary*)UObject::FindObject((L"Engine.KismetTextLibrary"));
		DeBug(("BlueprintFunctionLibrary->%llX\n"), GameData::BlueprintFunctionLibrary[(int)BlueprintType::KismetTextLibrary]);
		GameData::BlueprintFunctionLibrary[(int)BlueprintType::KismetStringLibrary] = (UBlueprintFunctionLibrary*)UObject::FindObject((L"Engine.KismetStringLibrary"));
		DeBug(("BlueprintFunctionLibrary->%llX\n"), GameData::BlueprintFunctionLibrary[(int)BlueprintType::KismetStringLibrary]);
		GameData::BlueprintFunctionLibrary[(int)BlueprintType::GameplayBlueprintHelper] = (UBlueprintFunctionLibrary*)UObject::FindObject((L"GPGameplay.GameplayBlueprintHelper"));
		DeBug(("BlueprintFunctionLibrary->%llX\n"), GameData::BlueprintFunctionLibrary[(int)BlueprintType::GameplayBlueprintHelper]);
		GameData::BlueprintFunctionLibrary[(int)BlueprintType::GPScalabilityBlueprintTools] = (UBlueprintFunctionLibrary*)UObject::FindObject((L"GPSettings.GPScalabilityBlueprintTools"));
		DeBug(("BlueprintFunctionLibrary->%llX\n"), GameData::BlueprintFunctionLibrary[(int)BlueprintType::GPScalabilityBlueprintTools]);
		GameData::BlueprintFunctionLibrary[(int)BlueprintType::KismetSystemLibrary] = (UBlueprintFunctionLibrary*)UObject::FindObject((L"Engine.KismetSystemLibrary"));
		DeBug(("BlueprintFunctionLibrary->%llX\n"), GameData::BlueprintFunctionLibrary[(int)BlueprintType::KismetSystemLibrary]);
		GameData::BlueprintFunctionLibrary[(int)BlueprintType::BlueprintFunctionLibrary] = (UBlueprintFunctionLibrary*)UObject::FindObject((L"Engine.BlueprintFunctionLibrary"));
		DeBug(("BlueprintFunctionLibrary->%llX\n"), GameData::BlueprintFunctionLibrary[(int)BlueprintType::BlueprintFunctionLibrary]);

		// 获取全局 GEngine 指针并存储到 GameData
		GameData::Engine = Mem::Read<UObject*>(Offsets::GEngine);
	}

	/**
	* InitHooks - 初始化所有 Hook
	* 获取 GameViewportClient 对象，Hook 函数 95（PostRender）并替换为 hkPostRender
	*/
	static void InitHooks()
	{
		if (!IsBadPtr(GameData::Engine))
		{
			auto GameViewportClient = *(UGameViewportClient**)((uintptr_t)GameData::Engine + Offsets::GameViewport);
			if (!IsBadPtr(GameViewportClient))
			{
				RenderHook.Initialize(GameViewportClient);
				RenderHook.Bind(95, hkPostRender);
			}
		}
	}

	/**
	* Init - 旧版入口，等待 HOME 键后初始化偏移量 + Hook
	*/
	static void Init(HMODULE hModule)
	{
		// 循环等待 HOME 键按下
		while (!(GetAsyncKeyState(VK_HOME) & 0x8000))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		Beep(800, 500);	// PC 扬声器提示音表示初始化开始
		InitOffsets();
		InitHooks();
	}

	/**
	* Init2 - DllMain 中使用的实际入口，创建新线程
	* 功能：HOME 等待 + 初始化 + F1 切换 Patch + 常驻轮询循环
	*/
	static void Init2(HMODULE hModule)
	{
		BYTE originalBytes[5] = { 0 };				// 保存原始指令字节，用于关闭时还原
		BOOL featureEnabled = FALSE;				// 功能开关状态
		BOOL bytesInitialized = FALSE;				// 是否成功获取原始字节
		BOOL f1KeyPressed = FALSE;					// 防抖标志，防止按住 F1 连续切换
		DWORD64 targetAddress = 0x140000000 + 0x199E3A5; // 需要 Patch 的 RVA，0x140000000 是 EXE 基址

		// Lambda：封装 ReadProcessMemory 读取内存字节
		auto readBytes = [](DWORD64 address, BYTE* buffer, SIZE_T size) -> BOOL
		{
			HANDLE hProcess = GetCurrentProcess();
			SIZE_T bytesRead;
			if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, &bytesRead))
			{
				return bytesRead == size;
			}
			return FALSE;
		};

		// Lambda：封装写内存，修改页权限为可读写执行，写入后恢复权限
		auto writeBytes = [](DWORD64 address, BYTE* buffer, SIZE_T size) -> BOOL
		{
			HANDLE hProcess = GetCurrentProcess();
			DWORD oldProtect;
			if (VirtualProtectEx(hProcess, (LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect))
			{
				SIZE_T bytesWritten;
				BOOL success = WriteProcessMemory(hProcess, (LPVOID)address, buffer, size, &bytesWritten);
				VirtualProtectEx(hProcess, (LPVOID)address, size, oldProtect, &oldProtect);
				return success && (bytesWritten == size);
			}
			return FALSE;
		};

		// 循环等待 HOME 键按下，注意注入后不会自动生效需要手动按 HOME
		while (!(GetAsyncKeyState(VK_HOME) & 0x8000))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		Beep(800, 500);

		// 读取目标地址原始 5 字节备份
		if (readBytes(targetAddress, originalBytes, 5))
		{
			bytesInitialized = TRUE;
		}

		// 调试用：将字节数组转十六进制字符串，仅注释未实际调用
		auto bytesToHex = [](BYTE* bytes, SIZE_T size) -> std::string
		{
			std::string result;
			char hex[3];
			for (SIZE_T i = 0; i < size; i++)
			{
				sprintf_s(hex, sizeof(hex), "%02X", bytes[i]);
				result += hex;
				if (i < size - 1) result += " ";
			}
			return result;
		};

		InitOffsets();
		InitHooks();

		// 主循环轮询，10ms 查询一次按键状态
		while (true)
		{
			if (bytesInitialized)
			{
				// F1 按下，位图检测按键，F1 抬起才触发切换
				if (GetAsyncKeyState(VK_F1) & 0x8000)
				{
					if (!f1KeyPressed)
					{
						f1KeyPressed = TRUE;
						featureEnabled = !featureEnabled; // 切换状态翻转

						if (featureEnabled)
						{
							// 开启：写入新机器码 {0xF3,0x0F,0x10,0x40,0x3C} 对应汇编 movss xmm0,dword ptr [rax+0x3C]
							BYTE patchBytes[] = { 243, 15, 16, 64, 60 };
							writeBytes(targetAddress, patchBytes, 5);
						}
						else
						{
							// 关闭：写入原始机器码字节，还原游戏逻辑
							writeBytes(targetAddress, originalBytes, 5);
						}
					}
				}
				else
				{
					f1KeyPressed = FALSE; // F1 松开重置标志
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
};