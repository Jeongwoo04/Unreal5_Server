#include "pch.h"
#include "Object.h"

Object::Object()
{
	_objectInfo.set_object_type(Protocol::OBJECT_TYPE_NONE);
}

Object::~Object()
{

}

void Object::Update()
{

}

void Object::OnDamaged(ObjectRef attacker, int32 damage)
{

}

void Object::OnDead(ObjectRef attacker)
{

}

void Object::SetId(uint64 id)
{
	_objectInfo.set_object_id(id);
	_posInfo.set_object_id(id);
}

void Object::SetPosInfo(const PosInfo& posInfo)
{
	_posInfo.CopyFrom(posInfo);
	_objectInfo.mutable_pos_info()->CopyFrom(_posInfo);
	_gridPos = WorldToGrid(Vector3(posInfo));
	_worldPos = Vector3(posInfo);
}

void Object::SetSpawnPos(const Vector3& pos, float yaw)
{
	_posInfo.set_x(pos._x);
	_posInfo.set_y(pos._y);
	_posInfo.set_z(100.f);
	_posInfo.set_yaw(yaw);
	_objectInfo.mutable_pos_info()->CopyFrom(_posInfo);
	_gridPos = WorldToGrid(Vector3(_posInfo));
	_worldPos = Vector3(_posInfo);
}

void Object::SetSpawnRandomPos(Vector3 pos, float yaw)
{
	pos = pos + Utils::GetRandom(0.f, 500.f);
	SetSpawnPos(pos, yaw);
}

void Object::ApplyVectorPos()
{
	_gridPos = WorldToGrid(_worldPos);
	_posInfo.set_x(_worldPos._x);
	_posInfo.set_y(_worldPos._y);
}
