#pragma once

class BuffSystem
{
public:
	static BuffSystem& Instance()
	{
		static BuffSystem instance;
		return instance;
	}

	void ApplyBuff(ObjectRef target, int32 buffId);
};

