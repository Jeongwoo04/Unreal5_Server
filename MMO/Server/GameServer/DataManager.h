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

    void LoadData(const std::string& basePath);

    std::unordered_map<int, std::shared_ptr<Protocol::StatInfo>> StatDict;
    std::unordered_map<int, std::shared_ptr<Skill>> SkillDict;
};