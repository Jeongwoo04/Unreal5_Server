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

    unordered_map<int32, Protocol::StatInfo> StatDict;
    unordered_map<int32, Skill> SkillDict;
};