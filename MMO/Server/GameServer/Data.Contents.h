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

// ProjectileInfo 구조체
struct ProjectileInfo
{
    string name;
    float speed = 0;
    int32 range = 0;
};

// Skill 구조체
struct Skill
{
    int32 id = 0;
    string name;
    float cooldown = 0;
    int32 damage = 0;
    SkillType skillType = SkillType::SKILL_AUTO;
    ProjectileInfo projectile; // optional
};

SkillType ToSkillType(const string& str);

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

// SkillData
class SkillData : public ILoader<int32, Skill>
{
public:
    vector<Skill> skills;

    unordered_map<int32, Skill> MakeDict() override;

    static SkillData LoadFromJsonFile(const string& path); // 이름 다르게!
};