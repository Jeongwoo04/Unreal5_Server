#include "pch.h"
#include "ObjectManager.h"
#include "Object.h"
#include "Projectile.h"
#include "Player.h"
#include "Monster.h"
#include "DataManager.h"

void ObjectManager::Init()
{
    _createRegistry.clear();
    _objects.clear();

	AddFactory(FactoryHash(OBJECT_TYPE_CREATURE, CREATURE_TYPE_PLAYER),
        []() -> ObjectRef { return make_shared<Player>(); }
	);

    AddFactory(FactoryHash(OBJECT_TYPE_CREATURE, CREATURE_TYPE_MONSTER),
        []() -> ObjectRef { return make_shared<Monster>(); }
    );

    AddFactory(FactoryHash(OBJECT_TYPE_PROJECTILE, 0),
        []() -> ObjectRef { return make_shared<Projectile>(); }
    );
}

int32 ObjectManager::GenerateId(ObjectType type)
{
    return (static_cast<int32>(type) << 24) | (_counter++);
}

int32 ObjectManager::FactoryHash(int32 mainType, int32 subType)
{
	return (mainType << 16) | (subType & 0xFFFF);
}

void ObjectManager::AddFactory(int32 hash, createFunc func)
{
    _createRegistry[hash] = func;
}

ObjectRef ObjectManager::Spawn(const string& templateName)
{
    auto it = DataManager::Instance().ObjectDict.find(templateName);

    if (it == DataManager::Instance().ObjectDict.end())
        return nullptr;

    auto objTemplate = it->second;

    int32 hash = FactoryHash(objTemplate.mainType, objTemplate.subType);

    auto funcIt = _createRegistry.find(hash);
    if (funcIt == _createRegistry.end())
        return nullptr;

    auto newObject = funcIt->second();
    newObject->SetId(GenerateId(static_cast<ObjectType>(objTemplate.mainType)));

    if (objTemplate.statId != -1)
    {
        auto statIt = DataManager::Instance().StatDict.find(objTemplate.statId);
        if (statIt != DataManager::Instance().StatDict.end())
        {
            newObject->_statInfo.CopyFrom(statIt->second);
        }
    }
    if (objTemplate.projectileId != -1)
    {
        if (auto proj = dynamic_pointer_cast<Projectile>(newObject))
        {
            auto projectileIt = DataManager::Instance().ProjectileDict.find(objTemplate.projectileId);
            if (projectileIt != DataManager::Instance().ProjectileDict.end())
            {
                proj->_projectileInfo = projectileIt->second;
            }
        }        
    }

    return newObject;
}