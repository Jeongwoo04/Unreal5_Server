#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Monster.h"
#include "Projectile.h"
#include "Field.h"
#include "ObjectManager.h"
#include "SkillSystem.h"

using namespace std::chrono;

Room::Room(string name) : JobQueue(name)
{
	_broadcastQueue = make_shared<JobQueue>("BC_" + name);

	_gameMap = make_shared<GameMap>();
	_objectManager = make_shared<ObjectManager>();
	_skillSystem = make_shared<SkillSystem>();
	_objectManager->Init();
	_skillSystem->Init();
}

Room::~Room()
{

}

void Room::Init(int32 mapId)
{
	auto mapIt = DataManager::Instance().MapDataDict.find(mapId);
	if (mapIt == DataManager::Instance().MapDataDict.end())
		return;

	_mapInfo = &mapIt->second;
	_gameMap->LoadGameMap(_mapInfo->filePath);

	_playerGrid = SpatialGrid<PlayerRef>(_gameMap);
	_monsterGrid = SpatialGrid<MonsterRef>(_gameMap);
	//_flushBCQueue.resize(_gameMap->_sizeX * _gameMap->_sizeY);

	//InitBaseOffsets();

	_skillSystem->SetRoom(static_pointer_cast<Room>(shared_from_this()));

	SpawnInit();

	UpdateTick();
	//StartHeartbeat();
}

//void Room::InitBaseOffsets()
//{
//	_baseOffsets.clear();
//	for (int32 y = -BROADCAST_RANGE; y <= BROADCAST_RANGE; ++y)
//	{
//		for (int32 x = -BROADCAST_RANGE; x <= BROADCAST_RANGE; ++x)
//		{
//			if (x * x + y * y <= BROADCAST_RANGE * BROADCAST_RANGE)
//			{
//				_baseOffsets.push_back(Vector2Int(x, y));
//			}
//		}
//	}
//}

void Room::UpdateTick()
{
	using namespace std::chrono;

	auto now = high_resolution_clock::now();
	auto diff = duration_cast<milliseconds>(now - _prevTime).count();
	_prevTime = now;

	if (diff > 0)
		std::cout << "[RoomTick] Interval: " << diff << " ms\n";

	constexpr float deltaTime = 0.1f;
	uint64 tick = static_cast<uint64>(deltaTime * 1000);
	DoTimer(tick, &Room::UpdateTick);

	_bench.Begin("Room");
	_bench.Begin("FlushImmediateBroadcast");
	FlushImmediateBroadcast();
	_bench.End("FlushImmediateBroadcast");

	UpdateMonster();
	UpdateProjectile();
	UpdateField();
	UpdateSkillSystem();
	ClearRemoveList();

	_bench.Begin("FlushDeferBroadcast");
	FlushDeferBroadcast();
	_bench.End("FlushDeferBroadcast");

	_bench.End("Room");

	//_bench.PrintAndSaveSummary(GetRoomId(), "1000 active dummy Hybrid Flush Server");
}

void Room::UpdateMonster()
{
	for (auto& m : _monsters)
		m.second->Update();
}

void Room::UpdateProjectile()
{
	for (auto& p : _projectiles)
		p.second->Update();
}

void Room::UpdateField()
{
	for (auto& f : _fields)
		f.second->Update();
}

void Room::UpdateSkillSystem()
{
	_skillSystem->Update();
}

void Room::StartHeartbeat()
{
	constexpr uint64 Tick = 10000;
	DoTimer(Tick, &Room::CheckHeartbeat);
}

void Room::CheckHeartbeat()
{
	//S_HEARTBEAT pkt;

	//pkt.set_servertime(GetTickCount64());
	//auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	//Broadcast(sendBuffer);

	//constexpr uint64 Tick = 10000;
	//DoTimer(Tick, &Room::CheckHeartbeat);
}

