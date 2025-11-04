#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "Protocol.pb.h"
#include "Data.Contents.h"

class DataManager
{
public:
	static DataManager& Instance()
	{
		static DataManager instance;
		return instance;
	}

	void LoadData(const string& basePath);

	unordered_map<int32, ObjectTemplate> ObjectDict;
	unordered_map<int32, Protocol::StatInfo> StatDict;
	unordered_map<int32, Skill> SkillDict;
	unordered_map<int32, ProjectileInfo> ProjectileDict;
	unordered_map<int32, FieldInfo> FieldDict;
	unordered_map<int32, MapInfo> MapDataDict;
	unordered_map<int32, BuffInfo> BuffDict;
};