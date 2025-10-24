#pragma once
#include "GameMap.h"
#include "Object.h"
#include "Monster.h"
#include "Player.h"
#include "Utils.h"

template<typename T>
class SpatialHashGrid
{
public:
	void Clear();
	void ApplyAdd(T object, Vector2Int gridPos);
    void ApplyRemove(T object, Vector2Int gridPos);
    void ApplyMove(T object, Vector2Int from, Vector2Int to);
	    
    vector<T> Find(const Vector2Int& cell);
    vector<T> FindAround(const Vector2Int& center, int32 radius);
    vector<T> FindAroundFloat(const Vector3& center, float radius);
	T FindNearest(const Vector2Int& center, float radius, const Vector3& worldPos);
    T FindNearestOnPath(Vector3& from, Vector3& end, float thisRadius);
    
    vector<Vector2Int> FindGridAroundFloat(const Vector2Int& center, float radius);

private:
	unordered_map<Vector2Int, vector<T>, Vector2IntHash> _cells;
};

template<typename T>
inline void SpatialHashGrid<T>::Clear()
{
	_cells.clear();
}

template<typename T>
inline void SpatialHashGrid<T>::ApplyAdd(T object, Vector2Int gridPos)
{
    _cells[gridPos].push_back(object);
}

template<typename T>
inline void SpatialHashGrid<T>::ApplyRemove(T object, Vector2Int gridPos)
{
    auto it = _cells.find(gridPos);
    if (it != _cells.end())
    {
        auto& vec = it->second;
        vec.erase(remove(vec.begin(), vec.end(), object), vec.end());

        // 해당 셀이 비면 제거
        if (vec.empty())
            _cells.erase(it);
    }
}

template<typename T>
inline void SpatialHashGrid<T>::ApplyMove(T object, Vector2Int from, Vector2Int to)
{
    if (from == to)
        return;

    ApplyRemove(object, from);
    ApplyAdd(object, to);
}

template<typename T>
inline vector<T> SpatialHashGrid<T>::Find(const Vector2Int& cell)
{
    vector<T> results;
    auto it = _cells.find(cell);
    if (it != _cells.end())
        return it->second;
    else
        return results;
}

template<typename T>
inline vector<T> SpatialHashGrid<T>::FindAround(const Vector2Int& center, int32 radius)
{
    vector<T> result;

    for (int32 y = -radius; y <= radius; ++y)
    {
        for (int32 x = -radius; x <= radius; ++x)
        {
            if ((x * x) + (y * y) > radius * radius)
                continue;

            Vector2Int pos = center + Vector2Int(x, y);
            auto it = _cells.find(pos);
            if (it != _cells.end())
            {
                const vector<T>& cellObjects = it->second;
                result.insert(result.end(), cellObjects.begin(), cellObjects.end());
            }
        }
    }

    return result;
}

template<typename T>
inline vector<T> SpatialHashGrid<T>::FindAroundFloat(const Vector3& center, float radius)
{
    vector<T> result;

    int32 range = static_cast<int32>(ceil(radius / CELL_SIZE));

    for (int32 y = -range; y <= range; ++y)
    {
        for (int32 x = -range; x <= range; ++x)
        {
            Vector2Int pos = WorldToGrid(center) + Vector2Int(x, y);

            auto it = _cells.find(pos);
            if (it == _cells.end())
                continue;

            float totalRadius = 0.f;
            for (auto& obj : it->second)
            {
                totalRadius = radius + obj->_collisionRadius;
                if ((obj->_worldPos - center).LengthSquared2D() <= totalRadius * totalRadius)
                    result.push_back(obj);
            }
        }
    }

    return result;
}

template<typename T>
inline vector<Vector2Int> SpatialHashGrid<T>::FindGridAroundFloat(const Vector2Int& center, float radius)
{
    vector<Vector2Int> result;

    int32 range = static_cast<int32>(ceil(radius / CELL_SIZE));

    for (int32 y = -range; y <= range; ++y)
    {
        for (int32 x = -range; x <= range; ++x)
        {
            if ((x * CELL_SIZE * x * CELL_SIZE) + (y * CELL_SIZE * y * CELL_SIZE) > radius * radius)
                continue;
            Vector2Int pos = center + Vector2Int(x, y);

            result.push_back(pos);
        }
    }

    return result;
}

template<typename T>
inline T SpatialHashGrid<T>::FindNearest(const Vector2Int& center, float radius, const Vector3& worldPos)
{
    T nearest = {};
    float minDistSq = numeric_limits<float>::max();
    float radiusSq = (radius * CELL_SIZE) * (radius * CELL_SIZE);
    int32 range = static_cast<int32>(radius);
    const float EARLY_EXIT_DIST = CELL_SIZE;

    for (int32 y = -range; y <= range; ++y)
    {
        for (int32 x = -range; x <= range; ++x)
        {
            if ((x * CELL_SIZE * x * CELL_SIZE + y * CELL_SIZE * y * CELL_SIZE) > radiusSq)
                continue;

            Vector2Int cellPos = center + Vector2Int(x, y);

            auto it = _cells.find(cellPos);
            if (it == _cells.end())
                continue;

            for (const T& obj : it->second)
            {
                if (!obj || obj->IsDead())
                    continue;

                float distSq = (obj->_worldPos - worldPos).LengthSquared2D();
                if (distSq > radiusSq)
                    continue;

                if (distSq < minDistSq)
                {
                    minDistSq = distSq;
                    nearest = obj;

                    if (minDistSq < EARLY_EXIT_DIST * EARLY_EXIT_DIST)
                        return nearest;
                }
            }
        }
    }

    return nearest;
}

template<typename T>
inline T SpatialHashGrid<T>::FindNearestOnPath(Vector3& from, Vector3& to, float thisRadius)
{
    Vector3 center = (from + to) * 0.5f;
    float searchRadius = thisRadius + 42.f; // monster capsule

    vector<T> candidates = FindAroundFloat(center, searchRadius);

    T nearest = {};
    float minT = 1.0f;

    for (T target : candidates)
    {
        if (!target || target->IsDead())
            continue;

        Vector3 targetPos(target->_posInfo);
        float totalRadius = thisRadius + target->_collisionRadius;

        float hitT;

        if (CheckCapsuleHitWithT(from, to, targetPos, totalRadius, hitT))
        {
            if (hitT < minT)
            {
                minT = hitT;
                nearest = target;
            }
        }
        else
        {
            ;
        }
    }

    if (nearest)
        ;
    else
        ;

    return nearest;
}
