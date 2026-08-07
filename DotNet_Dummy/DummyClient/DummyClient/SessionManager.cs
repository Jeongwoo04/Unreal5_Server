using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Text;
using Google.Protobuf.Protocol;

namespace DummyClient
{
    class SessionManager
    {
        static SessionManager _session = new SessionManager();
        public static SessionManager Instance { get { return _session; } }

        List<ServerSession> _sessions = new List<ServerSession>();
        object _lock = new object();
		Random _rand = new Random();

		public int GetSessionCount() { return _sessions.Count; }

        public void SendForEachMovePkt()
        {
			foreach (ServerSession session in _sessions)
			{
				if (session.MyPlayer.PosInfo == null) continue ;

				float dx = (_rand.Next(0, 3) - 1) * 100.0f;
				float dy = (_rand.Next(0, 3) - 1) * 100.0f;

				var pos = session.MyPlayer.PosInfo;

				if (pos.X + dx > 4000.0f || pos.X + dx < -4000.0f) continue;
				if (pos.Y + dy > 4000.0f || pos.Y + dy < -4000.0f) continue;

				pos.X += dx;
				pos.Y += dy;

				C_MOVE movePkt = new C_MOVE();
				movePkt.Info = pos;

				session.Send(movePkt);
			}
        }

        public ServerSession Generate()
        {
            lock (_lock)
            {
                ServerSession session = new ServerSession();
                _sessions.Add(session);
                return session;
            }
        }
    }
}
