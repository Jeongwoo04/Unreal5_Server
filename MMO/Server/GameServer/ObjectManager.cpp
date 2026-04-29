#include "pch.h"
#include "ObjectManager.h"
#include "Projectile.h"
#include "Field.h"
#include "Player.h"
#include "Monster.h"
#include "DataManager.h"

void ObjectManager::Init()
{
	_createRegistry.clear();
	_objects.clear();

#ifdef USE_OPTIMIZED_MEMORY_POOLING
	_playerPool = make_shared<ChunkList>("Player");
	_playerPool->Init<Player>();
	_monsterPool = make_shared<ChunkList>("Monster");
	_monsterPool->Init<Monster>();
	_projectilePool = make_shared<ChunkList>("Projectile");
	_projectilePool->Init<Projectile>();
	_fieldPool = make_shared<ChunkList>("Field");
	_fieldPool->Init<Field>();

	AddFactory(FactoryHash(OBJECT_TYPE_CREATURE, CREATURE_TYPE_PLAYER),
		[this]() -> ObjectRef {
			return _playerPool->AllocShared<Player>();
		}
	);

	AddFactory(FactoryHash(OBJECT_TYPE_CREATURE, CREATURE_TYPE_MONSTER),
		[this]() -> ObjectRef {
			return _monsterPool->AllocShared<Monster>();
		}
	);

	AddFactory(FactoryHash(OBJECT_TYPE_PROJECTILE, 0),
		[this]() -> ObjectRef {
			return _projectilePool->AllocShared<Projectile>();
		}
	);

	AddFactory(FactoryHash(OBJECT_TYPE_ENV, ENV_TYPE_FIELD),
		[this]() -> ObjectRef {
			return _fieldPool->AllocShared<Field>();
		}
	);
#else
	AddFactory(FactoryHash(OBJECT_TYPE_CREATURE, CREATURE_TYPE_PLAYER),
		[this]() -> ObjectRef {
			return make_shared<Player>();
		}
	);

	AddFactory(FactoryHash(OBJECT_TYPE_CREATURE, CREATURE_TYPE_MONSTER),
		[this]() -> ObjectRef {
			return make_shared<Monster>();
		}
	);

	AddFactory(FactoryHash(OBJECT_TYPE_PROJECTILE, 0),
		[this]() -> ObjectRef {
			return make_shared<Projectile>();
		}
	);

	AddFactory(FactoryHash(OBJECT_TYPE_ENV, ENV_TYPE_FIELD),
		[this]() -> ObjectRef {
			return make_shared<Field>();
		}
	);
#endif // !1
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

ObjectRef ObjectManager::Spawn(int32 dataId, const PosInfo& posInfo)
{
	return Spawn(dataId, false, Vector3(posInfo), posInfo.yaw());
}

ObjectRef ObjectManager::Spawn(int32 dataId, bool randPos, const Vector3& pos, float yaw)
{
	auto it = DataManager::Instance().ObjectDict.find(dataId);
	if (it == DataManager::Instance().ObjectDict.end())
		return nullptr;

	auto& objTemplate = it->second;

	int32 hash = FactoryHash(objTemplate.mainType, objTemplate.subType);

	auto funcIt = _createRegistry.find(hash);
	if (funcIt == _createRegistry.end())
		return nullptr;

	auto newObject = funcIt->second();
	newObject->SetId(GenerateId(static_cast<ObjectType>(objTemplate.mainType)));

	auto statIt = DataManager::Instance().StatDict.find(objTemplate.dataId);
	if (statIt != DataManager::Instance().StatDict.end())
	{
		newObject->_statInfo.CopyFrom(statIt->second);
		newObject->_posInfo.set_speed(statIt->second.speed());
	}

	if (auto proj = dynamic_pointer_cast<Projectile>(newObject))
	{
		auto projectileIt = DataManager::Instance().ProjectileDict.find(objTemplate.dataId);
		if (projectileIt != DataManager::Instance().ProjectileDict.end())
		{
			proj->SetData(&(projectileIt->second));
			proj->_statInfo.set_speed(proj->GetData()->speed);
		}
	}

	if (auto field = dynamic_pointer_cast<Field>(newObject))
	{
		auto fieldIt = DataManager::Instance().FieldDict.find(dataId);
		if (fieldIt != DataManager::Instance().FieldDict.end())
		{
			field->SetData(&(fieldIt->second));
		}
	}

	if (randPos)
		newObject->SetSpawnRandomPos(pos, yaw);
	else
		newObject->SetSpawnPos(pos, yaw);

	return newObject;
}

void ObjectManager::Despawn(ObjectRef obj)
{
	Despawn(obj->GetId());
}

void ObjectManager::Despawn(uint64 objId)
{
	_objects.erase(objId);
}

#ifdef USE_OPTIMIZED_MEMORY_POOLING
void ObjectManager::Update()
{
	_monsterPool->Update();
	_projectilePool->Update();
	_fieldPool->Update();
}
void ObjectManager::CheckPools()
{
	pair<int32, int32> check;

	check = _playerPool->CheckList();
	cout << "Player] Chunk : " << check.first << ", Object : " << check.second<< endl;
	check = _monsterPool->CheckList();
	cout << "Monster] Chunk : " << check.first << ", Object : " << check.second << endl;
	check = _projectilePool->CheckList();
	cout << "Projectile] Chunk : " << check.first << ", Object : " << check.second << endl;
	check = _fieldPool->CheckList();
	cout << "Field] Chunk : " << check.first << ", Object : " << check.second << endl;
}
#endif
