#include "pch.h"
#include "Object.h"

Object::Object()
{
	_objectInfo.set_object_type(OBJECT_TYPE_NONE);
}

Object::~Object()
{

}

void Object::Update()
{

}

void Object::SetId(uint64 id)
{
	_id = id;
	_objectInfo.set_object_id(id);
	_posInfo.set_object_id(id);
}