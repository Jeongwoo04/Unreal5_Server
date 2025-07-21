#pragma once
#include <fstream>
#include <sstream>
#include <iomanip>

constexpr float PI = 3.14159265f;
constexpr float DEG2RAD = PI / 180.0f;
const float CELL_SIZE = 100.0f;

struct Vector2float
{
	float _x = 0;
	float _y = 0;

	Vector2float() = default;
	Vector2float(float x, float y) : _x(x), _y(y) { }

	Vector2float operator+(const Vector2float& rhs) const
	{
		return Vector2float(_x + rhs._x, _y + rhs._y);
	}

	// ¿ìÃø ½ºÄ®¶ó °ö: Vector * float
	Vector2float operator*(float scalar) const
	{
		return Vector2float(_x * scalar, _y * scalar);
	}

	// ÁÂÃø ½ºÄ®¶ó °ö: float * Vector
	friend Vector2float operator*(float scalar, const Vector2float& vec)
	{
		return Vector2float(vec._x * scalar, vec._y * scalar);
	}
};

struct Vector2Int
{
	int32 _x = 0;
	int32 _y = 0;

	Vector2Int() = default;
	Vector2Int(int32 x, int32 y) : _x(x), _y(y) { }

	//Vector2Int operator+(const Vector2Int& other) const
	//{
	//	return  Vector2Int(_x + other._x, _y + other._y);
	//}

	//Vector2Int& operator+=(const Vector2Int& other)
	//{
	//	_x += other._x;
	//	_y += other._y;
	//	return *this;
	//}

	//Vector2Int operator-(const Vector2Int& other) const
	//{
	//	return Vector2Int(_x - other._x, _y - other._y);
	//}

	//Vector2Int& operator-=(const Vector2Int& other)
	//{
	//	_x -= other._x;
	//	_y -= other._y;
	//	return *this;
	//}
};

inline Vector2Int WorldToGrid(float x, float y)
{
	return Vector2Int(
		static_cast<int32>(floor(x / CELL_SIZE)),
		static_cast<int32>(floor(y / CELL_SIZE)));
}

inline Vector2float GridToWorld(int32 x, int32 y)
{
	return Vector2float(
		x * CELL_SIZE + CELL_SIZE / 2.0f,
		y * CELL_SIZE + CELL_SIZE / 2.0f);
}

class GameMap : public enable_shared_from_this<GameMap>
{
public:
	ObjectRef Find();
	bool CanGo(int32 x, int32 y);

	bool ApplyLeave();
	bool ApplyMove();

	void LoadGameMap(int32 mapId, string pathPrefix = "../Common/CollisionMap");

public:
	int32 _minX, _maxX, _minY,  _maxY;
	int32 _sizeX, _sizeY;
	vector<vector<bool>> _collision;
	vector<vector<ObjectRef>> _objects;
};