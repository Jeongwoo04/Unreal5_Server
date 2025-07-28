#include "pch.h"
#include "GameMap.h"
#include "Object.h"
#include "Room.h"

#define NOMINMAX
#include <limits>

ObjectRef GameMap::Find(Vector2Int cellPos)
{
    if (cellPos._x < _minX || cellPos._x > _maxX)
        return nullptr;
    if (cellPos._y < _minY || cellPos._y > _maxY)
        return nullptr;

    int32 x = cellPos._x - _minX;
    int32 y = _maxY - cellPos._y;
    return _objects[y][x];
}

bool GameMap::ApplyLeave(ObjectRef object)
{
    if (object->GetRoom() == nullptr)
        return false;
    if (object->GetRoom()->GetGameMap() != shared_from_this())
        return false;

    auto posInfo = object->_posInfo;
    if (posInfo.x() < _minX || posInfo.x() > _maxX)
        return false;
    if (posInfo.y() < _minY || posInfo.y() > _maxY)
        return false;

    {
        int32 x = static_cast<int32>(posInfo.x() / CELL_SIZE) - _minX;
        int32 y = _maxY - static_cast<int32>(posInfo.y() / CELL_SIZE);
        if (_objects[y][x] && _objects[y][x]->GetId() == object->GetId())
        {
            _objects[y][x] = nullptr;
        }
    }

    return true;
}

bool GameMap::ApplyMove(ObjectRef object, Vector2Int dest)
{
    ApplyLeave(object);

    if (object->GetRoom() == nullptr)
        return false;
    if (object->GetRoom()->GetGameMap() != shared_from_this())
        return false;

    auto posInfo = object->_posInfo;
    if (CanGo(dest, true) == false)
        return false;

    {
        int32 x = dest._x - _minX;
        int32 y = _maxY - dest._y;
        _objects[y][x] = object;
    }

    return true;
}

void GameMap::LoadGameMap(int32 mapId, string pathPrefix)
{
    stringstream ss;
    ss << pathPrefix << "/Map_" << setfill('0') << setw(3) << mapId << ".txt";
    // Collision file data load
    ifstream inFile(ss.str());
    if (!inFile.is_open())
        return;

    inFile >> _minX >> _maxX >> _minY >> _maxY;

    int32 xCount = _maxX - _minX + 1;
    int32 yCount = _maxY - _minY + 1;

    _collision.resize(yCount, std::vector<bool>(xCount, false));
    _objects.resize(yCount, std::vector<ObjectRef>(xCount, nullptr));

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

vector<Vector2Int> GameMap::FindPath(Vector2Int startCellPos, Vector2Int destCellPos, bool checkObjects)
{
    vector<vector<bool>> closed(_sizeY, vector<bool>(_sizeX, false));
    vector<vector<int32>> open(_sizeY, vector<int32>(_sizeX, std::numeric_limits<int32>::max()));
    vector<vector<Pos>> parent(_sizeY, vector<Pos>(_sizeX));

    priority_queue<PQNode> pq;

    Pos pos = Cell2Pos(startCellPos);
    Pos dest = Cell2Pos(destCellPos);

    int32 h = 10 * (abs(dest._y - pos._y) + abs(dest._x - pos._x));
    open[pos._y][pos._x] = h;
    pq.push({ h, 0, pos._y, pos._x });
    parent[pos._y][pos._x] = pos;

    const int32 deltaY[4] = { 1, -1, 0, 0 };
    const int32 deltaX[4] = { 0, 0, -1, 1 };

    while (!pq.empty()) {
        PQNode node = pq.top(); pq.pop();

        if (closed[node.Y][node.X])
            continue;
        closed[node.Y][node.X] = true;
        if (node.Y == dest._y && node.X == dest._x)
            break;

        for (int32 i = 0; i < 4; ++i) {
            Pos next(node.Y + deltaY[i], node.X + deltaX[i]);
            if (!InRange(next))
                continue;
            if ((next._y != dest._y || next._x != dest._x) && !CanGo(Pos2Cell(next), checkObjects))
                continue;
            if (closed[next._y][next._x])
                continue;

            int32 g = node.G + 10;
            int32 h = 10 * ((dest._y - next._y) * (dest._y - next._y) + (dest._x - next._x) * (dest._x - next._x));
            if (open[next._y][next._x] <= g + h)
                continue;

            open[next._y][next._x] = g + h;
            pq.push({ g + h, g, next._y, next._x });
            parent[next._y][next._x] = { node.Y, node.X };
        }
    }

    return CalcCellPathFromParent(parent, dest);
}

vector<Vector2Int> GameMap::CalcCellPathFromParent(const vector<vector<Pos>>& parent, const Pos& dest)
{
    vector<Vector2Int> cells;
    int32 y = dest._y;
    int32 x = dest._x;
    while (!(parent[y][x] == Pos(y, x))) {
        cells.push_back(Pos2Cell({ y, x }));
        Pos p = parent[y][x];
        y = p._y;
        x = p._x;
    }
    cells.push_back(Pos2Cell({ y, x }));
    reverse(cells.begin(), cells.end());
    return cells;
}

Pos GameMap::Cell2Pos(const Vector2Int& cell)
{
    return { _maxY - cell._y, cell._x - _minX };
}

Vector2Int GameMap::Pos2Cell(const Pos& pos)
{
    return { pos._x + _minX, _maxY - pos._y };
}

bool GameMap::InRange(const Pos& pos)
{
    return pos._y >= 0 && pos._y < _sizeY && pos._x >= 0 && pos._x < _sizeX;
}

bool GameMap::CanGo(Vector2Int cell, bool checkObjects)
{
    Pos pos = Cell2Pos(cell);

    if (!InRange(pos))
        return false;

    if (_collision[pos._y][pos._x])
        return false;

    if (checkObjects)
    {
        ObjectRef obj = _objects[pos._y][pos._x];
        if (obj != nullptr && obj->GetCreatureType() != Protocol::CREATURE_TYPE_PLAYER)
            return false;
    }

    return true;
}