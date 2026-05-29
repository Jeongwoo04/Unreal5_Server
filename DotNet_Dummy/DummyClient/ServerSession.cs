using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using ServerCore;
using Google.Protobuf.Protocol;
using Google.Protobuf;

namespace DummyClient
{
    class ServerSession : PacketSession
    {

		public void Send(IMessage packet)
		{
			string msgName = ("PKT_" + packet.Descriptor.Name).Replace("_", "");

			if (Enum.TryParse(typeof(MsgId), msgName, true, out object result))
			{
				MsgId msgId = (MsgId)result;

				ushort size = (ushort)packet.CalculateSize();
				byte[] sendBuffer = new byte[size + 4];

				Array.Copy(BitConverter.GetBytes((ushort)(size + 4)), 0, sendBuffer, 0, sizeof(ushort));
				Array.Copy(BitConverter.GetBytes((ushort)msgId), 0, sendBuffer, 2, sizeof(ushort));
				Array.Copy(packet.ToByteArray(), 0, sendBuffer, 4, size);

				Send(new ArraySegment<byte>(sendBuffer));
			}
			else
			{
				Console.WriteLine($"[Error] MsgId 매핑 실패: {msgName}");
			}
		}
		public ObjectInfo MyPlayer { get; set; } = new ObjectInfo();
        public override void OnConnected(EndPoint endPoint)
        {
            Console.WriteLine($"OnConnected : {endPoint}");

			C_ENTER_GAME enterGamePkt = new C_ENTER_GAME();
			enterGamePkt.PlayerIndex = 0;

			Send(enterGamePkt);
        }

        public override void OnDisconnected(EndPoint endPoint)
        {
            Console.WriteLine($"OnDisconnected : {endPoint}");
        }

        public override void OnRecvPacket(ArraySegment<byte> buffer)
        {
            PacketManager.Instance.OnRecvPacket(this, buffer);
        }

        public override void OnSend(int numOfBytes)
        {
            //Console.WriteLine($"Transferred bytes: {numOfBytes}");
        }

    }
}
