using System;
using System.Collections.Generic;
using System.Text;
using System.Buffers; // 닷넷 내부 공유 메모리 풀 사용해 Buffer 빌려오기

namespace ServerCore
{
	public class RecvBuffer
	{
		// [r][][w][][][][][][][]
		ArraySegment<byte> _buffer;
		byte[] _sharedArray;
		int _readPos;
		int _writePos;

		public RecvBuffer(int bufferSize) // 파편화 방지 및 Large Object Heap
		{
			_sharedArray = ArrayPool<byte>.Shared.Rent(bufferSize); // Close 또는 연결 종료시 반납 필수
			_buffer = new ArraySegment<byte>(_sharedArray, 0, bufferSize);
		}

		public int DataSize { get { return _writePos - _readPos; } }
		public int FreeSize { get { return _buffer.Count - _writePos; } }

		public ArraySegment<byte> ReadSegment
		{
			get { return new ArraySegment<byte>(_buffer.Array, _buffer.Offset + _readPos, DataSize); }
		}

		public ArraySegment<byte> WriteSegment
		{
			get { return new ArraySegment<byte>(_buffer.Array, _buffer.Offset + _writePos, FreeSize); }
		}

		public void Clean()
		{
			int dataSize = DataSize;
			if (dataSize == 0)
			{
				// 남은 데이터가 없으면 복사하지 않고 커서 위치만 리셋
				_readPos = _writePos = 0;
			}
			else
			{
				// 남은 찌끄레기가 있으면 시작 위치로 복사
				Array.Copy(_buffer.Array, _buffer.Offset + _readPos, _buffer.Array, _buffer.Offset, dataSize);
				_readPos = 0;
				_writePos = dataSize;
			}
		}

		public bool OnRead(int numOfBytes)
		{
			if (numOfBytes > DataSize)
				return false;

			_readPos += numOfBytes;
			return true;
		}

		public bool OnWrite(int numOfBytes)
		{
			if (numOfBytes > FreeSize)
				return false;

			_writePos += numOfBytes;
			return true;
		}

		public void Close()
		{
			if (_sharedArray != null)
			{
				ArrayPool<byte>.Shared.Return(_sharedArray);
				_sharedArray = null;
			}
		}
	}
}
