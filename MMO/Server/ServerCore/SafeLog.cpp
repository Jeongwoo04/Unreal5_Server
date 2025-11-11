#include "pch.h"
#include "SafeLog.h"

namespace
{
	USE_LOCK;
}

void SafeLog(const std::string& msg)
{
	WRITE_LOCK;
	std::cout << msg << std::endl;
}