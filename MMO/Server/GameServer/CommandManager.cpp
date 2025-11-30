#include "pch.h"
#include "CommandManager.h"
#include "RoomManager.h"
#include "Room.h"

void CommandManager::Init()
{
	RegisterCommand("help", [this](const vector<string>&) { PrintHelp(); });
	RegisterCommand("list", [this](const vector<string>& args) { CmdList(args); });
	RegisterCommand("spawn", [this](const vector<string>& args) { CmdSpawn(args); });
	RegisterCommand("kill", [this](const vector<string>& args) { CmdKill(args); });
	RegisterCommand("killall", [this](const vector<string>&) { CmdKillAll(); });
	RegisterCommand("dummy", [this](const vector<string>&) { CmdDummyStart(); });
}

void CommandManager::Execute(const string& line)
{
	istringstream iss(line);
	string cmd;
	iss >> cmd;
	if (cmd.empty())
		return;

	vector<string> args;
	string arg;
	while (iss >> arg)
		args.push_back(arg);

	auto it = _commands.find(cmd);
	if (it == _commands.end())
	{
		cout << "[Server] Command: Unknown command: " << cmd << endl;
		return;
	}

	// 숫자 유효성 검사 (명령어별로 필터링 가능)
	for (size_t i = 0; i < args.size(); i++)
	{
		if (!IsNumber(args[i]))
		{
			cout << "Invalid argument: " << args[i] << " (must be a number)" << endl;
			return;
		}
	}

	it->second(args);
}

void CommandManager::RegisterCommand(const string& name, CommandFunc func)
{
	_commands[name] = func;
}

bool CommandManager::IsNumber(const string& s)
{
	return !s.empty() && all_of(s.begin(), s.end(), ::isdigit);
}

void CommandManager::PrintHelp()
{
	cout << "[Server] Available commands:\n";
	cout << "  help\n";
	cout << "  list\n";
	cout << "  spawn <dataId>\n";
	cout << "  spawn <dataId> <x> <y>\n";
	cout << "  spawn <dataId> <x> <y> <count>\n";
	cout << "  kill <dataId>\n";
	cout << "  killall\n";
	cout << "  dummy\n";
}

void CommandManager::CmdList(const vector<string>& args)
{
	if (args.empty())
	{
		cout << "[Server] Command: List All monsters\n";
		auto room = RoomManager::Instance().Find(1);
		room->DoAsyncPushOnly(&Room::GetList);
	}
}

void CommandManager::CmdSpawn(const vector<string>& args)
{
	if (args.empty())
	{
		cout << "[Server] Usage: spawn <dataId> [x] [y] [count]" << endl;
		return;
	}

	int dataId = stoi(args[0]);
	float x = (args.size() >= 2) ? stof(args[1]) : 0.f;
	float y = (args.size() >= 3) ? stof(args[2]) : 0.f;
	int count = (args.size() >= 4) ? stoi(args[3]) : 1;

	cout << "[Server] Command: Spawn dataId=" << dataId << " pos=(" << x << "," << y << ") count=" << count << endl;
	
	auto room = RoomManager::Instance().Find(1);
	Vector3 SpawnPos = { x, y, 100.f };
	room->DoAsyncPushOnly(&Room::Spawn, dataId, false, SpawnPos, count);
}

void CommandManager::CmdKill(const vector<string>& args)
{
	if (args.empty())
	{
		cout << "[Server] Usage: kill <dataId>" << endl;
		return;
	}

	int dataId = stoi(args[0]);
	cout << "[Server] Command: Kill nearest monster of dataId=" << dataId << endl;
	auto room = RoomManager::Instance().Find(1);
	room->DoAsyncPushOnly(&Room::Kill);
}

void CommandManager::CmdKillAll()

{
	cout << "[Server] Command: KillAll monsters removed" << endl;
	auto room = RoomManager::Instance().Find(1);
	room->DoAsyncPushOnly(&Room::KillAll);
}

void CommandManager::CmdDummyStart()
{
	cout << "[Server] Command: 100 Dummy Start" << endl;

	STARTUPINFOW si = { 0 };
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = { 0 };

	// 서버 실행 파일 기준 상대 경로
	std::wstring exePath = L"..\\Binaries\\DummyClient.exe";

	BOOL ok = CreateProcessW(
		exePath.c_str(),   // 실행 파일
		NULL,              // 인자 없음
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE, // 콘솔 열고 싶으면 이 옵션
		NULL,
		NULL,
		&si,
		&pi
	);

	if (!ok)
	{
		DWORD err = GetLastError();
		wprintf(L"[RunDummyClient] Failed (%d)\n", err);
		return ;
	}

	// 프로세스 핸들은 추적하거나 닫을 수 있음
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
