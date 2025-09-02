#include "pch.h"
#include "DataManager.h"

void DataManager::LoadData(const std::string& basePath)
{
    auto objectData = ObjectData::LoadFromJson(basePath + "/ObjectData.json");
    ObjectDict = objectData.MakeDict();

    auto statData = StatData::LoadFromJson(basePath + "/StatData.json");
    StatDict = statData.MakeDict();

    auto skillData = SkillData::LoadFromJsonFile(basePath + "/SkillData.json");
    SkillDict = skillData.MakeDict();

    auto projectileData = ProjectileData::LoadFromJsonFile(basePath + "/ProjectileData.json");
    ProjectileDict = projectileData.MakeDict();

    auto MapData = MapData::LoadFromJsonFile(basePath + "/MapData.json");
    MapDataDict = MapData.MakeDict();
}