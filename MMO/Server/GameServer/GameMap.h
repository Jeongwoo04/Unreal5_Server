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

    Vector2Int() = default;
    Vector2Int(int x, int y) : _x(x), _y(y) { }

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

inline Vector2Int WorldToGrid(const Protocol::PosInfo& pos, float CELL_SIZE = 100.f)
{
    // tileSize는 1셀의 실제 월드 좌표 크기
    return Vector2Int(
        static_cast<int32>(round(pos.x() / CELL_SIZE)),
        static_cast<int32>(round(pos.y() / CELL_SIZE))
    );
}

class GameMap : public std::enable_shared_from_this<GameMap>
{
public:
    ObjectRef Find(Vector2Int cellPos);
    bool CanGo(Vector2Int cell, bool checkObjects = true);

    bool ApplyLeave(ObjectRef object);
    bool ApplyMove(ObjectRef object, Vector2Int dest);

    void LoadGameMap(int32 mapId, string pathPrefix = "../Common/CollisionMap");

    vector<Vector2Int> FindPath(Vector2Int startCellPos, Vector2Int destCellPos, bool checkObjects = true);

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