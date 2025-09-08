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
    Move,
    Attack,
    SpawnProjectile,
    SpawnField,
    ApplyStatus,
    React
};

struct ActionData
{
    ActionType type;

    // 공통
    float value = 0;
    float radius = 0;
    float angle = 0;
    float duration = 0;

    // Attack 전용
    float damage = 0;

    // Projectile / Field 전용
    int32 dataId = 0;

    // Status 전용
    int32 statusId;
};

// Skill 구조체
struct Skill
{
    int32 id = 0;
    string name;
    float cooldown = 0;
    SkillType skillType = SkillType::SKILL_AUTO;
    vector<ActionData> actions;
};



SkillType ToSkillType(const string& str);
ActionType ToActionType(const std::string& str);

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