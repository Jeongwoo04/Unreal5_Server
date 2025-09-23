#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "Protocol.pb.h"

using json = nlohmann::json;
using namespace Protocol;
using namespace std;
using int32 = __int32;

enum class ClientActionType
{
    None,
    PlayAnimation,
    PlayEffect,
    SpawnProjectile,
    SpawnField,
    Move
};

struct ClientAction
{
    ClientActionType actionType = ClientActionType::None;

    float actionDelay = 0.f;

    string animName;

    string effectName;
    string attachBone;

    int32 dataId = 0;

    float moveDistance = 0.f;
};

struct MarkerData
{
    // shape . . .
    float distance = 0.f;
    float range = 0.f;
};

struct ProjectileInfo
{
    int32 dataId = 0;
    string name = "";
    float distance = 0.f;
    float range = 0.f;
};

struct FieldInfo
{
    int32 dataId = 0;
    string name = "";
    float distance = 0.f;
    float range = 0.f;
};

// Skill 구조체
struct Skill
{
    int32 id = 0;
    string name;
    string iconPath;
    float cooldown = 0.f;
    float castTime = 0.f;
    bool marker = false;
    MarkerData markerData;

    vector<ClientAction> actions;
};

ClientActionType ToActionType(const string& str);

// 템플릿 인터페이스
template<typename Key, typename Value>
class ILoader
{
public:
    virtual unordered_map<Key, Value> MakeDict() = 0;
    virtual ~ILoader() = default;
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

class FieldData : public ILoader<int32, FieldInfo>
{
public:
    vector<FieldInfo> fields;

    unordered_map<int32, FieldInfo> MakeDict() override;

    static FieldData LoadFromJsonFile(const string& path); // 이름 다르게!
};

// SkillData
class SkillData : public ILoader<int32, Skill>
{
public:
    vector<Skill> skills;

    unordered_map<int32, Skill> MakeDict() override;

    static SkillData LoadFromJsonFile(const string& path); // 이름 다르게!
};