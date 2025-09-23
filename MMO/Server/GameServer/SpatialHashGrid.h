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
	
    vector<T> FindAround(const Vector2Int& center, int32 radius);
    vector<T> FindAroundFloat(const Vector2Int& center, float radius);
	T FindNearest(const Vector2Int& center, int32 radius, const Vector3& worldPos);
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
inline vector<T> SpatialHashGrid<T>::FindAround(const Vector2Int& center, int32 radius)
{
    vector<T> result;

    for (int32 y = -radius; y <= radius; ++y)
    {
        for (int32 x = -radius; x <= radius; ++x)
        {
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
inline vector<T> SpatialHashGrid<T>::FindAroundFloat(const Vector2Int& center, float radius)
{
    vector<T> result;

    int32 range = static_cast<int32>(ceil(radius / CELL_SIZE));

    for (int32 y = -range; y <= range; ++y)
    {
        for (int32 x = -range; x <= range; ++x)
        {
            Vector2Int pos = center + Vector2Int(x, y);

            auto it = _cells.find(pos);
            if (it != _cells.end())
            {
                result.insert(result.end(), it->second.begin(), it->second.end());
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
            Vector2Int pos = center + Vector2Int(x, y);

            result.push_back(pos);
        }
    }

    return result;
}

template<typename T>
inline T SpatialHashGrid<T>::FindNearest(const Vector2Int& center, int32 radius, const Vector3& worldPos)
{
    T nearest = nullptr;
    float minDistSq = numeric_limits<float>::max();

    for (int32 y = -radius; y <= radius; ++y)
    {
        for (int32 x = -radius; x <= radius; ++x)
        {
            Vector2Int cellPos = center + Vector2Int(x, y);

            auto it = _cells.find(cellPos);
            if (it == _cells.end())
                continue;

            for (const T& obj : it->second)
            {
                if (!obj)
                    continue;

                Vector3 objPos(obj->_posInfo);
                float distSq = (objPos - worldPos).LengthSquared();

                if (distSq < minDistSq)
                {
                    minDistSq = distSq;
                    nearest = obj;
                }
            }
        }
    }

    return nearest;
}

template<typename T>
inline T SpatialHashGrid<T>::FindNearestOnPath(Vector3& from, Vector3& to, float thisRadius)
{
    Vector2Int center = WorldToGrid((from + to) * 0.5f);
    float searchRadius = thisRadius + 42.f; // monster capsule

    vector<T> candidates = FindAroundFloat(center, searchRadius);

    T nearest = nullptr;
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
