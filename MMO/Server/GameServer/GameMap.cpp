#include "pch.h"
#include "GameMap.h"

ObjectRef GameMap::Find()
{

	return nullptr;
}

bool GameMap::CanGo()
{

	return true;
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
	stringstream ss;
	ss << pathPrefix << "/Map_" << setfill('0') << setw(3) << mapId << ".txt";

	ifstream inFile(ss.str());
	if (!inFile.is_open())
		return;

	inFile >> _minX >> _maxX >> _minY >> _maxY;

	int32 xCount = _maxX - _minX + 1;
	int32 yCount = _maxX - _minY + 1;

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
