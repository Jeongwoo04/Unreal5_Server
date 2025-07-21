#include "pch.h"
#include "GameMap.h"

ObjectRef GameMap::Find()
{

	return nullptr;
}

bool GameMap::CanGo(int32 x, int32 y)
{
	if (x < 0 || x >= _sizeX || y < 0 || y >= _sizeY)
		return false;

	return !_collision[y][x];
}

bool GameMap::ApplyLeave()
{

	return true;
}

bool GameMap::ApplyMove()
{

	return true;
}

void GameMap::LoadGameMap(int32 mapId, string pathPrefix)
{
	std::stringstream ss;
	ss << pathPrefix << "/Map_" << std::setfill('0') << std::setw(3) << mapId << ".txt";
	std::string fullPath = ss.str();

	std::ifstream inFile(fullPath);
	if (!inFile.is_open())
	{
		std::cerr << "Failed to open collision map: " << fullPath << std::endl;
		return;
	}

	inFile >> _minX >> _maxX >> _minY >> _maxY;

	int32 xCount = _maxX - _minX + 1;
	int32 yCount = _maxY - _minY + 1;

	_collision.resize(yCount, vector<bool>(xCount, false));
	_objects.resize(yCount, vector<ObjectRef>(xCount, nullptr));

	string line;
	getline(inFile, line);
	for (int32 y = 0; y < yCount; y++)
	{
		getline(inFile, line);
		for (int32 x = 0; x < xCount; x++)
		{
			_collision[y][x] = (line[x] == '1' ? true : false);
		}
	}

	_sizeX = xCount;
	_sizeY = yCount;
}