// TEMP: Command Spawn
void Room::Spawn(int32 dataId, bool randPos, Vector3 pos, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		auto obj = _objectManager->Spawn(dataId, true, pos);
		if (!obj)
			return;

		EnterRoom(obj);
	}
}

void Room::Kill()
{	
	Vector2Int centerInt = { 0,0 };
	Vector3 centerFloat = { 0,0,100 };
	auto monster = _monsterGrid.FindNearest(centerInt, 40.f, centerFloat);
	AddRemoveList(monster);
}

void Room::KillAll()
{
	for (auto& [id, monster] : _monsters)
	{
		AddRemoveList(monster);
	}
}

void Room::GetList()
{
	for (auto& [id, monster] : _monsters)
	{
		cout << "Monster ID: " << id << endl;
	}	
}

void Room::SpawnInit()
{
	for (auto& spawnIt : _mapInfo->spawnTables)
	{
		for (int32 i = 0; i < spawnIt.second.count; i++)
		{
			SpawnMonster(spawnIt.second.spawnId);
		}
	}
}

void Room::SpawnMonster(int32 spTableId)
{
	auto spTableData = _mapInfo->spawnTables.find(spTableId)->second;

	auto obj = _objectManager->Spawn(spTableData.dataId, true, spTableData.spawnPos);
	if (!obj)
		return;

	obj->_spTableId = spTableId;

	EnterRoom(obj);
}

void Room::SpawnProjectile(ObjectRef owner, int32 dataId, const Vector3& pos, const Vector3& dir)
{
	auto obj = _objectManager->Spawn(dataId, false, pos);
	if (!obj)
		return;
	else
	{
		auto pro = static_pointer_cast<Projectile>(obj);
		pro->SetOwner(owner);
		pro->SetDir(dir);
	}

	EnterRoom(obj);
}

void Room::SpawnField(ObjectRef owner, int32 dataId, const Vector3& pos)
{
	// TODO : EnterRoom switch문 추가
	auto obj = _objectManager->Spawn(dataId, false, pos);
	if (!obj)
		return;
	else
	{
		auto field = static_pointer_cast<Field>(obj);
		field->SetOwner(owner);
	}

	EnterRoom(obj);
}

bool Room::EnterRoom(ObjectRef object)
{
	if (object == nullptr)
		return false;

	bool success = AddObject(object);
	//object->_interestCell = InterestCells(object->_gridPos);

	NotifySpawn(object, success);

	return success;
}

bool Room::LeaveRoom(ObjectRef object)
{
	if (object == nullptr)
		return false;

	const uint64 objectId = object->_objectInfo.object_id();

	bool success = RemoveObject(object, objectId);

	return success;
}

bool Room::LeaveGame(ObjectRef object, uint64 objectId)
{
	if (object == nullptr)
		return false;

	bool removed = LeaveRoom(object);

	if (removed)
	{
		// 성공적으로 Room에서 빠졌으니 클라에 알림
		Protocol::S_DESPAWN pkt;
		pkt.add_object_ids(object->GetId());
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		Broadcast(sendBuffer);
	}

	// 나에게 퇴장 패킷 보내기
	if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(object);

		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->GetSession())
			session->Send(sendBuffer);
	}

	return true;
}

bool Room::HandleEnterPlayer(GameSessionRef gameSession)
{
	//printf("[Server] Handle Enter Player\n");
	PlayerRef player = static_pointer_cast<Player>(_objectManager->Spawn(10101, true, {0, 0, 0}));
	if (player == nullptr)
		return false;

	gameSession->_player = player;
	player->SetSession(gameSession);

	return EnterRoom(player);
}

bool Room::HandleLeavePlayer(PlayerRef player)
{	
	return LeaveGame(static_pointer_cast<Object>(player), player->GetId());
}

