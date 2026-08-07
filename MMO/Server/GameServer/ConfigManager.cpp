#include "pch.h"
#include "ConfigManager.h"
#include <fstream>
#include <iostream>

bool ConfigManager::LoadConfig(const string& basePath)
{
	std::ifstream file(basePath + "/Config.json");
	if (!file.is_open())
		return false;
	
	json j;
	file >> j;

	const auto& server = j["Server"];

	_config._serverConfig.Port = server.value("Port", 0);
	_config._serverConfig.IP = Utf8ToWString(server.value("IP", ""));
	_config._serverConfig.MaxSession = server.value("MaxSession", 0);
	_config._serverConfig.ServerTick = server.value("ServerTick", 0);
	_config._serverConfig.WorkerTick = server.value("WorkerTick", 0);

	const auto& thread = j["Thread"];

	_config._threadConfig.IO = thread.value("IO", 0);
	_config._threadConfig.LOGIC = thread.value("LOGIC", 0);
	_config._threadConfig.SEND = thread.value("SEND", 0);

	const auto& room = j["Room"];

	_config._roomConfig.MaxRoom = room.value("MaxRoom", 0);
	_config._roomConfig.MaxPlayer = room.value("MaxPlayer", 0);
	_config._roomConfig.MapID = room.value("MapID", 0);
	_config._roomConfig.RoomTick = room.value("RoomTick", 0);

	return true;
}