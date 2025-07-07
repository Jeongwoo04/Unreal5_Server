#pragma once

class Object : public enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

	bool IsPlayer() { return _isPlayer; }

public:
	Protocol::ObjectInfo _objectInfo;
	Protocol::PosInfo _posInfo;

public:
	weak_ptr<Room> _room; // 스마트포인터는 set 할때 멀티스레드에서 위험

protected:
	bool _isPlayer = false;
};

