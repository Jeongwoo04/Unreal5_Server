#pragma once

class Room;
using RoomRef = shared_ptr<class Room>;

class SendQueue : public JobQueue
{
public:
    SendQueue(string name);

    void SendJob(SendBufferRef sendBuffer, vector<PlayerRef> snapshot, double enqueueTime);

    void SetRoom(RoomRef room) { _room = room; }
    RoomRef GetRoom() { return _room.lock(); }

private:
    weak_ptr<Room> _room;
};