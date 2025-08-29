#include "pch.h"
#include "Data.Contents.h"
#include <fstream>

unordered_map<string, ObjectTemplate> ObjectData::MakeDict()
{
    unordered_map<string, ObjectTemplate> dict;
    for (auto& objectTemplate : objectTemplates)
    {
        dict[objectTemplate.name] = objectTemplate;
    }
    return dict;
}

ObjectData ObjectData::LoadFromJson(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    auto data = ObjectData();
    for (auto& element : j["Object"])
    {
        auto objTemplate = ObjectTemplate();
        objTemplate.name = element["name"].get<string>();
        objTemplate.mainType = element["mainType"].get<int32>();
        objTemplate.subType = element["subType"].get<int32>();
        if (element.contains("statId"))
            objTemplate.statId = element["statId"].get<int32>();
        if (element.contains("projectileId"))
            objTemplate.projectileId = element["projectileId"].get<int32>();

        data.objectTemplates.push_back(objTemplate);
    }
    return data;
}


unordered_map<int32, StatInfo> StatData::MakeDict()
{
    unordered_map<int32, StatInfo> dict;
    for (auto& stat : stats)
    {
        stat.set_hp(stat.maxhp()); // 초기 HP 세팅
        dict[stat.statid()] = stat;
    }
    return dict;
}

StatData StatData::LoadFromJson(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    auto data = StatData();
    for (auto& element : j["Stat"])
    {
        auto stat = StatInfo();
        stat.set_statid(element["statId"].get<int32>());
        stat.set_level(element["level"].get<int32>());
        stat.set_maxhp(element["maxHp"].get<int32>());
        stat.set_attack(element["attack"].get<int32>());
        stat.set_speed(element["speed"].get<float>());
        stat.set_totalexp(element["totalExp"].get<int32>());

        data.stats.push_back(stat);
    }
    return data;
}

unordered_map<int32, ProjectileInfo> ProjectileData::MakeDict()
{
    unordered_map<int32, ProjectileInfo> dict;
    for (auto& projectile : projectiles)
    {
        dict[projectile.projectileid()] = projectile;
    }
    return dict;
}

ProjectileData ProjectileData::LoadFromJsonFile(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    auto data = ProjectileData();
    for (auto& element : j["Projectile"])
    {
        auto projectile = ProjectileInfo();
        projectile.set_projectileid(element["projectileId"].get<int32>());
        projectile.set_name(element["name"].get<string>());
        projectile.set_speed(element["speed"].get<float>());
        projectile.set_range(element["range"].get<int32>());

        data.projectiles.push_back(projectile);
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
        skill.skillType = ToSkillType(element["skillType"].get<string>());
        skill.distance = element["distance"].get<float>();

        if (element.contains("projectileId"))
        {
            skill.projectileId = element["projectileId"].get<int32>();
        }

        data.skills.push_back(skill);
    }
    return data;
}

SkillType ToSkillType(const std::string& str)
{
    if (str == "SkillAuto") return SkillType::SKILL_AUTO;
    if (str == "SkillProjectile") return SkillType::SKILL_PROJECTILE;
    if (str == "SkillAoeDot") return SkillType::SKILL_AOE_DOT;
    return SkillType::SKILL_NONE;
}