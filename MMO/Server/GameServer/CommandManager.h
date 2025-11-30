#pragma once

class CommandManager
{
public:
	using CommandFunc = function<void(const vector<string>&)>;

	void Init();
	void Execute(const string& line);

private:
	unordered_map<string, CommandFunc> _commands;

	void RegisterCommand(const string& name, CommandFunc func);

	static bool IsNumber(const string& s);

	void PrintHelp();

	void CmdList(const vector<string>& args);
	void CmdSpawn(const vector<string>& args);
	void CmdKill(const vector<string>& args);
	void CmdKillAll();
	void CmdDummyStart();
};