void Room::HandleMovePlayer(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
	if (_players.find(objectId) == _players.end())
		return;

	PlayerRef player = _players[objectId];
	if (player == nullptr || player->GetRoom() == nullptr || player->GetRoom()->GetGameMap() == nullptr)
		return;

	if (player->GetState() == Protocol::STATE_MACHINE_CASTING)
	{
		auto activeSkill = player->GetActiveSkill();
		if (activeSkill)
		{
			_skillSystem->CancelCasting(player, activeSkill->castId);
			cout << "[Server] CancelCasting\n";
		}
	}
	else if (player->GetState() == Protocol::STATE_MACHINE_SKILL)
	{
		cout << "[Server] Move Fail: Player state is Skill\n";
		return;
	}

	Vector3 destPos = Vector3(pkt.info());

	player->_posInfo.set_state(pkt.info().state());
	player->MoveToNextPos(destPos);
	//BroadcastMove(player->_posInfo);
}

void Room::HandleSkill(PlayerRef player, Protocol::C_SKILL pkt)
{
	if (player == nullptr)
		return;

	int32 skillId = pkt.skillid();

	auto it = DataManager::Instance().SkillDict.find(skillId);
	if (it == DataManager::Instance().SkillDict.end())
		return;

	const Skill& skillData = it->second;
	uint64 now = ::GetTickCount64();

	if (player->GetState() == Protocol::STATE_MACHINE_CASTING
		|| player->GetState() == Protocol::STATE_MACHINE_SKILL)
	{
		auto activeSkill = player->GetActiveSkill();
		if (activeSkill && pkt.castid() != activeSkill->castId)
		{
			_skillSystem->CancelCasting(player, activeSkill->castId);
		}
	}

	// 1. 사용 가능 여부 체크 (쿨타임, 캐스팅, 자원)
	if (!player->CanUseSkill(skillId, now))
	{
		return;
	}

	player->_posInfo.set_yaw(Vector3::DirToYaw2D(Vector3(pkt.targetpos()) - player->_worldPos));
	_skillSystem->ExecuteSkill(player, skillId, Vector3(pkt.targetpos()), pkt.castid(), pkt.clientsend());
}

const SpawnTable* Room::GetSpawnTable(int32 spawnId) const
{
	auto it = _mapInfo->spawnTables.find(spawnId);
	if (it != _mapInfo->spawnTables.end())
		return &it->second;
	return nullptr;
}

bool Room::AddObject(ObjectRef object)
{
	if (object == nullptr)
		return false;

	object->SetRoom(static_pointer_cast<Room>(shared_from_this()));

	switch (object->GetObjectType())
	{
	case OBJECT_TYPE_CREATURE:
		switch (object->GetCreatureType())
		{
		case CREATURE_TYPE_PLAYER:
		{
			auto player = static_pointer_cast<Player>(object);
			_players[player->GetId()] = player;
			_playerGrid.ApplyAdd(player, player->_gridPos);
		}	break;
		case CREATURE_TYPE_MONSTER:
		{
			auto monster = static_pointer_cast<Monster>(object);
			_monsters[monster->GetId()] = monster;
			_monsterGrid.ApplyAdd(monster, monster->_gridPos);
		}	break;
		default:
			break;
		}
		break;
	case OBJECT_TYPE_PROJECTILE:
	{
		auto proj = static_pointer_cast<Projectile>(object);
		_projectiles[proj->GetId()] = proj;
	}	break;
	case OBJECT_TYPE_ENV:
	{
		auto field = static_pointer_cast<Field>(object);
		_fields[field->GetId()] = field;
	} break;
	default:
		break;
	}

	return true;
}

