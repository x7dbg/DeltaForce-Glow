template<class T>
struct TArray
{
public:
    T* Data;                // 数组数据指针，虚幻堆内存
    int32_t Count;          // 当前有效元素数量
    int32_t Max;            // 已分配最大容量

    TArray()
    {
        Data = nullptr;
        Max = 0;
        Count = 0;
    }

    // 返回数组有效元素个数
    int Num() const
    {
        return Count;
    }

    // 下标运算符 读写
    T& operator[](int i)
    {
        return Data[i];
    }

    // 下标运算符 只读
    const T& operator[](int i) const
    {
        return Data[i];
    }

    // 判断索引是否合法
    bool IsValidIndex(int i) const
    {
        return i < Num();
    }

    // 本地释放，注意：虚幻分配的内存不能调用此Free
    void Free()
    {
        delete[] Data;
        Count = 0;
        Max = 0;
        Data = nullptr;
    }

    // 添加元素
    void Add(const T& Element)
    {
        if (GetSlack() <= 3) Reserve(3);
        Data[Count] = Element;
        Count++;
    }

    // 获取剩余可用空间
    int32_t GetSlack() const { return Max - Count; }

    // 扩容，重新分配内存并拷贝旧数据
    void Reserve(int32_t Count)
    {
        Max += Count;
        T* NewData = new T[Max];
        if (Data)
        {
            std::copy(Data, Data + Count, NewData);
            delete[] Data;
        }
        Data = NewData;
    }
};

// 虚幻三维向量，世界坐标使用
struct FVector
{
public:
    float X;
    float Y;
    float Z;

    FVector() : X(0), Y(0), Z(0) {}
    FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}

    FVector operator + (const FVector& other) const { return FVector(X + other.X, Y + other.Y, Z + other.Z); }
    FVector operator - (const FVector& other) const { return FVector(X - other.X, Y - other.Y, Z - other.Z); }
    FVector operator * (float scalar) const { return FVector(X * scalar, Y * scalar, Z * scalar); }
    FVector operator * (const FVector& other) const { return FVector(X * other.X, Y * other.Y, Z * other.Z); }
    FVector operator / (float scalar) const { return FVector(X / scalar, Y / scalar, Z / scalar); }
    FVector operator^(const FVector& V) const { return FVector(Y * V.Z - Z * V.Y, Z * V.X - X * V.Z, X * V.Y - Y * V.X); }
    FVector operator / (const FVector& other) const { return FVector(X / other.X, Y / other.Y, Z / other.Z); }

    FVector& operator=  (const FVector& other) { X = other.X; Y = other.Y; Z = other.Z; return *this; }
    FVector& operator+= (const FVector& other) { X += other.X; Y += other.Y; Z += other.Z; return *this; }
    FVector& operator-= (const FVector& other) { X -= other.X; Y -= other.Y; Z -= other.Z; return *this; }
    FVector& operator*= (const float other) { X *= other; Y *= other; Z *= other; return *this; }

    // 点积
    float Dot(const FVector& b) const { return (X * b.X) + (Y * b.Y) + (Z * b.Z); }
    // 距离平方
    float Distance(FVector v) { return float(sqrt(pow(v.X - X, 2.0) + pow(v.Y - Y, 2.0) + pow(v.Z - Z, 2.0))); }
    // 模长平方
    float MagnitudeSqr() const { return Dot(*this); }
    // 模长
    float Magnitude() const { return sqrt(MagnitudeSqr()); }
    // 单位化向量
    FVector Unit() const
    {
        const float fMagnitude = Magnitude();
        return FVector(X / fMagnitude, Y / fMagnitude, Z / fMagnitude);
    }

    friend bool operator==(const FVector& first, const FVector& second) { return first.X == second.X && first.Y == second.Y && first.Z == second.Z; }
    friend bool operator!=(const FVector& first, const FVector& second) { return !(first == second); }
};

// 虚幻二维向量，屏幕坐标使用
struct FVector2D
{
public:
    float X;
    float Y;

    FVector2D() : X(0), Y(0) {}
    FVector2D(float x, float y) : X(x), Y(y) {}

