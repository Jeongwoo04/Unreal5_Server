#include "Data.h"
#include <fstream>
#include <Logging/MessageLog.h>

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
    if (!file.is_open())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open StatData file: %s"), *FString(path.c_str()));
        return StatData();
    }
    json j;
    file >> j;

    auto data = StatData();
    for (auto& element : j["stats"])
    {
        auto stat = StatInfo();
        stat.set_level(element["level"].get<int32>());
        stat.set_maxhp(element["maxHp"].get<int32>());
        stat.set_speed(element["speed"].get<float>());

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
        // 렌더링 , 애니메이션 등등

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

SkillData SkillData::LoadFromJsonFile(const string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open SkillData file: %s"), *FString(path.c_str()));
        return SkillData();
    }
    json j;
    file >> j;

    auto data = SkillData();
    for (auto& element : j["skills"])
    {
        auto skill = Skill();
        skill.id = element["id"].get<int32>();
        skill.name = element["name"].get<string>();
        skill.iconPath = element["iconPath"].get<string>();
        skill.cooldown = element["cooldown"].get<float>();
        skill.skillType = ToSkillType(element["skillType"].get<string>());

        if (element.contains("projectileId"))
        {
            skill.projectileId = element["projectileId"].get<int32>();
        }

        data.skills.push_back(skill);
    }
    return data;
}

SkillType ToSkillType(const string& str)
{
    if (str == "SkillAuto") return SkillType::SKILL_AUTO;
    if (str == "SkillProjectile") return SkillType::SKILL_PROJECTILE;
    if (str == "SkillAoeDot") return SkillType::SKILL_AOE_DOT;
    return SkillType::SKILL_NONE;
}