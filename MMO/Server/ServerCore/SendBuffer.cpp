#include "pch.h"
#include "SendBuffer.h"

/*----------------
	SendBuffer
-----------------*/

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
	: _owner(owner), _buffer(buffer), _allocSize(allocSize)
{
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(uint32 writeSize)
{
	ASSERT_CRASH(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}

SendBufferChunk::SendBufferChunk()
{
}

SendBufferChunk::~SendBufferChunk()
{
}

void SendBufferChunk::Reset()
{
	_usedSize = 0;
	_isOpen = false;
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize)
{
	ASSERT_CRASH(allocSize <= LARGE_SENDBUFFER_CHUNK_SIZE);
	ASSERT_CRASH(_isOpen == false);

	if (allocSize > FreeSize())
		return nullptr;

	_isOpen = true;

	return ObjectPool<SendBuffer>::MakeShared(shared_from_this(), Buffer(), allocSize);
}

void SendBufferChunk::Close(uint32 writeSize)
{
	ASSERT_CRASH(_isOpen == true);
	_isOpen = false;
	_usedSize += writeSize;
}

SendBufferManager::~SendBufferManager()
{
	if (_sendBufferChunks.empty() == false)
	{
		for (SendBufferChunk* chunk : _sendBufferChunks)
			S1_Delete<SendBufferChunk>(chunk);
	}

	_sendBufferChunks.clear();
}

SendBufferRef SendBufferManager::Open(uint32 size)
{
	if (LSendBufferChunk == nullptr)
	{
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

	if (LSendBufferChunk->FreeSize() < size)
	{
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	return LSendBufferChunk->Open(size);
}

SendBufferChunkRef SendBufferManager::Pop()
{
	{
		WRITE_LOCK;
		if (_sendBufferChunks.empty() == false)
		{
			SendBufferChunk* sendBufferChunk = _sendBufferChunks.back();
			_sendBufferChunks.pop_back();

			return SendBufferChunkRef(sendBufferChunk, PushGlobal);
		}
	}

	return SendBufferChunkRef(S1_New<SendBufferChunk>(), PushGlobal);
}

void SendBufferManager::Push(SendBufferChunk* chunk)
{
	WRITE_LOCK;
	if (_sendBufferChunks.size() > 10)
		return;

	_sendBufferChunks.push_back(chunk);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
	GSendBufferManager->Push(buffer);
}