bool Room::RemoveObject(ObjectRef object, uint64 objectId)
{
	if (object == nullptr)
		return false;

	int32 eraseCount = 0;

	switch (object->GetObjectType())
	{
	case OBJECT_TYPE_CREATURE:
	{
		switch (object->GetCreatureType())
		{
		case CREATURE_TYPE_PLAYER:
		{
			eraseCount = static_cast<int32>(_players.erase(objectId));
			auto player = static_pointer_cast<Player>(object);
			_playerGrid.ApplyRemove(player, player->_gridPos);
		} break;
		case CREATURE_TYPE_MONSTER:
		{
			eraseCount = static_cast<int32>(_monsters.erase(objectId));
			auto monster = static_pointer_cast<Monster>(object);
			_monsterGrid.ApplyRemove(monster, monster->_gridPos);
		} break;
		default:
			break;
		}
	} break;
	case OBJECT_TYPE_PROJECTILE:
		eraseCount = static_cast<int32>(_projectiles.erase(objectId));
		break;
	case OBJECT_TYPE_ENV:
		eraseCount = static_cast<int32>(_fields.erase(objectId));
		break;
	}

	_objectManager->Despawn(object);
	object->SetRoom(nullptr);

	return eraseCount > 0 ? true : false;
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	vector<PlayerRef> snapshot;
	snapshot.reserve(_players.size());

	for (auto& [id, player] : _players)
	{
		if (player->_objectInfo.object_id() == exceptId)
			continue;

		snapshot.push_back(player);
	}

	auto enqueueTime = GetTimeMs();
	_broadcastQueue->Push(make_shared<Job>(
		[snapshot = std::move(snapshot), sendBuffer, enqueueTime, this]()
		{
			double executeTime = GetTimeMs();
			double delayMs = static_cast<double>(executeTime - enqueueTime);

			_bench.AddBCQDelay(delayMs);
			_bench.AddSendCount(int32(snapshot.size()));

			for (auto& player : snapshot)
			{
				if (auto session = player->GetSession())
					session->Send(sendBuffer);
			}
		}));
}

void Room::BroadcastNearby(SendBufferRef sendBuffer, const Vector3& center, uint64 exceptId)
{
	_bench.Begin("BCNearbySnapshot");
	vector<PlayerRef> nearbyPlayers =
		_playerGrid.FindAround(WorldToGrid(center), BROADCAST_RANGE);
	_bench.End("BCNearbySnapshot");
	auto enqueueTime = GetTickCount64();
	_broadcastQueue->Push(make_shared<Job>([nearbyPlayers = std::move(nearbyPlayers), sendBuffer, exceptId, enqueueTime, this]()
		{
			uint64 executeTime = GetTickCount64();
			double delayMs = static_cast<double>(executeTime - enqueueTime);

			_bench.AddBCQDelay(delayMs);
			_bench.AddSendCount(int32(nearbyPlayers.size()));

			//auto preExeTime = GetTickCount64();
			for (auto& player : nearbyPlayers)
			{
				if (player->GetId() == exceptId)
					continue;
				if (auto session = player->GetSession())
					session->Send(sendBuffer);
			}
			//double exeTime = static_cast<double>(GetTickCount64() - preExeTime);
			//_bench.AddExecuteTime(exeTime);
		}));
}

void Room::BroadcastMove(const Protocol::PosInfo& posInfo, uint64 exceptId)
{
	Protocol::S_MOVE movePkt;
	*movePkt.add_info() = posInfo;

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);

	BroadcastNearby(sendBuffer, Vector3(posInfo), exceptId);
}

