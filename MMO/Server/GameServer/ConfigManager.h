#pragma once
#include <nlohmann/json.hpp>

struct ServerConfig
{
    std::string dataPath;
};

using json = nlohmann::json;

class ConfigManager
{
public:
    static ConfigManager& Instance()
    {
        static ConfigManager instance;
        return instance;
    }

    static bool LoadConfig(const std::string& filePath);
    static const ServerConfig& GetConfig();

private:
    static ServerConfig _config;
};