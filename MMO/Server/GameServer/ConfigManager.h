#pragma once
#include <nlohmann/json.hpp>

struct ServerConfig
{
    string dataPath;
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

    static bool LoadConfig(const string& filePath);
    static const ServerConfig& GetConfig();

private:
    static ServerConfig _config;
};