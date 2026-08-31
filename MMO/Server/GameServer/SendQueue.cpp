#include "pch.h"
#include "SendQueue.h"
#include "Player.h"

SendQueue::SendQueue(string name) : JobQueue(name)
{
}

void SendQueue::SendJob(SendBufferRef sendBuffer, vector<PlayerRef> snapshot, double enqueueTime, bool justEnqueue)
{
    auto room = GetRoom();
    if (room == nullptr)
        return;

    if (justEnqueue == true)
    {
        for (auto& p : snapshot)
        {
            if (p == nullptr)
                continue;
            if (auto session = p->GetSession())
                session->Send(sendBuffer, true);
        }
    }
    else
    {
        for (auto& p : snapshot)
        {
            if (p == nullptr)
                continue;
            if (auto session = p->GetSession())
                session->Send(sendBuffer);
        }
    }
}
