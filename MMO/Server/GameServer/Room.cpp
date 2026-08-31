#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Monster.h"
#include "Projectile.h"
#include "Field.h"
#include "ObjectManager.h"
#include "SkillSystem.h"
#include "SendQueue.h"

using namespace std::chrono;
using namespace std::chrono_literals;

Room::Room(uint64 roomTick, string name) : JobQueue(name), _roomTick(roomTick)
{
	_sendQueue = make_shared<SendQueue>("SendQueue_" + name);
	_nextTick = GServerStartTick + _roomTick + (GetRoomId() % 100);

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

	_playerGrid = SpatialGrid<PlayerRef>(_gameMap, 20);
	_monsterGrid = SpatialGrid<MonsterRef>(_gameMap, 50);

	_skillSystem->SetRoom(static_pointer_cast<Room>(shared_from_this()));
	_sendQueue->SetRoom(static_pointer_cast<Room>(shared_from_this()));

	SpawnInit();

	//UpdateTick();
	DoTimer(_nextTick - GetTickCount64(), &Room::UpdateTick);
}

void Room::UpdateTick()
{
	if (MonitoringRoom)
		_diag.BeginTick();

#ifdef BENCHMARK

	_bench.End("TickInterval");
	_bench.Begin("Room");

#endif

	_nextTick += _roomTick;

	uint64 now = ::GetTickCount64();

	while (_nextTick <= now)
		_nextTick += _roomTick;

	// Tick-Drift 방지 (Jitter 존재 확인 필요)
	DoTimer(_nextTick - now, &Room::UpdateTick);


#ifdef BENCHMARK

	_bench.Begin("I-Flush");
	FlushImmediateBroadcast();
	_bench.End("I-Flush");

#else

	FlushImmediateBroadcast();

#endif

#ifdef USE_OPTIMIZED_MEMORY_POOLING
	
	_bench.Begin("Update");
	_objectManager->Update();
	_bench.End("Update");

#else

	_bench.Begin("Update");
	
	UpdateMonster();
	UpdateProjectile();
	UpdateField();
	
	_bench.End("Update");

#endif

#ifdef BENCHMARK

	_bench.Begin("SkillSystem");
	UpdateSkillSystem();
	_bench.End("SkillSystem");

	ClearRemoveList();

	_bench.Begin("EnterFlush");
	FlushEnterPkt();
	_bench.End("EnterFlush");

	_bench.Begin("D-Flush");
	FlushDeferBroadcast();
	_bench.End("D-Flush");
	
	_bench.End("Room");
	_bench.Begin("TickInterval");
	_bench.SendData(GetRoomId());

#else

	UpdateSkillSystem();
	ClearRemoveList();
	FlushEnterPkt();
	FlushDeferBroadcast();

#endif

	if (MonitoringRoom)
	{
		_diag.EndTick();
		_diag.SendRoomData(_objectManager->CheckPools(), _roomId);
	}
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

// TEMP: Command Spawn ///////////////////////////////
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

void Room::KillPlayer()
{
	for (auto& [id, player] : _players)
	{
		AddRemoveList(player);
	}
}

/////////////////////////////////////////////////////

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
	
	RegisterSpawn(object, success);

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

bool Room::LeaveGame(GameSessionRef session)
{
	PlayerRef player = session->_player;

	if (player == nullptr)
		return false;

	session->_player = nullptr;

	bool removed = LeaveRoom(player);

	if (removed)
	{
		// 성공적으로 Room에서 빠졌으니 클라에 알림
		Protocol::S_DESPAWN pkt;
		pkt.add_object_ids(player->GetId());
		player->AddDespawnFlushQueue(player);
	}

	// 나에게 퇴장 패킷 보내기
	{
		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (session)
			session->Send(sendBuffer);
	}

	return true;
}

bool Room::HandleEnterPlayer(GameSessionRef gameSession, bool monitoringRoom)
{
	PlayerRef player = static_pointer_cast<Player>(_objectManager->Spawn(10101, true, {0, 0, 0}));
	if (player == nullptr)
		return false;

	gameSession->_player = player;
	if (gameSession->_player == nullptr)
		return false;
	player->SetSession(gameSession);

	if (monitoringRoom)
	{
		MonitoringRoom = true;
		GMonitoringCV.notify_one();
	}

	return EnterRoom(player);
}

bool Room::HandleLeavePlayer(GameSessionRef session)
{	
	return LeaveGame(session);
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
		SkillInstanceRef activeSkill = player->GetActiveSkill();
		if (activeSkill)
		{
			_skillSystem->CancelCasting(player, activeSkill->castId);
		}
	}
	else if (player->GetState() == Protocol::STATE_MACHINE_SKILL)
	{
		return;
	}

	Vector3 destPos = Vector3(pkt.info());

	player->_posInfo.set_state(pkt.info().state());
	player->MoveToNextPos(destPos);
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

			_isLeavePlayer = true;
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

	return eraseCount > 0 ? true : false;
}

