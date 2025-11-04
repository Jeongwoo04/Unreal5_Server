#include "pch.h"
#include "Field.h"
#include "Monster.h"
#include "BuffSystem.h"

Field::Field()
{
	_objectInfo.set_object_type(OBJECT_TYPE_ENV);
}

Field::~Field()
{
	//cout << "Field " << GetId() << " Destructor" << endl;
}

void Field::GridCaching()
{
	auto room = GetRoom();
	if (room == nullptr)
		return;

	if (GetOwner()->GetCreatureType() == CREATURE_TYPE_MONSTER)
	{
		_cachedGrids = room->_playerGrid.FindGridAroundFloat(WorldToGrid(_worldPos), _data->range);
	}
	else if (GetOwner()->GetCreatureType() == CREATURE_TYPE_PLAYER)
	{
		_cachedGrids = room->_monsterGrid.FindGridAroundFloat(WorldToGrid(_worldPos), _data->range);
	}
}

void Field::Update(float deltaTime)
{
	if (GetOwner() == nullptr || GetOwner()->GetRoom() == nullptr)
		return;

	if (_cachedGrids.empty()) // °íÁ¤Çü
		GridCaching();

	_affectedTargets.clear();
	auto room = GetRoom();
	if (room == nullptr)
		return;

	_elapsedTime += deltaTime;
	if (_elapsedTime >= _data->duration)
	{
		room->AddRemoveList(shared_from_this());
		return;
	}

	Protocol::HpChange change;
	Protocol::S_CHANGE_HP pkt;

	for (const auto& grid : _cachedGrids)
	{
		if (GetOwner()->GetCreatureType() == CREATURE_TYPE_MONSTER)
		{
			auto targets = room->_playerGrid.Find(grid);
			if (targets.empty())
				continue;

			for (auto target : targets)
			{
				if (target == nullptr || target->IsDead())
					continue;
				if (_affectedTargets.find(target) != _affectedTargets.end())
					continue;
				if ((target->_worldPos - _worldPos).LengthSquared2D() > (_data->range * _data->range))
					continue;

				target->OnDamaged(GetOwner(), _data->damagePerTick);

				change.set_object_id(target->GetId());
				change.set_hp(target->_statInfo.hp());
				*pkt.add_changes() = change;

				if (_data->buffId > 0)
					BuffSystem::Instance().ApplyBuff(target, _data->buffId);

				_affectedTargets.insert(target);
			}
		}			
		else if (GetOwner()->GetCreatureType() == CREATURE_TYPE_PLAYER)
		{
			auto targets = room->_monsterGrid.Find(grid);

			for (auto target : targets)
			{
				if (target == nullptr || target->IsDead())
					continue;
				if (_affectedTargets.find(target) != _affectedTargets.end())
					continue;
				if ((target->_worldPos - _worldPos).LengthSquared2D() > (_data->range * _data->range))
					continue;

				target->OnDamaged(GetOwner(), _data->damagePerTick);

				change.set_object_id(target->GetId());
				change.set_hp(target->_statInfo.hp());
				*pkt.add_changes() = change;

				if (_data->buffId > 0)
					BuffSystem::Instance().ApplyBuff(target, _data->buffId);

				_affectedTargets.insert(target);
			}
		}
	}

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	room->BroadcastNearby(sendBuffer, _worldPos, GetId());
}