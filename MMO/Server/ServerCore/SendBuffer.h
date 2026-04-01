#pragma once

/*----------------
	SendBuffer
-----------------*/

class SendBufferChunk;

class SendBuffer
{
public:
	SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 size);
	~SendBuffer();

	BYTE* Buffer() { return _buffer; }
	int32 WriteSize() { return _writeSize; }
	void Close(uint32 writeSize);

private:
	BYTE* _buffer;
	uint32 _allocSize = 0;
	uint32 _writeSize = 0;
	
	// Async IO 완료 전 Chunk 메모리 반납이 되지 않도록 참조해야한다.
	SendBufferChunkRef _owner;
};

class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
	enum SENDBUFFER_CHUNK_SIZE
	{
		LARGE_SENDBUFFER_CHUNK_SIZE = 0x100000,
		SMALL_SENDBUFFER_CHUNK_SIZE = 0x10000
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