void Room::BroadcastTargets(SendBufferRef sendBuffer, vector<PlayerRef>& snapShot)
{
	auto enqueueTime = GetTimeMs();
	_sendQueue->DoAsyncSendJob(&SendQueue::SendJob, sendBuffer, snapShot, enqueueTime, false);
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
	_sendQueue->DoAsyncSendJob(&SendQueue::SendJob, sendBuffer, snapshot, enqueueTime, true);
}

void Room::BroadcastNearby(SendBufferRef sendBuffer, const Vector3& center, uint64 exceptId)
{
	vector<PlayerRef> nearbyPlayers =
		_playerGrid.FindAround(WorldToGrid(center), BROADCAST_RANGE);

	auto enqueueTime = GetTimeMs();
	_sendQueue->DoAsyncSendJob(&SendQueue::SendJob,
		sendBuffer, nearbyPlayers, enqueueTime, true);
}

void Room::BroadcastMove(const Protocol::PosInfo& posInfo, uint64 exceptId)
{
	Protocol::S_MOVE movePkt;
	*movePkt.add_info() = posInfo;

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);

	BroadcastNearby(sendBuffer, Vector3(posInfo), exceptId);
}

void Room::FlushImmediateBroadcast()
{
	Protocol::S_IMMEDIATE_FLUSH immediateFlushPkt;

	auto* skillPkt = immediateFlushPkt.mutable_skill_pkt();
	auto* movePkt = immediateFlushPkt.mutable_move_pkt();
		
	for (auto& obj : _immediateFlushQueue)
	{
		switch (obj.type)
		{
		case Type::CAST_START:
		case Type::CAST_CANCEL:
		{
			if (obj.eventInfo.has_value())
				*skillPkt->add_event() = obj.eventInfo.value();
		} break;
		case Type::MOVE:
		{
			if (obj.object->IsMoveBatch() && obj.object->_hasMove == true)
				*movePkt->add_info() = obj.object->_posInfo;

			obj.object->FlushStateInit();
		} break;
		
		default:
			_playerGrid.ApplyMove(static_pointer_cast<Player>(obj.object), WorldToGrid(obj.object->_lastFlushPos), obj.object->_gridPos);
			break;
		};
	}
	//int32 count = _immediateFlushQueue.size();
	int32 byteSize = immediateFlushPkt.ByteSizeLong();
	//_diag.SetImmediateFlushInfo(count, byteSize);
	if (MonitoringRoom)
		_diag.SetImmediateFlushInfo(byteSize);

	if (IsEmptyImmediatePkt(immediateFlushPkt))
		return;

	_immediateFlushQueue.clear();

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(immediateFlushPkt);
	Broadcast(sendBuffer);
}

