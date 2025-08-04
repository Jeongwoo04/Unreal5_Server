#pragma once
#include "GameMap.h"
#include "Object.h"

template<typename T>
class SpatialHashGrid
{
public:
	void Clear();
	void ApplyAdd(T object, Vector2Int gridPos);
    void ApplyRemove(T object, Vector2Int gridPos);
    void ApplyMove(T object, Vector2Int from, Vector2Int to);
	
    vector<T> FindAround(const Vector2Int& center, int32 radius);
	T FindNearest(const Vector2Int& center, int32 radius, const Vector3& worldPos);
    
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

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
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
inline T SpatialHashGrid<T>::FindNearest(const Vector2Int& center, int32 radius, const Vector3& worldPos)
{
    T nearest = nullptr;
    float minDistSq = numeric_limits<float>::max();

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
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
