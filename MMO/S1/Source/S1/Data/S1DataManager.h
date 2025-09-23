#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "Protocol.pb.h"
#include "Data.h"

using int32 = __int32;

class S1DataManager
{
public:
    static S1DataManager& Instance()
    {
        static S1DataManager instance;
        return instance;
    }

    void LoadData(const string& basePath);

    unordered_map<int32, StatInfo> StatDict;
    unordered_map<int32, Skill> SkillDict;
    unordered_map<int32, ProjectileInfo> ProjectileDict;
    unordered_map<int32, FieldInfo> FieldDict;
};