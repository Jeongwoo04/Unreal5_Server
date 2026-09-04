#include "pch.h"
#include "BenchMarkManager.h"

float ServerTickInterval = 0.1f;
RoomLog GRoomLog;
RoundLog GRoundLog;
std::shared_mutex GLogLock;
atomic<int32> GMonitoringRoomID{-1};
std::mutex GMonitoringMutex;
std::condition_variable GMonitoringCV;