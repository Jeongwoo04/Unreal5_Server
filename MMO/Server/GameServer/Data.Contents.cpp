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
        objTemplate.dataId = element.value("dataId", 0);
        objTemplate.name = element.value("name", "");
        objTemplate.mainType = element.value("mainType", 0);
        objTemplate.subType = element.value("subType", 0);

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
        stat.set_dataid(element.value("dataId", 0));
        stat.set_level(element.value("level", 1));
        stat.set_maxhp(element.value("maxHp", 100));
        stat.set_attack(element.value("attack", 10));
        stat.set_speed(element.value("speed", 1.f));
        stat.set_totalexp(element.value("totalExp", 0));

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

    ProjectileData data;
    for (auto& element : j["Projectile"])
    {
        ProjectileInfo projectile;
        projectile.dataId = element["dataId"].get<int32>();
        projectile.name = element["name"].get<string>();
        projectile.shapeType = ToShapeType(element["shape"].get<string>());
        projectile.damage = element.value("damage", 0);
        projectile.speed = element.value("speed", 0.f);
        projectile.distance = element.value("distance", 0.f);
        projectile.radius = element.value("radius", 0.f);

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

    FieldData data;
    for (auto& element : j["Field"])
    {
        FieldInfo field;
        field.dataId = element["dataId"].get<int32>();
        field.name = element["name"].get<string>();
        field.shapeType = ToShapeType(element["shape"].get<string>());
        field.damagePerTick = element.value("damagePerTick", 0);
        field.duration = element.value("duration", 0.f);
        field.distance = element.value("distance", 0.f);
        field.range = element.value("range", 0.f);
        field.buffId = element.value("buffId", 0);

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
    json j;
    file >> j;

    SkillData data;
    for (auto& element : j["skills"])
    {
        Skill skill;
        skill.id = element["id"].get<int32>();
        skill.name = element["name"].get<string>();
        skill.cooldown = element["cooldown"].get<float>();
        skill.castTime = element["castTime"].get<float>();

        if (element.contains("actions"))
        {
            for (auto& actionElem : element["actions"])
            {
                ActionType type = ToActionType(actionElem["actionType"].get<string>());
                ActionData* action = nullptr;
                //float distance = actionElem.value("distance", 0.f);
                //float actionDelay = actionElem.value("actionDelay", 0.f);

                switch (type)
                {
                case ActionType::Attack:
                {
                    auto attack = new AttackActionData();
                    attack->shape = ToShapeType(actionElem["shape"].get<string>());
                    attack->damage = actionElem["damage"].get<int32>();
                    attack->radius = actionElem.value("radius", 0.f);
                    attack->width = actionElem.value("width", 0.f);
                    attack->length = actionElem.value("length", 0.f);
                    attack->angle = actionElem.value("angle", 0.f);
                    action = attack;
                    break;
                }
                case ActionType::Move:
                {
                    auto move = new MoveActionData();
                    move->actionType = ActionType::Move;
                    move->moveDistance = actionElem["moveDistance"].get<float>();
                    action = move;
                    break;
                }
                case ActionType::SpawnProjectile:
                case ActionType::SpawnField:
                {
                    // ActionData가 struct라 다형성 적용이 안됨
                    auto spawn = new SpawnActionData(type);
                    spawn->dataId = actionElem["dataId"].get<int32>();
                    action = spawn;
                    break;
                }
                case ActionType::Buff:
                {
                    auto buff = new BuffActionData();
                    buff->buffId = actionElem["buffId"].get<int32>();
                    action = buff;
                    break;
                }
                default:
                    break;
                }
                if (action != nullptr)
                {
                    action->distance = actionElem.value("distance", 0.f);
                    action->actionDelay = actionElem.value("actionDelay", 0.f);
                    skill.actions.push_back(action);
                }
            }
        }

        data.skills.push_back(skill);
    }
    return data;
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
        map.mapId = element.value("mapId", 0);
        map.mapName = element.value("mapName", "");
        map.filePath = element.value("filePath", "");

        if (element.contains("spawns"))
        {
            for (auto& spawnElement : element["spawns"])
            {
                SpawnTable spawn;
                spawn.spawnId = spawnElement.value("spawnId", 0);
                spawn.dataId = spawnElement.value("dataId", 0);

                auto pos = spawnElement.value("position", json::object());
                spawn.spawnPos._x = pos.value("x", 0.f);
                spawn.spawnPos._y = pos.value("y", 0.f);
                spawn.spawnPos._z = pos.value("z", 0.f);

                spawn.respawnInterval = spawnElement.value("respawnInterval", 0);
                spawn.count = spawnElement.value("count", 0);

                map.spawnTables[spawn.spawnId] = spawn;
            }
        }        

        data.maps.push_back(map);
    }

    return data;
}

unordered_map<int32, BuffInfo> BuffData::MakeDict()
{
    unordered_map<int32, BuffInfo> dict;
    for (auto& buff : buffs)
    {
        dict[buff.buffId] = buff;
    }

    return dict;
}

BuffData BuffData::LoadFromJsonFile(const string& path)
{
    std::ifstream file(path);
    json j;
    file >> j;

    BuffData data;
    for (auto& element : j["buffs"])
    {
        BuffInfo buff;
        buff.buffId = element.value("buffId", 0);
        buff.name = element.value("name", "");
        buff.duration = element.value("duration", 0.f);
        buff.tickInterval = element.value("tickInterval", 0.f);
        buff.effectPerTick = element.value("effectPerTick", 0.f);
        buff.effectValue = element.value("effectValue", 0.f);
        buff.stackable = element.value("stackable", false);
        buff.maxStacks = element.value("maxStacks", 1);
        buff.refreshOnApply = element.value("refreshOnApply", true);
        buff.effectType = ToEffectType(element.value("effectType", "None"));
        buff.movementModifier = element.value("movementModifier", 1.f);
        buff.attackModifier = element.value("attackModifier", 1.f);
        buff.isCrowdControl = element.value("isCrowdControl", false);
        buff.root = element.value("root", false);
        buff.stun = element.value("stun", false);
        buff.silence = element.value("silence", false);
        buff.priority = element.value("priority", 0);

        data.buffs.push_back(buff);
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

ActionType ToActionType(const string& str)
{
    if (str == "Move") return ActionType::Move;
    if (str == "Attack") return ActionType::Attack;
    if (str == "SpawnProjectile") return ActionType::SpawnProjectile;
    if (str == "SpawnField") return ActionType::SpawnField;
    if (str == "Buff") return ActionType::Buff;
    return ActionType::None;
}

ShapeType ToShapeType(const string& str)
{
    if (str == "Circle") return ShapeType::Circle;
    if (str == "Cone") return ShapeType::Cone;
    if (str == "Rectangle") return ShapeType::Rectangle;
    if (str == "Line") return ShapeType::Line;
    return ShapeType::None;
}

EffectType ToEffectType(const string& str)
{
    if (str == "HP") return EffectType::HP;
    if (str == "MP") return EffectType::MP;
    if (str == "Attack") return EffectType::Attack;
    if (str == "Defense") return EffectType::Defense;
    if (str == "Speed") return EffectType::Speed;
    return EffectType::None;
}