void Room::FlushEnterPkt()
{
	for (auto& player : _enterSnapShot)
	{
		if (auto session = player->GetSession())
		{
			S_ENTER_GAME pkt;
			pkt.set_success(true);
			pkt.mutable_object()->CopyFrom(player->_objectInfo);

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
			session->Send(sendBuffer);
		}
	}
}

void Room::FlushDeferBroadcast()
{

	Protocol::S_DEFER_FLUSH newcomerPkt;
	Protocol::S_DEFER_FLUSH existingPkt;

	auto* newcomerSpawnPkt = newcomerPkt.mutable_spawn_pkt();
	auto* newcomerMovePkt = newcomerPkt.mutable_move_pkt();
	auto* newcomerSkillPkt = newcomerPkt.mutable_skill_pkt();
	auto* newcomerHitPkt = newcomerPkt.mutable_hit_pkt();
	auto* newcomerDiePkt = newcomerPkt.mutable_die_pkt();
	auto* newcomerDespawnPkt = newcomerPkt.mutable_despawn_pkt();

	auto* existingSpawnPkt = existingPkt.mutable_spawn_pkt();
	auto* existingMovePkt = existingPkt.mutable_move_pkt();
	auto* existingSkillPkt = existingPkt.mutable_skill_pkt();
	auto* existingHitPkt = existingPkt.mutable_hit_pkt();
	auto* existingDiePkt = existingPkt.mutable_die_pkt();
	auto* existingDespawnPkt = existingPkt.mutable_despawn_pkt();

	Protocol::HpChange hp;
	Protocol::Death death;

	vector<PlayerRef> enterSnapShot;

	int32 count = _deferFlushQueue.size();

	for (auto& obj : _deferFlushQueue)
	{
		count++;
		switch (obj.type)
		{
		case Type::SPAWN:
		{
			//auto* spawnPkt = pkt.mutable_spawn_pkt();
			*existingSpawnPkt->add_objects() = obj.object->_objectInfo;
		} break;
		case Type::MOVE:
		{
			if (obj.object->IsMoveBatch())
			{
				//auto* movePkt = pkt.mutable_move_pkt();
				*existingMovePkt->add_info() = obj.object->_posInfo;
				if (_isEnterPlayer)
					*newcomerMovePkt->add_info() = obj.object->_posInfo;
			}
			obj.object->FlushStateInit();
		} break;
		case Type::CAST_START:
		{
			if (obj.eventInfo.has_value())
			{
				//*pkt.mutable_skill_pkt()->add_event() = obj.eventInfo.value();
				*existingSkillPkt->add_event() = obj.eventInfo.value();
				if (_isEnterPlayer)
					*newcomerSkillPkt->add_event() = obj.eventInfo.value();
			}
		} break;
		case Type::CAST_CANCEL:
		{
			if (obj.eventInfo.has_value())
			{
				//*pkt.mutable_skill_pkt()->add_event() = obj.eventInfo.value();
				*existingSkillPkt->add_event() = obj.eventInfo.value();
				if (_isEnterPlayer)
					*newcomerSkillPkt->add_event() = obj.eventInfo.value();
			}
		} break;
		case Type::CAST_SUCCESS:
		{
			if (obj.eventInfo.has_value())
			{
				//*pkt.mutable_skill_pkt()->add_event() = obj.eventInfo.value();
				*existingSkillPkt->add_event() = obj.eventInfo.value();
				if (_isEnterPlayer)
					*newcomerSkillPkt->add_event() = obj.eventInfo.value();

			}
		} break;
		case Type::SKILL_ACTION:
		{
			if (obj.eventInfo.has_value())
			{
				//*pkt.mutable_skill_pkt()->add_event() = obj.eventInfo.value();
				*existingSkillPkt->add_event() = obj.eventInfo.value();
				if (_isEnterPlayer)
					*newcomerSkillPkt->add_event() = obj.eventInfo.value();
			}
		} break;
		case Type::HIT:
		{
			//auto* hitPkt = pkt.mutable_hit_pkt();
			hp.set_object_id(obj.object->GetId());
			hp.set_hp(obj.object->_statInfo.hp());
			*existingHitPkt->add_changes() = hp;
			if (_isEnterPlayer)
				*newcomerHitPkt->add_changes() = hp;
		} break;
		case Type::DIE:
		{
			//auto* diePkt = pkt.mutable_die_pkt();
			death.set_object_id(obj.object->GetId());
			death.set_attacker_id(obj.object->_attackerId);
			*existingDiePkt->add_death() = death;
			if (_isEnterPlayer)
				*newcomerDiePkt->add_death() = death;
		} break;
		case Type::DESPAWN:
		{
			//auto* despawnPkt = pkt.mutable_despawn_pkt();
			existingDespawnPkt->add_object_ids(obj.object->GetId());
			if (_isEnterPlayer)
				newcomerDespawnPkt->add_object_ids(obj.object->GetId());

			obj.object->SetRoom(nullptr);
		} break;
		default:
			break;
		};
	}

	if (_isEnterPlayer)
	{
		for (auto& [id, player] : _players)
		{
			*newcomerSpawnPkt->add_objects() = player->_objectInfo;
		}
		for (auto& [id, monster] : _monsters)
		{
			*newcomerSpawnPkt->add_objects() = monster->_objectInfo;
		}
		for (auto& [id, projectile] : _projectiles)
		{
			*newcomerSpawnPkt->add_objects() = projectile->_objectInfo;
		}
		for (auto& [id, field] : _fields)
		{
			*newcomerSpawnPkt->add_objects() = field->_objectInfo;
		}

		enterSnapShot = std::move(_enterSnapShot);

		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(newcomerPkt);
		BroadcastTargets(sendBuffer, enterSnapShot);
	}
	
	if (!_nativeSnapShot.empty())
	{
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(existingPkt);
		BroadcastTargets(sendBuffer, _nativeSnapShot);
	}

	_deferFlushQueue.clear();

	if (_isEnterPlayer || _isLeavePlayer)
	{
		_nativeSnapShot.clear();
		_nativeSnapShot.reserve(_players.size());

		for (auto& [id, player] : _players)
			_nativeSnapShot.push_back(player);
	}

	_isEnterPlayer = false;
	_isLeavePlayer = false;

	int32 byteSize = existingPkt.ByteSizeLong();
	//_diag.SetDeferFlushInfo(count, byteSize);
	if (MonitoringRoom)
		_diag.SetDeferFlushInfo(byteSize);

	//Broadcast(sendBuffer);
}