    // 判断是否近似零点
    bool Zero() const
    {
        return (X > -0.1f && X < 0.1f && Y > -0.1f && Y < 0.1f);
    }

    // 屏幕两点距离
    float ScreenDis(float GameX, float GameY) {
        FVector2D Vec;
        Vec.X = GameX - X;
        Vec.Y = GameY - Y;
        float dis = sqrt(Vec.X * Vec.X + Vec.Y * Vec.Y);
        return dis;
    }

    FVector2D operator + (const FVector2D& other) const { return FVector2D(X + other.X, Y + other.Y); }
    FVector2D operator - (const FVector2D& other) const { return FVector2D(X - other.X, Y - other.Y); }
    FVector2D operator * (float scalar) const { return FVector2D(X * scalar, Y * scalar); }
    FVector2D operator * (const FVector2D& other) const { return FVector2D(X * other.X, Y * other.Y); }
    FVector2D operator / (float scalar) const { return FVector2D(X / scalar, Y / scalar); }
    FVector2D operator / (const FVector2D& other) const { return FVector2D(X / other.X, Y / other.Y); }

    FVector2D& operator=  (const FVector2D& other) { X = other.X; Y = other.Y; return *this; }
    FVector2D& operator+= (const FVector2D& other) { X += other.X; Y += other.Y; return *this; }
    FVector2D& operator-= (const FVector2D& other) { X -= other.X; Y -= other.Y; return *this; }
    FVector2D& operator*= (const float other) { X *= other; Y *= other; return *this; }
};

// FName 虚幻名字池结构，Index+Number映射字符串
struct FName {
    int32_t Index;      // NameTable索引
    int32_t Number;     // 序号

    // 获取名字转std::string
    std::string GetName()
    {
        wchar_t Unicode[1024];
        auto _Len = Utils::FastCall<int>(Offsets::GetName, this, Unicode);
        std::string name = Utils::WCHAR2String(Unicode);
        auto pos = name.rfind('/');
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 1);
        }
        return name;
    }

    // 获取名字转std::wstring
    std::wstring GetNameW()
    {
        wchar_t Unicode[1024];
        auto _Len = Utils::FastCall<int>(Offsets::GetName, this, Unicode);
        auto Name = std::wstring(Unicode);
        auto pos = Name.rfind('/');
        if (pos != std::wstring::npos)
        {
            Name = Name.substr(pos + 1);
        }
        return Name;
    }

    // 通过FName索引直接读取名字
    static std::string GetName(int id) {
        wchar_t Unicode[1024];
        FName NameID = { id ,0 };
        auto _Len = Utils::FastCall<int>(Offsets::GetName, &NameID, Unicode);
        if (_Len <= 0 || _Len > 1024) {
            return "";
        }
        std::string name = Utils::WCHAR2String(Unicode);
        auto pos = name.rfind('/');
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 1);
        }
        return name;
    }
};

// UField 字段基类
class UField
{
public:
    uint8_t Pad_00[0x18];
    class UField* Next;         // 下一个Field
    uint8_t Pad_20[0x08];
    FName Name;                 // 字段名字
    uint8_t Pad_30[0x24];
    uint32_t Offset;            // 成员偏移
    uint8_t Pad_58[0x38];
};

// UObject 虚幻所有对象根基类
class UObject
{
public:
    void** VFTable;                 // 虚表指针
    class UClass* ClassPrivate;     // 对象所属UClass
    class UObject* OuterPrivate;    // Outer外层对象
    int32_t InternalIndex;          // 对象索引
    FName NamePrivate;              // 对象名字FName

    // 判断对象是否属于目标类（向上遍历继承链）
    bool IsA(void* cmp)
    {
        for (auto super = ClassPrivate; super; super = Mem::Read<UClass*>((ULONG64)super + 0x48))
        {
            if (super == cmp) {
                return true;
            }
        }
        return false;
    }

