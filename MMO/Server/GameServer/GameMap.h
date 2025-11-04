#pragma once
#include "pch.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>

using GameMapRef = shared_ptr<class GameMap>;
using ObjectRef = shared_ptr<class Object>;

struct Pos
{
	Pos() {}
	Pos(int32 y, int32 x) { _y = y; _x = x; }
	int32 _y = 0;
	int32 _x = 0;
	bool operator==(const Pos& other) const { return _y == other._y && _x == other._x; }
};

struct PQNode
{
	int32 F;
	int32 G;
	int32 Y;
	int32 X;

	bool operator<(const PQNode& other) const
	{
		// C#의 CompareTo와 유사. F가 작을수록 우선순위 높음
		if (F == other.F)
			return false; // 같으면 false (priority_queue는 max-heap, false가 우선순위 높음)
		return F > other.F; // F가 작을수록 우선순위 높게
	}
};

class GameMap : public std::enable_shared_from_this<GameMap>
{
public:
	bool CanGo(Vector2Int cell, bool checkObjects = true);

	void LoadGameMap(string path);

	vector<Vector2Int> FindPath(const Vector2Int& startCellPos, const Vector2Int& destCellPos, bool checkObjects = true);
	vector<Vector3> SimplifyPathRaycast(const Vector3& start, const vector<Vector2Int>& path, float deltaStep);
	// RayCast로 장애물 사이의 시야 확인
	bool HasLineOfSightRayCast(const Vector3& from, const Vector3& to, float deltaStep);
	Vector3 GetSafePosRayCast(const Vector3& from, const Vector3& to, float deltaStep, Vector2Int* blocked);

private:
	vector<Vector2Int> CalcCellPathFromParent(const vector<vector<Pos>>& parent, const Pos& dest);

	Pos Cell2Pos(const Vector2Int& cell);
	Vector2Int Pos2Cell(const Pos& pos);

	bool InRange(const Pos& pos);

public:
	int32 _minX, _maxX, _minY, _maxY;
	int32 _sizeX, _sizeY;
	int32 _width, _height;
	vector<vector<bool>> _collision;
};