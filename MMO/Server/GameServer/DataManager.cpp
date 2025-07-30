#include "pch.h"
#include "DataManager.h"

void DataManager::LoadData(const std::string& basePath)
{
    auto statData = StatData::LoadFromJson(basePath + "/StatData.json");
    StatDict = statData->MakeDict();

    auto skillData = SkillData::LoadFromJsonFile(basePath + "/SkillData.json");
    SkillDict = skillData->MakeDict();
}