    // ProcessEvent调用蓝图UFunction，fn为UFunction*，parms参数结构体指针
    void ProcessEvent(void* fn, void* parms)
    {
        auto vtable = *reinterpret_cast<void***>(this);
        auto call = vtable[0x44];   // ProcessEvent虚表索引
        if (*(BYTE*)call == 0xE9 || *(BYTE*)call == 0xE8)
            return;
        Utils::FastCall<bool>(call, this, fn, parms);
    }

    // 获取对象名字std::string
    std::string GetName()
    {
        return  NamePrivate.GetName();
    }

    // 获取对象名字std::wstring
    std::wstring GetNameW()
    {
        return  NamePrivate.GetNameW();
    }

    // FindObject查找UObject
    static UObject* FindObject(const wchar_t* name)
    {
        return Utils::FastCall<UObject*>(Offsets::StaticFindObject, (uintptr_t)0, (uintptr_t)-1, name, false);
    }
};

// UStruct 结构体基类，继承UObject
class UStruct : public UObject
{
public:
    uint8_t Pad_20[0x28];
    class UStruct* SuperStruct;     // 父结构体
    uint8_t Pad_50[0x20];
    class UField* ChildProperties; // 属性链表头
};

// UClass 类对象，继承UStruct
class UClass : public UStruct
{
public:
};

// UCanvas 画布类
class UCanvas : public UObject {
public:
};

// AActor 游戏中可放置Actor基类
class AActor : public UObject
{
public:
    // 获取Actor静态UClass
    static UClass* StaticClass()
    {
        static UClass* Class;
        if (!Class) Class = (UClass*)FindObject((L"Engine.Actor"));
        return Class;
    }

    // 安全类型转换模板
    template<typename T>
    inline T As()
    {
        if (!this)
            return nullptr;
        return static_cast<T>(this);
    }
};

// APawn 可被控制器拥有的Actor
class APawn : public AActor
{
public:

};

// UWorld 世界对象，保存关卡、Actor集合
class UWorld : public UObject
{
public:

};

// UGameViewportClient 视口客户端，渲染主入口
class UGameViewportClient : public UObject
{
public:
};

// UGPHealthDataComponent 血量组件
class UGPHealthDataComponent : public AActor
{
public:
    // 调用蓝图函数获取当前血量
    float GetHealth() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPHealthDataComponent.GetHealth"));
        struct {
            float ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 调用蓝图函数获取最大血量
    float GetHealthMax() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPHealthDataComponent.GetHealthMax"));
        struct {
            float ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }
};

// UGPTeamComponent 队伍/阵营组件
class UGPTeamComponent : public AActor
{
public:
    // 获取小队TeamID
    int32_t GetTeamID() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPTeamComponent.GetTeamID"));
        struct {
            int32_t ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 获取突破模式Camp阵营
    int32_t GetCamp() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPTeamComponent.GetCamp"));
        struct {
            int32_t ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }
};

// ACharacter 角色Pawn基类
class ACharacter : public APawn
{
public:
    static UClass* StaticClass()
    {
        static UClass* Class;
        if (!Class) Class = (UClass*)FindObject((L"Engine.CHARACTER"));
        return Class;
    }
};

// AIntCharacter 内部角色基类
class AIntCharacter : public ACharacter {
public:
    static UClass* StaticClass()
    {
        static UClass* Class;
        if (!Class) Class = (UClass*)FindObject((L"GPGameplay.IntCharacter"));
        return Class;
    }
};

// ACharacterBase 角色基础逻辑类
class ACharacterBase : public AIntCharacter {
public:
    static UClass* StaticClass()
    {
        static UClass* Class;
        if (!Class) Class = (UClass*)FindObject((L"GPGameplay.CharacterBase"));
        return Class;
    }

    // 判断角色是否死亡
    bool IsDead() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.CharacterBase.IsDead"));
        struct {
            bool ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }
};

// AGPCharacterBase GP角色基类
class AGPCharacterBase : public ACharacterBase {
public:
    static UClass* StaticClass()
    {
        static UClass* Class;
        if (!Class) Class = (UClass*)FindObject((L"GPGameplay.GPCharacterBase"));
        return Class;
    }

