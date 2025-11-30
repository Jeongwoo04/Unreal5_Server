#include "pch.h"
#include "SendQueue.h"
#include "Player.h"

SendQueue::SendQueue(string name) : JobQueue(name)
{
}

void SendQueue::SendJob(SendBufferRef sendBuffer, vector<PlayerRef> snapshot, double enqueueTime)
{
    double executeTime = GetTimeMs();
    double delayMs = static_cast<double>(executeTime - enqueueTime);

    auto room = GetRoom();
    if (room == nullptr)
        return;

    room->_diag.AddSendDelay(delayMs);
    room->_diag.SetSendWorkerInfo(_queueName + " | Thread: " + LThreadName);
    for (auto& p : snapshot)
    {
        if (p == nullptr)
            continue;
        if (auto session = p->GetSession())
            session->Send(sendBuffer);
    }
}
