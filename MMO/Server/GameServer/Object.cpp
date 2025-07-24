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

Vector2Int Object::GetCellPos()
{
	return _gridPos;
}

void Object::SetCellPos(const Protocol::PosInfo& pos)
{
	_gridPos = WorldToGrid(pos);
}

void Object::SetId(uint64 id)
{
	_id = id;
	_objectInfo.set_object_id(id);
	_posInfo.set_object_id(id);
}

void Object::GridToWorld(const Vector2Int& vec, float CELL_SIZE)
{
	_posInfo.set_x(vec._x * CELL_SIZE);
	_posInfo.set_y(vec._y * CELL_SIZE);
}
