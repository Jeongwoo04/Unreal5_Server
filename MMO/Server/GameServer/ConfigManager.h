#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ServerConfig
{
	uint16 Port;
	std::wstring IP;
	int32 MaxSession;
	uint64 ServerTick;
	uint64 WorkerTick;	
};

struct ThreadConfig
{
	int32 IO;
	int32 LOGIC;
	int32 SEND;
};

struct RoomConfig
{
	int32 MaxRoom;
	int32 MaxPlayer;
	int32 MapID;
	uint64 RoomTick;
};

struct Config
{
	ServerConfig _serverConfig;
	ThreadConfig _threadConfig;
	RoomConfig _roomConfig;
};

class ConfigManager
{
public:
	static ConfigManager& Instance()
	{
		static ConfigManager instance;
		return instance;
	}

	bool LoadConfig(const string& filePath);
	const Config& GetConfig() const
	{
		return _config;
	}

private:
	Config _config;
};

static std::wstring Utf8ToWString(const std::string& str)
{
	if (str.empty())
		return {};

	int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

	std::wstring result(size, L'\0');

	MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size);

	return result;
}