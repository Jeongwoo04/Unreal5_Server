#pragma once
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace Protocol;

// Skill 구조체
struct Skill
{
    int32 id = 0;
    string name;
    float cooldown = 0;
    int32 damage = 0;
    SkillType skillType = SkillType::SKILL_AUTO;
    float distance = 0.f;
    int32 projectileId;
};

SkillType ToSkillType(const string& str);

struct ObjectTemplate
{
    string name = "";
    int32 mainType = 0;
    int32 subType = 0;
    int32 statId = -1;
    int32 projectileId = -1;
};

// 템플릿 인터페이스
template<typename Key, typename Value>
class ILoader
{
public:
    virtual unordered_map<Key, Value> MakeDict() = 0;
    virtual ~ILoader() = default;
};

class ObjectData : public ILoader<string, ObjectTemplate>
{
public:
    vector<ObjectTemplate> objectTemplates;

    unordered_map<string, ObjectTemplate> MakeDict() override;

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