/*
InterestDiff Room::UpdateInterestCell(const Vector2Int& oldCenter, const Vector2Int& newCenter)
{
	Vector2Int delta = newCenter - oldCenter;
	InterestDiff diff;

	int32 range = BROADCAST_RANGE;

	// delta가 너무 크면 풀 비교 (드물게 발생)
	if (abs(delta._x) > 1 || abs(delta._y) > 1)
	{
		vector<Vector2Int> oldCells = InterestCells(oldCenter);
		vector<Vector2Int> newCells = InterestCells(newCenter);

		// DiffInterestCells를 호출
		InterestDiff fullDiff = DiffInterestCells(oldCells, newCells, delta);

		diff.despawns = std::move(fullDiff.despawns);
		diff.spawns = std::move(fullDiff.spawns);
		diff.moves = std::move(fullDiff.moves);

		return diff;
	}

	// 전체 old 영역에서 despawn/spawn을 제외한 나머지는 move
	// => move 영역 = oldInterest, newInterest 교집합

	// 수평 이동
	if (delta._x == 1)
	{
		// 오른쪽 이동 → 왼쪽줄 despawn, 오른쪽줄 spawn
		for (int y = -range; y <= range; y++)
		{
			Vector2Int left = { oldCenter._x - range, oldCenter._y + y };
			Vector2Int right = { newCenter._x + range, newCenter._y + y };

			if (_playerGrid.IsValid(left))
				diff.despawns.push_back(left);

			if (_playerGrid.IsValid(right))
				diff.spawns.push_back(right);
		}

		// 겹치는 영역 (중앙 블록)은 move 처리
		for (int x = -range + 1; x <= range; x++)
		{
			for (int y = -range; y <= range; y++)
			{
				Vector2Int pos = { newCenter._x + x, newCenter._y + y };

				if (_playerGrid.IsValid(pos))
					diff.moves.push_back(pos);
			}
		}
	}
	else if (delta._x == -1)
	{
		// 왼쪽 이동 → 오른쪽줄 despawn, 왼쪽줄 spawn
		for (int y = -range; y <= range; y++)
		{
			Vector2Int right = { oldCenter._x + range, oldCenter._y + y };
			Vector2Int left = { newCenter._x - range, newCenter._y + y };

			if (_playerGrid.IsValid(right))
				diff.despawns.push_back(right);

			if (_playerGrid.IsValid(left))
				diff.spawns.push_back(left);
		}

		for (int x = -range; x <= range - 1; x++)
		{
			for (int y = -range; y <= range; y++)
			{
				Vector2Int pos = { newCenter._x + x, newCenter._y + y };

				if (_playerGrid.IsValid(pos))
					diff.moves.push_back(pos);
			}
		}
	}

	// 수직 이동
	if (delta._y == 1)
	{
		// 위로 이동 → 아래줄 despawn, 위줄 spawn
		for (int x = -range; x <= range; x++)
		{
			Vector2Int bottom = { oldCenter._x + x, oldCenter._y - range };
			Vector2Int top = { newCenter._x + x, newCenter._y + range };

			if (_playerGrid.IsValid(bottom))
				diff.despawns.push_back(bottom);

			if (_playerGrid.IsValid(top))
				diff.spawns.push_back(top);
		}

		// move
		for (int x = -range; x <= range; x++)
		{
			for (int y = -range + 1; y <= range; y++)
			{
				Vector2Int pos = { newCenter._x + x, newCenter._y + y };

				if (_playerGrid.IsValid(pos))
					diff.moves.push_back(pos);
			}
		}
	}
	else if (delta._y == -1)
	{
		// 아래로 이동 → 위줄 despawn, 아래줄 spawn
		for (int x = -range; x <= range; x++)
		{
			Vector2Int top = { oldCenter._x + x, oldCenter._y + range };
			Vector2Int bottom = { newCenter._x + x, newCenter._y - range };

			if (_playerGrid.IsValid(top))
				diff.despawns.push_back(top);

			if (_playerGrid.IsValid(bottom))
				diff.spawns.push_back(bottom);
		}

		for (int x = -range; x <= range; x++)
		{
			for (int y = -range; y <= range - 1; y++)
			{
				Vector2Int pos = { newCenter._x + x, newCenter._y + y };

				if (_playerGrid.IsValid(pos))
					diff.moves.push_back(pos);
			}
		}
	}

	return diff;
}

vector<Vector2Int> Room::InterestCells(const Vector2Int& center) const
{
	vector<Vector2Int> res;
	res.reserve(_baseOffsets.size());

	for (auto& offset : _baseOffsets)
	{
		Vector2Int cell = center + offset;
		if (_playerGrid.IsValid(cell))
			res.push_back(cell);
	}

	return res;
}

InterestDiff Room::DiffInterestCells(const vector<Vector2Int>& oldCell, const vector<Vector2Int>& newCell, const Vector2Int& delta) const
{
	InterestDiff diff;

	auto compare = [](const Vector2Int& a, const Vector2Int& b)
		{
			if (a._x != b._x)
				return a._x < b._x;

			return a._y < b._y;
		};

	vector<Vector2Int> sortOld = oldCell;
	vector<Vector2Int> sortNew = newCell;

	std::sort(sortOld.begin(), sortOld.end(), compare);
	std::sort(sortNew.begin(), sortNew.end(), compare);

	std::set_difference(sortOld.begin(), sortOld.end(),
		sortNew.begin(), sortNew.end(),
		std::back_inserter(diff.despawns), compare);

	std::set_difference(sortNew.begin(), sortNew.end(),
		sortOld.begin(), sortOld.end(),
		std::back_inserter(diff.spawns), compare);

	std::set_intersection(sortOld.begin(), sortOld.end(),
		sortNew.begin(), sortNew.end(),
		std::back_inserter(diff.moves), compare);

	return diff;
}
*/

