using ServerCore;
using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;


namespace DummyClient
{
	class Program
	{
		static void Main(string[] args)
		{
			// DNS (Domain Name System)
			string host = "127.0.0.1";
			int port = 7777;

			IPAddress ipAddr = IPAddress.Parse(host);
			IPEndPoint endPoint = new IPEndPoint(ipAddr, port);

			Connector connector = new Connector();

			connector.Connect(endPoint,
				() => { return SessionManager.Instance.Generate(); },
				5000);

			while (true)
			{
				int sessionCount = SessionManager.Instance.GetSessionCount();

				if (sessionCount == 5000)
					break;

				Console.WriteLine("Connected Session Count = " + sessionCount);

				Thread.Sleep(1000);
			}

			while (true)
			{
				try
				{
					SessionManager.Instance.SendForEachMovePkt();
				}
				catch (Exception e)
				{
					Console.WriteLine(e.ToString());
				}

				Thread.Sleep(300);
			}
		}
	}
}
