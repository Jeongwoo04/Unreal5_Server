#include "S1ConfigManager.h"
#include "Logging/LogMacros.h"
#include <fstream>
#include <iostream>

ClientConfig S1ConfigManager::_config;

bool S1ConfigManager::LoadConfig(const string& filePath)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open config file: %s"), *FString(filePath.c_str()));
        return false;
    }

    json j;
    input >> j;

    _config.dataPath = j.value("dataPath", "");

    return true;
}

const ClientConfig& S1ConfigManager::GetConfig()
{
    return _config;
}
