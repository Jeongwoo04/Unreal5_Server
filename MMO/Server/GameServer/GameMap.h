#pragma once
#include "pch.h"
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

struct Vector2Int
{
    int32 _x = 0;
    int32 _y = 0;

    const float CELL_SIZE = 100.f;

    Vector2Int() = default;
    Vector2Int(int x, int y) : _x(x), _y(y) { }
    Vector2Int(const Protocol::PosInfo& pos)
    {
        _x = static_cast<int32>(round(pos.x() / CELL_SIZE));
        _y = static_cast<int32>(round(pos.y() / CELL_SIZE));
    }

    Vector2Int& operator=(const Vector2Int& other)
    {
        _x = other._x; _y = other._y;
        return *this;
    }

    Vector2Int operator+(const Vector2Int& other) const
    {
        return  Vector2Int(_x + other._x, _y + other._y);
    }

    Vector2Int& operator+=(const Vector2Int& other)
    {
        _x += other._x;
        _y += other._y;
        return *this;
    }

    Vector2Int operator-(const Vector2Int& other) const
    {
        return Vector2Int(_x - other._x, _y - other._y);
    }

    Vector2Int& operator-=(const Vector2Int& other)
    {
        _x -= other._x;
        _y -= other._y;
        return *this;
    }

    float magnitude() const
    {
        return std::sqrt(static_cast<float>(sqrMagnitude()));
    }
    int32 sqrMagnitude() const
    {
        return _x * _x + _y * _y;
    }
    int32 cellDist() const
    {
        return std::abs(_x) + std::abs(_y);
    }

    // Debug 출력
    friend std::ostream& operator<<(std::ostream& os, const Vector2Int& vec)
    {
        os << "(" << vec._x << ", " << vec._y << ")";
        return os;
    }
};

struct Vector3
{
    float _x, _y;
    Vector3() { };
    Vector3(float x, float y): _x(x), _y(y) { }
    Vector3(const Protocol::PosInfo& pos) { _x = pos.x(); _y = pos.y(); }

    Vector3 operator-(const Vector3& other)
    {
        return Vector3(_x - other._x, _y - other._y);
    }

    Vector3 operator+(const Vector3& other) const
    {
        return Vector3(_x + other._x, _y + other._y);
    }

    Vector3& operator+=(const Vector3& other)
    {
        _x += other._x;
        _y += other._y;
        return *this;
    }

    Vector3 operator*(float scalar) const
    {
        return Vector3(_x * scalar, _y * scalar);
    }

    float Length() const
    {
        return sqrtf(_x * _x + _y * _y);
    }

    Vector3 Normalized() const
    {
        float len = sqrtf(_x * _x + _y * _y);
        if (len == 0)
            return Vector3{ 0, 0 };
        return Vector3{ _x / len, _y / len };
    }
};

static const float CELL_SIZE = 100.f;

class GameMap : public std::enable_shared_from_this<GameMap>
{
public:
    ObjectRef Find(Vector2Int cellPos);
    bool CanGo(Vector2Int cell, bool checkObjects = true);

    bool ApplyLeave(ObjectRef object);
    bool ApplyMove(ObjectRef object, Vector2Int dest);

    void LoadGameMap(int32 mapId, string pathPrefix = "../Common/CollisionMap");

    vector<Vector2Int> FindPath(Vector2Int startCellPos, Vector2Int destCellPos, bool checkObjects = true);

    static Vector2Int WorldToGrid(const Vector3& vec, float CELL_SIZE = 100.f)
    {
        return Vector2Int(
            static_cast<int32>(round(vec._x / CELL_SIZE)),
            static_cast<int32>(round(vec._y / CELL_SIZE))
        );
    }
    static Vector3 GridToWorld(const Vector2Int& vec)
    {
        return Vector3(vec._x * CELL_SIZE, vec._y * CELL_SIZE);
    }

    static float YawFromDirection(const Vector3& dir)
    {
        return atan2f(dir._y, dir._x) * (180.f / 3.141592f); // 라디안 → 도
    }

private:
    vector<Vector2Int> CalcCellPathFromParent(const vector<vector<Pos>>& parent, const Pos& dest);

    Pos Cell2Pos(const Vector2Int& cell);
    Vector2Int Pos2Cell(const Pos& pos);

    bool InRange(const Pos& pos);

public:
    int32 _minX, _maxX, _minY, _maxY;
    int32 _sizeX, _sizeY;
    std::vector<std::vector<bool>> _collision;
    std::vector<std::vector<ObjectRef>> _objects;
};