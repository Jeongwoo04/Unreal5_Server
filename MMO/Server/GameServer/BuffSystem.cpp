#include "pch.h"
#include "BuffSystem.h"
#include "DataManager.h"
#include "Object.h"

void BuffSystem::ApplyBuff(ObjectRef target, int32 buffId)
{
	auto it = DataManager::Instance().BuffDict.find(buffId);
	if (it == DataManager::Instance().BuffDict.end())
		return;

	const BuffInfo* buff = &it->second;

	target->AddBuff(*buff);
}
