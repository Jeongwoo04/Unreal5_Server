#include "pch.h"
#include "Data.Contents.h"
#include <fstream>

unordered_map<int32, StatInfo> StatData::MakeDict()
{
    unordered_map<int32, StatInfo> dict;
    for (auto& stat : stats)
    {
        stat.set_hp(stat.maxhp()); // 초기 HP 세팅
        dict[stat.level()] = stat;
    }
    return dict;
}

StatData StatData::LoadFromJson(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    auto data = StatData();
    for (auto& element : j["stats"])
    {
        auto stat = StatInfo();
        stat.set_level(element["level"].get<int32>());
        stat.set_maxhp(element["maxHp"].get<int32>());
        stat.set_attack(element["attack"].get<int32>());
        stat.set_speed(element["speed"].get<float>());
        stat.set_totalexp(element["totalExp"].get<int32>());

        data.stats.push_back(stat);
    }
    return data;
}

unordered_map<int32, Skill> SkillData::MakeDict()
{
    unordered_map<int32, Skill> dict;
    for (auto& skill : skills)
    {
        dict[skill.id] = skill;
    }
    return dict;
}

SkillData SkillData::LoadFromJsonFile(const std::string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    auto data = SkillData();
    for (auto& element : j["skills"])
    {
        auto skill = Skill();
        skill.id = element["id"].get<int32>();
        skill.name = element["name"].get<std::string>();
        skill.cooldown = element["cooldown"].get<float>();
        skill.damage = element["damage"].get<int32>();
        skill.skillType = ToSkillType(element["skillType"].get<std::string>());

        if (element.contains("projectile"))
        {
            auto proj = ProjectileInfo();
            proj.name = element["projectile"]["name"].get<std::string>();
            proj.speed = element["projectile"]["speed"].get<float>();
            proj.range = element["projectile"]["range"].get<int32>();
            skill.projectile = proj;
        }

        data.skills.push_back(skill);
    }
    return data;
}

SkillType ToSkillType(const std::string& str)
{
    if (str == "SkillAuto") return SkillType::SKILL_AUTO;
    if (str == "SkillProjectile") return SkillType::SKILL_PROJECTILE;
    return SkillType::SKILL_NONE;
}