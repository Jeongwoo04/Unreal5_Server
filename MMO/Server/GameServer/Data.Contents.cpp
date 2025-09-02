#include "pch.h"
#include "Data.Contents.h"
#include <fstream>

unordered_map<int32, ObjectTemplate> ObjectData::MakeDict()
{
    unordered_map<int32, ObjectTemplate> dict;
    for (auto& objectTemplate : objectTemplates)
    {
        dict[objectTemplate.dataId] = objectTemplate;
    }
    return dict;
}

ObjectData ObjectData::LoadFromJson(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    ObjectData data;
    for (auto& element : j["Object"])
    {
        ObjectTemplate objTemplate;
        objTemplate.dataId = element["dataId"].get<int32>();
        objTemplate.name = element["name"].get<string>();
        objTemplate.mainType = element["mainType"].get<int32>();
        objTemplate.subType = element["subType"].get<int32>();

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
        dict[stat.dataid()] = stat;
    }
    return dict;
}

StatData StatData::LoadFromJson(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    StatData data;
    for (auto& element : j["Stat"])
    {
        StatInfo stat;
        stat.set_dataid(element["dataId"].get<int32>());
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
        dict[projectile.dataid()] = projectile;
    }
    return dict;
}

ProjectileData ProjectileData::LoadFromJsonFile(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    ProjectileData data;
    for (auto& element : j["Projectile"])
    {
        ProjectileInfo projectile;
        projectile.set_dataid(element["dataId"].get<int32>());
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

    SkillData data;
    for (auto& element : j["skills"])
    {
        Skill skill;
        skill.id = element["id"].get<int32>();
        skill.name = element["name"].get<std::string>();
        skill.cooldown = element["cooldown"].get<float>();
        skill.damage = element["damage"].get<int32>();
        skill.skillType = ToSkillType(element["skillType"].get<string>());
        skill.distance = element["distance"].get<float>();

        if (element.contains("dataId"))
        {
            skill.dataId = element["dataId"].get<int32>();
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

unordered_map<int32, MapInfo> MapData::MakeDict()
{
    unordered_map<int32, MapInfo> dict;
    for (auto& map : maps)
    {
        dict[map.mapId] = map;
    }

    return dict;
}

MapData MapData::LoadFromJsonFile(const string& path)
{
    ifstream file(path);
    json j;
    file >> j;

    MapData data;
    for (auto& element : j["Map"])
    {
        MapInfo map;
        map.mapId = element["mapId"].get<int32>();
        map.mapName = element["mapName"].get<string>();
        map.filePath = element["filePath"].get<string>();

        for (auto& spawnElement : element["spawns"])
        {
            SpawnTable spawn;
            spawn.spawnId = spawnElement["spawnId"].get<int32>();
            spawn.dataId = spawnElement["dataId"].get<int32>();

            spawn.spawnPos._x = spawnElement["position"]["x"].get<float>();
            spawn.spawnPos._y = spawnElement["position"]["y"].get<float>();
            spawn.spawnPos._z = spawnElement["position"]["z"].get<float>();

            spawn.respawnInterval = spawnElement["respawnInterval"].get<int32>();
            spawn.count = spawnElement["count"].get<int32>();

            map.spawnTables[spawn.spawnId] = spawn;
        }

        data.maps.push_back(map);
    }

    return data;
}
