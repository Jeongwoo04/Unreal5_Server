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

    void LoadData(const std::string& basePath);

    std::unordered_map<int32, Protocol::StatInfo> StatDict;
    std::unordered_map<int32, Skill> SkillDict;
};