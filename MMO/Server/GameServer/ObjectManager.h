#pragma once
#include "Object.h"
#include "Projectile.h"
#include "Protocol.pb.h"

using namespace Protocol;

class Player;
class Monster;
class Projectile;

class ObjectManager
{
public:
	using createFunc = function<ObjectRef()>;

public:
	void Init();

	int32 GenerateId(ObjectType type);

	int32 FactoryHash(int32 mainType, int32 subType);
	void AddFactory(int32 hash, createFunc func);

	ObjectRef Spawn(const string& templateName = "");

private:
	int32 _counter = 0;

	unordered_map<int32, createFunc> _createRegistry;
	unordered_map<uint64, ObjectRef> _objects;
};