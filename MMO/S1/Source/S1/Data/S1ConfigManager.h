#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

struct ClientConfig
{
    string dataPath;
};

class S1ConfigManager
{
public:
    static S1ConfigManager& Instance()
    {
        static S1ConfigManager instance;
        return instance;
    }

    static bool LoadConfig(const string& filePath);
    static const ClientConfig& GetConfig();

private:
    static ClientConfig _config;
};