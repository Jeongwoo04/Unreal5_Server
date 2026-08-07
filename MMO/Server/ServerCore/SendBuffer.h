#pragma once
#include "JobQueue.h"
#define USE_OPTIMIZED_SENDBUFFER_CHUNK
/*----------------
	SendBuffer
-----------------*/

class SendBufferChunk;

#ifdef USE_OPTIMIZED_SENDBUFFER_CHUNK
class SendBuffer
{
public:
	SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 size);
	~SendBuffer();

	BYTE* Buffer() { return _buffer; }
	int32 WriteSize() { return _writeSize; }
	void Close(uint32 writeSize);

	void SetReserveTime(uint64 tick) { _reserveTime = tick; }
	uint64 GetReserveTime() { return _reserveTime; }

private:
	BYTE* _buffer;
	uint32 _allocSize = 0;
	uint32 _writeSize = 0;
	uint64 _reserveTime = 0;
	
	// Async IO 완료 전 Chunk 메모리 반납이 되지 않도록 참조.
	SendBufferChunkRef _owner;
};

class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
	enum SENDBUFFER_CHUNK_SIZE
	{
		LARGE_SENDBUFFER_CHUNK_SIZE = 0x100000,
		SENDBUFFER_CHUNK_SIZE = 0x10000
	};

public:
	SendBufferChunk();
	~SendBufferChunk();

	void Reset();
	SendBufferRef Open(uint32 allocSize);
	void Close(uint32 writeSize);

	BYTE* Buffer() { return &_buffer[_usedSize]; }
	uint32 FreeSize() { return static_cast<uint32>(_buffer.size()) - _usedSize; }
	bool IsOpen() { return _isOpen; }

private:
	array<BYTE, LARGE_SENDBUFFER_CHUNK_SIZE> _buffer = {};
	uint32 _usedSize = 0;
	bool _isOpen = false;
};

class SendBufferManager
{
public:
	~SendBufferManager();

	SendBufferRef Open(uint32 size);

private:
	SendBufferChunkRef Pop();
	void Push(SendBufferChunk* chunk);

	static void PushGlobal(SendBufferChunk* buffer);

private:
	USE_LOCK;
	vector<SendBufferChunk*> _sendBufferChunks;
};
#else
class SendBuffer : enable_shared_from_this<SendBuffer>
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

	BYTE* Buffer() { return _buffer.data(); }
	int32 WriteSize() { return _writeSize; }
	int32 Capacity() { return static_cast<int32>(_buffer.size()); }

	void SetReserveTime(uint64 tick) { _reserveTime = tick; }
	uint64 GetReserveTime() { return _reserveTime; }

	void CopyData(void* data, int32 len);
	void Close(uint32 writeSize);

private:
	vector<BYTE>	_buffer;
	int32			_writeSize = 0;
	uint64			_reserveTime = 0;
};
#endif