void Room::FlushImmediateBroadcast()
{
	Protocol::S_IMMEDIATE_FLUSH pkt;
	int32 count = 0;

	for (auto& obj : _immediateFlushQueue)
	{
		count++;
		switch (obj.type)
		{
		case Type::CAST_START:
		case Type::CAST_CANCEL:
		{
			if (obj.eventInfo.has_value())
			{
				auto* skillPkt = pkt.mutable_skill_pkt();
				*skillPkt->add_event() = obj.eventInfo.value();
			}
		} break;
		case Type::MOVE:
		{
			if (obj.object->IsMoveBatch() && obj.object->_hasMove == true)
			{
				auto* movePkt = pkt.mutable_move_pkt();
				*movePkt->add_info() = obj.object->_posInfo;
			}
			obj.object->FlushStateInit();
		} break;
		
		default:
			break;
		};
	}
	_immediateFlushQueue.clear();

	if (IsEmptyImmediatePkt(pkt))
		return;

	_bench.AddImmediatePktCount(count);
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(sendBuffer);
}

void Room::FlushDeferBroadcast()
{
	Protocol::S_DEFER_FLUSH pkt;
	Protocol::HpChange hp;
	Protocol::Death death;
	int32 count = 0;

	for (auto& obj : _deferFlushQueue)
	{
		count++;
		switch (obj.type)
		{
		case Type::SPAWN:
		{
			auto* spawnPkt = pkt.mutable_spawn_pkt();
			*spawnPkt->add_objects() = obj.object->_objectInfo;
		} break;
		case Type::MOVE:
		{
			if (obj.object->IsMoveBatch())
			{
				auto* movePkt = pkt.mutable_move_pkt();
				*movePkt->add_info() = obj.object->_posInfo;
			}
			obj.object->FlushStateInit();
		} break;
		case Type::CAST_START:
		case Type::CAST_CANCEL:
		case Type::CAST_SUCCESS:
		case Type::SKILL_ACTION:
		{
			if (obj.eventInfo.has_value())
			{
				*pkt.mutable_skill_pkt()->add_event() = obj.eventInfo.value();
			}
		} break;
		case Type::HIT:
		{
			auto* hitPkt = pkt.mutable_hit_pkt();
			hp.set_object_id(obj.object->GetId());
			hp.set_hp(obj.object->_statInfo.hp());
			*hitPkt->add_changes() = hp;
		} break;
		case Type::DIE:
		{
			auto* diePkt = pkt.mutable_die_pkt();
			death.set_object_id(obj.object->GetId());
			death.set_attacker_id(obj.object->_attackerId);
			*diePkt->add_death() = death;
		} break;
		case Type::DESPAWN:
		{
			auto* despawnPkt = pkt.mutable_despawn_pkt();
			despawnPkt->add_object_ids(obj.object->GetId());
		} break;
		default:
			break;
		};
	}
	_deferFlushQueue.clear();

	if (IsEmptyDeferPkt(pkt))
		return;

	_bench.AddDeferPktCount(count);
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);

	Broadcast(sendBuffer);
}

