#include "pch.h"
#include "ConfigManager.h"
#include <fstream>
#include <iostream>

ServerConfig ConfigManager::_config;

bool ConfigManager::LoadConfig(const std::string& filePath)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        std::cerr << "Failed to open config file: " << filePath << std::endl;
        return false;
    }

    json j;
    input >> j;

    _config.dataPath = j.value("dataPath", "");

    return true;
}

const ServerConfig& ConfigManager::GetConfig()
{
    return _config;
}
