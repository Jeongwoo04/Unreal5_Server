#include "pch.h"
#include "GameMap.h"
#include "Object.h"
#include "Room.h"

void GameMap::LoadGameMap(string path)
{
    // Collision file data load
    ifstream inFile(path);
    if (!inFile.is_open())
        return;

    inFile >> _minX >> _maxX >> _minY >> _maxY;

    int32 xCount = _maxX - _minX + 1;
    int32 yCount = _maxY - _minY + 1;

    _collision.resize(yCount, std::vector<bool>(xCount, false));

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

vector<Vector3> GameMap::SimplifyPathRaycast(Vector3& start, vector<Vector2Int>& path)
{
    std::vector<Vector3> result;
    if (path.empty())
        return result;

    int anchor = 0;
    Vector3 anchorPos = start;
    result.push_back(anchorPos);

    for (int i = 1; i < path.size(); ++i)
    {
        Vector3 to = GridToWorld(path[i]);

        if (HasLineOfSightRayCast(anchorPos, to))
        {
            continue; // 아직 직선 가능
        }

        Vector3 candidate = GridToWorld(path[i - 1]);
        if ((candidate - result.back()).Length() >= 1.0f)
        {
            result.push_back(candidate);
        }
        anchorPos = candidate;
        anchor = i - 1;
    }

    Vector3 last = GridToWorld(path.back());
    if ((result.back() - last).Length() >= 1.0f)
    {
        result.push_back(last);
    }

    return result;
}

bool GameMap::HasLineOfSightRayCast(Vector3& from, Vector3& to)
{
    // float 기반 RayCast
    Vector3 dir = (to - from);
    int steps = static_cast<int>(dir.Length() / 0.1f);
    Vector3 step = dir * (1.f / steps);

    Vector3 current = from;
    for (int i = 0; i <= steps; ++i)
    {
        Vector2Int cell = WorldToGrid(current);
        if (!CanGo(cell, false))
            return false;
        current += step;
    }
    return true;
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
        // TODO : Room으로 gridCell 이전 Object 체크는 Room에서. Map은 static collision만
    }

    return true;
}