    // 判断角色是否存活
    char IsAlive() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPCharacterBase.IsAlive"));
        struct {
            char ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 判断是否AI角色
    bool IsAI() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPCharacterBase.IsAI"));
        struct {
            bool ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 判断是否玩家
    bool IsPlayer() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPCharacterBase.IsPlayer"));
        struct {
            bool ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 获取队伍组件指针
    UGPTeamComponent* GetTeamComp() {
        UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPCharacterBase.GetTeamComp"));
        struct {
            UGPTeamComponent* ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 获取血量组件指针
    UGPHealthDataComponent* GetHealthComp() {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPCharacterBase.GetHealthComp"));
        struct {
            UGPHealthDataComponent* ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }
};

// AGPCharacter GP游戏角色
class AGPCharacter : public AGPCharacterBase {
public:
    static UClass* StaticClass()
    {
        static UClass* Class;
        if (!Class) Class = (UClass*)FindObject((L"GPGameplay.GPCharacter"));
        return Class;
    }
};

// ADFMCharacter 三角洲行动最终角色类
class ADFMCharacter : public AGPCharacter {
public:
    static UClass* StaticClass()
    {
        static UClass* Class;
        if (!Class) Class = (UClass*)FindObject((L"DFMGameplay.DFMCharacter"));
        return Class;
    }
};

// UBlueprintFunctionLibrary 蓝图工具库封装
class UBlueprintFunctionLibrary : public UObject
{
public:
    // 获取世界中所有指定类的Actor
    void GetAllActorsOfClass(UObject* WorldContextObject, UClass* ActorClass, TArray<AActor*>* OutActors)
    {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"Engine.GameplayStatics.GetAllActorsOfClass"));
        struct
        {
            UObject* WorldContextObject;
            UClass* ActorClass;
            TArray<AActor*> OutActors;
        } params{};
        params.WorldContextObject = WorldContextObject;
        params.ActorClass = ActorClass;
        if (fn)
            ProcessEvent(fn, &params);
        if (OutActors != nullptr)
            *OutActors = params.OutActors;
    }

    // 判断是否本地玩家
    bool IsLocalPlayer(UObject* WorldContextObject)
    {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GameplayBlueprintHelper.IsLocalPlayer"));
        struct {
            UObject* WorldContextObject;
            bool ReturnValue;
        } Params{};
        Params.WorldContextObject = WorldContextObject;
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 获取GP游戏状态对象指针
    uintptr_t GetGPGameState(UObject* WorldContextObject)
    {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GameplayBlueprintHelper.GetGPGameState"));
        struct {
            UObject* WorldContextObject;
            uintptr_t ReturnValue;
        } Params{};
        Params.WorldContextObject = WorldContextObject;
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 获取本地GPCharacter角色
    AGPCharacter* GetLocalGPCharacter(UObject* WorldContextObject)
    {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GameplayBlueprintHelper.GetLocalGPCharacter"));
        struct {
            UObject* WorldContextObject;
            AGPCharacter* ReturnValue;
        } Params{};
        Params.WorldContextObject = WorldContextObject;
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 获取主流程状态（大厅/对局/安全屋）
    enum EMainFlowState GetMainFlowState()
    {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPSettings.GPScalabilityBlueprintTools.GetMainFlowState"));
        struct {
            enum EMainFlowState ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }

    // 获取游戏模式
    int32_t GetGameMode()
    {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPSettings.GPScalabilityBlueprintTools.GetGameMode"));
        struct {
            int32_t ReturnValue;
        } Params{};
        if (fn)
            ProcessEvent(fn, &Params);
        return Params.ReturnValue;
    }
};

// UGPOutLineEffectComponent 角色描边高亮组件
class UGPOutLineEffectComponent : public UObject {
public:
    // 播放指定类型描边效果
    void PlayOutLineEffect(enum class EOutLineEffectType OutLineType) {
        static UObject* fn = nullptr;
        if (fn == nullptr)
            fn = FindObject((L"GPGameplay.GPOutLineEffectComponent.PlayOutLineEffect"));
        struct
        {
            enum class EOutLineEffectType OutLineType;
        } params{};
        params.OutLineType = OutLineType;
        if (fn)
            ProcessEvent(fn, &params);
    }
};

