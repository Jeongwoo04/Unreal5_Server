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
	_id = id;
	_objectInfo.set_object_id(id);
	_posInfo.set_object_id(id);
}

void Object::ApplyPos()
{
	_gridPos = GameMap::WorldToGrid(_worldPos);
	_posInfo.set_x(_worldPos._x);
	_posInfo.set_y(_worldPos._y);
	_posInfo.set_yaw(GameMap::YawFromDirection(_dir));
}
