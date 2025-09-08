#pragma once
#include "Object.h"
#include "Protocol.pb.h"

using namespace Protocol;

class ObjectManager
{
public:
	using createFunc = function<ObjectRef()>;

public:
	void Init();

	int32 GenerateId(ObjectType type);

	int32 FactoryHash(int32 mainType, int32 subType);
	void AddFactory(int32 hash, createFunc func);

	ObjectRef Spawn(int32 dataId, const PosInfo& posInfo);
	ObjectRef Spawn(int32 dataId, bool randPos, const Vector3& pos, float yaw = 0.f);
	
	void Despawn(ObjectRef obj);
	void Despawn(uint64 objId);

private:
	int32 _counter = 0;

	unordered_map<int32, createFunc> _createRegistry;
	unordered_map<uint64, ObjectRef> _objects;
};