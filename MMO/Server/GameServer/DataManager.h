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

    template<typename T>
    const T* GetData(int32 id) const
    {
        const auto* dict = GetDict<T>();
        auto it = dict->find(id);
        return (it != dict->end()) ? &it->second : nullptr;
    }

    template<typename T>
    const std::unordered_map<int32, T>* GetDict() const;

    unordered_map<int32, ObjectTemplate> ObjectDict;
    unordered_map<int32, Protocol::StatInfo> StatDict;
    unordered_map<int32, Skill> SkillDict;
    unordered_map<int32, ProjectileInfo> ProjectileDict;
    unordered_map<int32, MapInfo> MapDataDict;
};

template<>
inline const std::unordered_map<int32, ObjectTemplate>* DataManager::GetDict<ObjectTemplate>() const {
    return &ObjectDict;
}

template<>
inline const std::unordered_map<int32, Protocol::StatInfo>* DataManager::GetDict<Protocol::StatInfo>() const {
    return &StatDict;
}

template<>
inline const std::unordered_map<int32, Skill>* DataManager::GetDict<Skill>() const {
    return &SkillDict;
}

template<>
inline const std::unordered_map<int32, ProjectileInfo>* DataManager::GetDict<ProjectileInfo>() const {
    return &ProjectileDict;
}

template<>
inline const std::unordered_map<int32, MapInfo>* DataManager::GetDict<MapInfo>() const {
    return &MapDataDict;
}