void Room::NotifySpawn(ObjectRef object, bool success)
{
	// object가 player일 경우 본인에게 Enter 패킷 + 이미 존재하는 주변 object spawn
	if (object->GetCreatureType() == CREATURE_TYPE_PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(object);

		{
			Protocol::S_ENTER_GAME enterGamePkt;
			enterGamePkt.set_success(success);
			enterGamePkt.mutable_object()->CopyFrom(object->_objectInfo);

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
			if (auto session = player->GetSession())
				session->Send(sendBuffer);
		}
		{
			Protocol::S_SPAWN spawnPkt;

			for (auto& playerIt : _players)
			{
				if (playerIt.first == object->_objectInfo.object_id())
					continue;

				*spawnPkt.add_objects() = playerIt.second->_objectInfo;
			}
			for (auto& monsterIt : _monsters)
			{
				*spawnPkt.add_objects() = monsterIt.second->_objectInfo;
			}
			for (auto& projectileIt : _projectiles)
			{
				*spawnPkt.add_objects() = projectileIt.second->_objectInfo;
			}

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
			if (auto session = player->GetSession())
				session->Send(sendBuffer);
		}
	}

	// 다른 플레이어에게 object 입장 알림 (player, monster, projectile 입장 시)
	object->AddSpawnFlushQueue(object);
}

void Room::AddRemoveList(ObjectRef object)
{
	_removePending.push_back(object);
}

void Room::ClearRemoveList()
{
	for (auto& obj : _removePending)
	{
		LeaveRoom(obj);
		obj->AddDespawnFlushQueue(obj);
	}
}

int32 Room::Index(const Vector2Int& pos) const
{
	int32 localX = pos._x - _gameMap->_minX;
	int32 localY = pos._y - _gameMap->_minY;

	return localY * _gameMap->_sizeX + localX;
}

// 조건 확인
//void Room::AddMoveFlushQueue(ObjectRef obj)
//{
//	if (obj->GetCreatureType() == Protocol::CREATURE_TYPE_PLAYER)
//	{
//		if (static_pointer_cast<Player>(obj)->_isDirty == false)
//			_immediateFlushQueue.push_back({ obj, Type::MOVE });
//		return;
//	}
//	_deferFlushQueue.push_back({ obj, Type::MOVE });
//}
//
//void Room::AddSkillFlushQueue(ObjectRef obj, const Protocol::CastState& state, const SkillEvent& event)
//{
//	
//
//}
//
//void Room::AddHitFlushQueue(ObjectRef obj)
//{
//	_deferFlushQueue.push_back({ obj, Type::HIT });
//}
//
//void Room::AddDieFlushQueue(ObjectRef obj)
//{
//	_deferFlushQueue.push_back({ obj, Type::DIE });
//}
//
//void Room::AddSpawnFlushQueue(ObjectRef obj)
//{
//	_deferFlushQueue.push_back({ obj, Type::SPAWN});
//}
//
//void Room::AddDespawnFlushQueue(ObjectRef obj)
//{
//	_deferFlushQueue.push_back({ obj, Type::DESPAWN });
//}

bool Room::IsEmptyImmediatePkt(const Protocol::S_IMMEDIATE_FLUSH& pkt)
{
	return !pkt.has_move_pkt() && !pkt.has_skill_pkt();
}

bool Room::IsEmptyDeferPkt(const Protocol::S_DEFER_FLUSH& pkt)
{
	return !pkt.has_spawn_pkt() && pkt.has_move_pkt() && !pkt.has_hit_pkt() && !pkt.has_die_pkt() && !pkt.has_despawn_pkt();
}
