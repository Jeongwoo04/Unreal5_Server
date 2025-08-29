#include "S1DataManager.h"
#include "Logging/LogMacros.h"

void S1DataManager::LoadData(const std::string& basePath)
{
    auto statData = StatData::LoadFromJson(basePath + "Client_StatData.json");
    StatDict = statData.MakeDict();

    auto skillData = SkillData::LoadFromJsonFile(basePath + "Client_SkillData.json");
    SkillDict = skillData.MakeDict();

    auto projectileData = ProjectileData::LoadFromJsonFile(basePath + "Client_ProjectileData.json");
    ProjectileDict = projectileData.MakeDict();
}