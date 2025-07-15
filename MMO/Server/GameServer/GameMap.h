#pragma once
#include <fstream>
#include <sstream>
#include <iomanip>

class GameMap : public enable_shared_from_this<GameMap>
{
	ObjectRef Find();
	bool CanGo();

	bool ApplyLeave();
	bool ApplyMove();

	void LoadGameMap(int32 mapId, string pathPrefix);

public:
	int32 _minX, _maxX, _minY,  _maxY;
	int32 _sizeX, _sizeY;
	vector<vector<bool>> _collision;
	vector<vector<ObjectRef>> _objects;
};