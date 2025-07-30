#include "pch.h"
#include "Data.Contents.h"
#include <fstream>

std::unordered_map<int, std::shared_ptr<StatInfo>> StatData::MakeDict() const
{
    std::unordered_map<int, std::shared_ptr<StatInfo>> dict;
    for (auto& stat : stats)
    {
        stat->set_hp(stat->maxhp()); // 초기 HP 세팅
        dict[stat->level()] = stat;
    }
    return dict;
}

std::shared_ptr<StatData> StatData::LoadFromJson(const std::string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    auto data = std::make_shared<StatData>();
    for (auto& element : j["stats"])
    {
        auto stat = std::make_shared<StatInfo>();
        stat->set_level(element["level"].get<int>());
        stat->set_maxhp(element["maxHp"].get<int>());
        stat->set_attack(element["attack"].get<int>());
        stat->set_speed(element["speed"].get<float>());
        stat->set_totalexp(element["totalExp"].get<int>());

        data->stats.push_back(stat);
    }
    return data;
}

std::unordered_map<int, std::shared_ptr<Skill>> SkillData::MakeDict() const
{
    std::unordered_map<int, std::shared_ptr<Skill>> dict;
    for (auto& skill : skills)
    {
        dict[skill->id] = skill;
    }
    return dict;
}

std::shared_ptr<SkillData> SkillData::LoadFromJsonFile(const std::string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    auto data = std::make_shared<SkillData>();
    for (auto& element : j["skills"])
    {
        auto skill = std::make_shared<Skill>();
        skill->id = element["id"].get<int>();
        skill->name = element["name"].get<std::string>();
        skill->cooldown = element["cooldown"].get<float>();
        skill->damage = element["damage"].get<int>();
        skill->skillType = ToSkillType(element["skillType"].get<std::string>());

        if (element.contains("projectile"))
        {
            auto proj = std::make_shared<ProjectileInfo>();
            proj->name = element["projectile"]["name"].get<std::string>();
            proj->speed = element["projectile"]["speed"].get<float>();
            proj->range = element["projectile"]["range"].get<int>();
            proj->prefab = element["projectile"]["prefab"].get<std::string>();
            skill->projectile = proj;
        }

        data->skills.push_back(skill);
    }
    return data;
}

SkillType ToSkillType(const std::string& str)
{
    if (str == "SkillAuto") return SkillType::SKILL_AUTO;
    if (str == "SkillProjectile") return SkillType::SKILL_PROJECTILE;
    return SkillType::SKILL_NONE;
}