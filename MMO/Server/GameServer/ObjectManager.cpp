#include "pch.h"
#include "ObjectManager.h"

int32 ObjectManager::GenerateId(ObjectType type)
{
    return (static_cast<int32>(type) << 24) | (_counter++);
}

bool ObjectManager::Remove(int32 objectId)
{
	ObjectType type = GetObjectTypeById(objectId);

	if (type != Protocol::OBJECT_TYPE_CREATURE)
		return false;

	WRITE_LOCK;
	return _players.erase(objectId) > 0;
}

PlayerRef ObjectManager::Find(int32 objectId)
{
	WRITE_LOCK;
	auto it = _players.find(objectId);
	if (it != _players.end())
		return it->second;
	return nullptr;
}
