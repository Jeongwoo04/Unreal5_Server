#pragma once
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "Protocol.pb.h"

using json = nlohmann::json;
using namespace Protocol;

enum class ActionType
{
    None,
    Move,
    Attack,
    SpawnProjectile,
    SpawnField,
    Buff
};

enum class ShapeType
{
    None,
    Circle,
    Cone,
    Rectangle,
    Line
};

struct ActionData
{
    ActionType actionType = ActionType::None;
    float distance = 0.f;
};

struct AttackActionData : public ActionData
{
    ShapeType shape = ShapeType::None;
    int32 damage = 0;
    float radius = 0.f;
    float width = 0.f;
    float length = 0.f;
    float angle = 0.f;
};

struct MoveActionData : public ActionData
{
    float moveDistance = 0.f;
    // float dashDuration = 0.f;
};

struct SpawnActionData : public ActionData
{
    int32 dataId = 0;
};

struct BuffActionData : public ActionData
{
    int32 buffId = 0;
};

// Skill 구조체
struct Skill
{
    int32 id = 0;
    string name;
    float cooldown = 0;
    vector<ActionData> actions;
};

struct ObjectTemplate
{
    int32 dataId;
    string name;
    int32 mainType;
    int32 subType;
};

struct SpawnTable
{
    int32 spawnId;
    int32 dataId;
    Vector3 spawnPos;
    int32 respawnInterval;
    int32 count;
};

struct MapInfo
{
    int32 mapId;
    string mapName;
    string filePath;
    unordered_map<int32, SpawnTable> spawnTables;
};

enum class EffectType
{
    None,
    HP,
    MP,
    Attack,
    Defense,
    Speed
};

struct BuffInfo
{
    int32 buffId = 0;             // 고유 ID
    string name;               // 이름
    float duration = 0.f;           // 지속 시간(초)
    float tickInterval = 0.f;       // Tick 간격(초)
    bool stackable = false;         // 중첩 가능 여부
    int32 maxStacks = 1;          // 최대 중첩
    bool refreshOnApply = true;     // 재적용 시 duration 갱신

    // 효과 수치
    EffectType effectType = EffectType::None;
    float effectValue = 0.f;        // 즉시 적용 효과
    float effectPerTick = 0.f;      // Tick마다 적용 효과

    // 스탯 보정 (slow, debuff)
    float movementModifier = 1.0f;  // 이동 속도 계수 (0.0~1.0)
    float attackModifier = 1.0f;    // 공격력 계수 (0.0~1.0)

    // 상태 제어 (Crowd Control)
    bool isCrowdControl = false;
    bool root = false;
    bool stun = false;
    bool silence = false;

    // 중첩/우선순위
    int32_t priority = 0;            // 값 높을수록 우선 적용
};

SkillType ToSkillType(const string& str);
ActionType ToActionType(const string& str);
ShapeType ToShapeType(const string& str);
EffectType ToEffectType(const string& str);

// 템플릿 인터페이스
template<typename Key, typename Value>
class ILoader
{
public:
    virtual unordered_map<Key, Value> MakeDict() = 0;
    virtual ~ILoader() = default;
};

class ObjectData : public ILoader<int32, ObjectTemplate>
{
public:
    vector<ObjectTemplate> objectTemplates;

    unordered_map<int32, ObjectTemplate> MakeDict() override;

    static ObjectData LoadFromJson(const string& path);
};

// StatData
class StatData : public ILoader<int32, StatInfo>
{
public:
    vector<StatInfo> stats;

    unordered_map<int32, StatInfo> MakeDict() override;

    static StatData LoadFromJson(const string& path);
};

// ProjectileData
class ProjectileData : public ILoader<int32, ProjectileInfo>
{
public:
    vector<ProjectileInfo> projectiles;

    unordered_map<int32, ProjectileInfo> MakeDict() override;

    static ProjectileData LoadFromJsonFile(const string& path); // 이름 다르게!
};

// SkillData
class SkillData : public ILoader<int32, Skill>
{
public:
    vector<Skill> skills;

    unordered_map<int32, Skill> MakeDict() override;

    static SkillData LoadFromJsonFile(const string& path); // 이름 다르게!
};

class MapData : public ILoader<int32, MapInfo>
{
public:
    vector<MapInfo> maps;

    unordered_map<int32, MapInfo> MakeDict() override;

    static MapData LoadFromJsonFile(const string& path);
};

class BuffData : public ILoader<int32, BuffInfo>
{
public:
    vector<BuffInfo> buffs;

    unordered_map<int32, BuffInfo> MakeDict() override;

    static BuffData LoadFromJsonFile(const string& path);
};