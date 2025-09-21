#pragma once

class Field : public Object
{
public:
	Field();
	virtual ~Field();

	virtual void Update();

	void GridCaching();

	void SetData(const FieldInfo* data) { _data = data; }
	void SetOwner(ObjectRef owner) { _owner = owner; }

	ObjectRef GetOwner() { return _owner.lock(); }

private:
	weak_ptr<Object> _owner;
	const FieldInfo* _data = nullptr;
	float _elapsedTime = 0.f;

	vector<Vector2Int> _cachedGrids;
	unordered_set<ObjectRef> _affectedTargets;
};