void Room::RegisterSpawn(ObjectRef object, bool success)
{
	// object가 player일 경우 본인에게 Enter 패킷 + 이미 존재하는 주변 object spawn
	if (object->GetCreatureType() == CREATURE_TYPE_PLAYER)
	{
		_enterSnapShot.push_back(static_pointer_cast<Player>(object));
		_isEnterPlayer = true;
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
	_removePending.clear();
}

int32 Room::Index(const Vector2Int& pos) const
{
	int32 localX = pos._x - _gameMap->_minX;
	int32 localY = pos._y - _gameMap->_minY;

	return localY * _gameMap->_sizeX + localX;
}

bool Room::IsEmptyImmediatePkt(const Protocol::S_IMMEDIATE_FLUSH& pkt)
{
	return (pkt.skill_pkt().event_size() == 0 &&
		pkt.move_pkt().info_size() == 0);
}

bool Room::IsEmptyDeferPkt(const Protocol::S_DEFER_FLUSH& pkt)
{
	return (pkt.spawn_pkt().objects_size() == 0 &&
		pkt.move_pkt().info_size() == 0 &&
		pkt.skill_pkt().event_size() == 0 && // 스킬 추가
		pkt.hit_pkt().changes_size() == 0 &&
		pkt.die_pkt().death_size() == 0 &&
		pkt.despawn_pkt().object_ids_size() == 0);
}