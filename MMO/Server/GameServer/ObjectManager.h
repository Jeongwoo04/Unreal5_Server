#pragma once
#include "Object.h"
#include "Player.h"
#include "Projectile.h"
#include "Monster.h"

using namespace Protocol;

class ObjectManager
{
public:
	static ObjectManager& Instance()
	{
		static ObjectManager instance;
		return instance;
	}

	static ObjectType GetObjectTypeById(int32 id)
	{
		int32 type = (id >> 24) & 0x7F;
		return static_cast<ObjectType>(type);
	}

	int32 GenerateId(ObjectType type);

	template<typename T>
	shared_ptr<T> Add()
	{
		shared_ptr<T> gameObject = make_shared<T>();
		{
			WRITE_LOCK;
			
			gameObject->SetId(GenerateId(gameObject->_objectInfo.object_type()));

			if constexpr (std::is_same_v<T, Player>)
			{
				_players[gameObject->_objectInfo.object_id()] = static_pointer_cast<Player>(gameObject);
			}
		}

		return gameObject;
	}

	bool Remove(int32 objectId);
	PlayerRef Find(int32 objectId);

private:
	ObjectManager() = default;

private:
	USE_LOCK;
	unordered_map<uint64, PlayerRef> _players;
	int32 _counter = 0;
};