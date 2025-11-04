#pragma once

template<typename TObjectPtr>
class SpatialGrid
{
public:
	using ObjectPtr = TObjectPtr;

	SpatialGrid() = default;

	SpatialGrid(const GameMapRef& map)
	{
		_minX = map->_minX;
		_maxX = map->_maxX;
		_minY = map->_minY;
		_maxY = map->_maxY;

		_width = _maxX - _minX + 1;
		_height = _maxY - _minY + 1;

		_cells.resize(_width * _height);
	}

	void Clear()
	{
		for (auto& cell : _cells)
			cell.clear();
	}

	void ApplyAdd(ObjectPtr object, Vector2Int gridPos)
	{
		if (!IsValid(gridPos))
			return;

		_cells[Index(gridPos)].push_back(object);
	}

	void ApplyRemove(ObjectPtr object, Vector2Int gridPos)
	{
		if (!IsValid(gridPos))
			return;
		auto& vec = _cells[Index(gridPos)];
		vec.erase(remove(vec.begin(), vec.end(), object), vec.end());
	}

	// Move
	void ApplyMove(ObjectPtr object, Vector2Int from, Vector2Int to)
	{
		if (from == to)
			return;

		if (IsValid(from))
		{
			auto& vec = _cells[Index(from)];
			vec.erase(remove(vec.begin(), vec.end(), object), vec.end());
		}

		if (IsValid(to))
		{
			_cells[Index(to)].push_back(object);
		}
	}

	vector<ObjectPtr> Find(const Vector2Int& cell) const
	{
		if (!IsValid(cell))
			return {};

		return _cells[Index(cell)];
	}

	vector<ObjectPtr>& GetCellPlayers(int32 index)
	{
		static vector<ObjectPtr> empty;

		if (!IsValid(index))
			return empty;

		return _cells[index];
	}

	vector<ObjectPtr> FindAround(const Vector2Int& center, int32 radius) const
	{
		vector<ObjectPtr> result;

		for (int32 y = -radius; y <= radius; ++y)
		{
			for (int32 x = -radius; x <= radius; ++x)
			{
				if (x * x + y * y > radius * radius)
					continue;

				Vector2Int pos(center._x + x, center._y + y);
				if (!IsValid(pos))
					continue;

				const auto& cellObjects = _cells[Index(pos)];
				result.insert(result.end(), cellObjects.begin(), cellObjects.end());
			}
		}

		return result;
	}

	vector<ObjectPtr> FindAroundFloat(const Vector3& center, float radius) const
	{
		vector<ObjectPtr> result;
		int32 range = static_cast<int32>(ceil(radius / CELL_SIZE));
		Vector2Int gridCenter(WorldToGrid(center));

		for (int32 y = -range; y <= range; ++y)
		{
			for (int32 x = -range; x <= range; ++x)
			{
				Vector2Int pos = gridCenter + Vector2Int(x, y);
				if (!IsValid(pos))
					continue;

				for (auto& obj : _cells[Index(pos)])
				{
					if (!obj)
						continue;

					float totalRadius = radius + obj->_collisionRadius;
					if ((obj->_worldPos - center).LengthSquared2D() <= totalRadius * totalRadius)
						result.push_back(obj);
				}
			}
		}
		return result;
	}

	vector<Vector2Int> FindGridAroundFloat(const Vector2Int& center, float radius) const
	{
		vector<Vector2Int> result;
		int32 range = static_cast<int32>(ceil(radius / CELL_SIZE));

		for (int32 y = -range; y <= range; ++y)
		{
			for (int32 x = -range; x <= range; ++x)
			{
				if ((x * CELL_SIZE * x * CELL_SIZE + y * CELL_SIZE * y * CELL_SIZE) > radius * radius) continue;
				Vector2Int pos = center + Vector2Int(x, y);
				if (IsValid(pos)) result.push_back(pos);
			}
		}
		return result;
	}

	ObjectPtr FindNearest(const Vector2Int& center, float radius, const Vector3& worldPos) const
	{
		ObjectPtr nearest = nullptr;
		float minDistSq = std::numeric_limits<float>::max();
		float radiusSq = radius * radius;

		int32 range = static_cast<int32>(ceil(radius / CELL_SIZE));

		for (int32 y = -range; y <= range; ++y)
		{
			for (int32 x = -range; x <= range; ++x)
			{
				Vector2Int cell(center._x + x, center._y + y);
				if (!IsValid(cell))
					continue;

				for (auto& obj : _cells[Index(cell)])
				{
					if (!obj || obj->IsDead())
						continue;

					float distSq = (obj->_worldPos - worldPos).LengthSquared2D();
					if (distSq <= radiusSq && distSq < minDistSq)
					{
						minDistSq = distSq;
						nearest = obj;
					}
				}
			}
		}

		return nearest;
	}

	ObjectPtr FindNearestOnPath(Vector3& from, Vector3& to, float thisRadius) const
	{
		Vector3 center = (from + to) * 0.5f;
		float searchRadius = thisRadius + 42.f; // monster capsule

		vector<ObjectPtr> candidates = FindAroundFloat(center, searchRadius);
		ObjectPtr nearest = nullptr;
		float minT = 1.0f;

		for (auto& target : candidates)
		{
			if (!target || target->IsDead()) continue;
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
		}
		return nearest;
	}

public:
	bool IsValid(int32 index) const
	{
		return index >= 0 && index < _width * _height;
	}

	bool IsValid(const Vector2Int& pos) const
	{
		return pos._x >= _minX && pos._x <= _maxX &&
			pos._y >= _minY && pos._y <= _maxY;
	}

	int32 Index(const Vector2Int& pos) const
	{
		int32 localX = pos._x - _minX;
		int32 localY = pos._y - _minY;
		return localY * _width + localX;
	}

private:
	int32 _minX = 0;
	int32 _maxX = 0;
	int32 _minY = 0;
	int32 _maxY = 0;

	int32 _width = 0;
	int32 _height = 0;

	vector<vector<ObjectPtr>> _cells;
};