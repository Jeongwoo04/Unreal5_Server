using DummyClient;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Text;

class PacketHandler
{
    public static void S_ENTER_GAMEHandler(PacketSession session, IMessage packet)
    {
        S_ENTER_GAME enterPkt = packet as S_ENTER_GAME;
        ServerSession serverSession = session as ServerSession;

        //Console.WriteLine($"C_Move ({movePacket.PosInfo.PosX}, {movePacket.PosInfo.PosY})");

		serverSession.MyPlayer = enterPkt.Object;
		session._enter = true;

        //GameRoom room = player.Room;
        //if (room == null)
        //    return;

        //room.Push(room.HandleMove, player, movePacket);
    }
}
