#include "Data.h"
#include "S1DataManager.h"
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
        dict[projectile.dataId] = projectile;
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
        ProjectileInfo projectile;
        projectile.dataId = element["dataId"].get<int32>();
        projectile.name = element["name"].get<string>();
        projectile.distance = element["distance"].get<float>();
        projectile.range = element["range"].get<float>();
        // 렌더링 , 애니메이션 등등

        data.projectiles.push_back(projectile);
    }
    return data;
}

unordered_map<int32, FieldInfo> FieldData::MakeDict()
{
    unordered_map<int32, FieldInfo> dict;
    for (auto& field : fields)
    {
        dict[field.dataId] = field;
    }
    return dict;
}

FieldData FieldData::LoadFromJsonFile(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    auto data = FieldData();
    for (auto& element : j["Field"])
    {
        FieldInfo field;
        field.dataId = element["dataId"].get<int32>();
        field.name = element["name"].get<string>();
        field.distance = element["distance"].get<float>();
        field.range = element["range"].get<float>();
        // 렌더링 , 애니메이션 등등

        data.fields.push_back(field);
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
        skill.castTime = element["castTime"].get<float>();
        skill.marker = element["marker"].get<bool>();

        if (element.contains("actions"))
        {
            for (auto& act : element["actions"])
            {
                ClientAction action;
                action.actionType = ToActionType(act["actionType"].get<string>());

                if (act.contains("actionDelay"))
                    action.actionDelay = act["actionDelay"].get<float>();

                switch (action.actionType)
                {
                case ClientActionType::PlayAnimation:
                    action.animName = act["animName"].get<string>();
                    break;
                case ClientActionType::PlayEffect:
                    action.effectName = act["effectName"].get<string>();
                    action.attachBone = act["attachBone"].get<string>();
                    break;
                case ClientActionType::SpawnProjectile:
                {
                    action.dataId = act["dataId"].get<int32>();
                    auto it = S1DataManager::Instance().ProjectileDict.find(action.dataId);
                    if (it != S1DataManager::Instance().ProjectileDict.end())
                    {
                        const auto& proj = it->second;
                        //skill.markerData.shape = EMarkerShape::Line;
                        skill.markerData.distance = proj.distance;
                        skill.markerData.range = proj.range;
                    }
                }   break;
                case ClientActionType::SpawnField:
                {
                    action.dataId = act["dataId"].get<int32>();
                    auto it = S1DataManager::Instance().FieldDict.find(action.dataId);
                    if (it != S1DataManager::Instance().FieldDict.end())
                    {
                        const auto& field = it->second;
                        //skill.markerData.shape = EMarkerShape::Line;
                        skill.markerData.distance = field.distance;
                        skill.markerData.range = field.range;
                    }
                }   break;
                case ClientActionType::Move:
                    action.moveDistance = act["moveDistance"].get<float>();
                    break;
                default:
                    break;
                }

                skill.actions.push_back(action);
            }
        }

        data.skills.push_back(skill);
    }

    return data;
}

ClientActionType ToActionType(const string& str)
{
    if (str == "PlayAnimation") return ClientActionType::PlayAnimation;
    if (str == "PlayEffect") return ClientActionType::PlayEffect;
    if (str == "SpawnProjectile") return ClientActionType::SpawnProjectile;
    if (str == "SpawnField") return ClientActionType::SpawnField;
    if (str == "Move") return ClientActionType::Move;
    return ClientActionType::None;
}