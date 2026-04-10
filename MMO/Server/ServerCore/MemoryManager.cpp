#include "pch.h"
#include "MemoryManager.h"

void* MyAllocator::Alloc(uint32 size)
{
    return ::malloc(size);
}

void MyAllocator::Release(void* ptr)
{
    ::free(ptr);
}

void* PoolAllocator::Alloc(int32 size)
{
    return GMemoryManager->Alloc(size);
}

void PoolAllocator::Release(void* ptr)
{
    GMemoryManager->Release(ptr);
}

MemoryManager::MemoryManager()
{
    int32 size = 0;
    int32 tableIndex = 0;

    for (size = 32; size <= 1024; size += 32)
    {
        MemoryPool* pool = new MemoryPool(size);
        _pools.push_back(pool);

        while (tableIndex <= size)
        {
            _poolTable[tableIndex] = pool;
            tableIndex++;
        }
    }
    for (; size <= 2048; size += 128)
    {
        MemoryPool* pool = new MemoryPool(size);
        _pools.push_back(pool);

        while (tableIndex <= size)
        {
            _poolTable[tableIndex] = pool;
            tableIndex++;
        }
    }
    for (; size <= 4096; size += 256)
    {
        MemoryPool* pool = new MemoryPool(size);
        _pools.push_back(pool);

        while (tableIndex <= size)
        {
            _poolTable[tableIndex] = pool;
            tableIndex++;
        }
    }
}

MemoryManager::~MemoryManager()
{
    for (MemoryPool* pool : _pools)
        delete pool;

    _pools.clear();
}

void* MemoryManager::Alloc(uint32 size)
{
    MemoryHeader* header = nullptr;
    const int32 allocSize = size + sizeof(MemoryHeader);

    if (allocSize > MAX_ALLOC_SIZE)
    {
        header = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(allocSize, MEMORY_ALIGNMENT));
    }
    else
    {
        header = _poolTable[allocSize]->Pop();
    }

    return MemoryHeader::AttachHeader(header, allocSize);
}

void MemoryManager::Release(void* ptr)
{
    MemoryHeader* header = MemoryHeader::DetachHeader(ptr);

    const int32 allocSize = header->_allocSize;

    if (allocSize > MAX_ALLOC_SIZE)
    {
        ::_aligned_free(header);
    }
    else
    {
        _poolTable[allocSize]->Push(header);
    }
}

// LocalChunk

LocalChunk::LocalChunk()
{
    _basePtr = ::_aligned_malloc(CHUNK_SIZE, 64);
}

LocalChunk::~LocalChunk()
{
    cout << "Chunk free !" << endl;
    ::_aligned_free(_basePtr);
}

void LocalChunk::Init(const size_t typeSize, const size_t alignment, UpdateFunc func)
{
    //uint32 count = (typeSize - 1 + MEMORY_ALIGNMENT) / MEMORY_ALIGNMENT;
    //uint32 alignSize = MEMORY_ALIGNMENT * count;
    _updateFunc = func;

    _alignSize = (typeSize + alignment - 1) & ~(alignment - 1);

    _header = reinterpret_cast<void**>(_basePtr);
    void** current = _header;
    uint32 offset = 0;

    while (offset + (_alignSize * 2) <= CHUNK_SIZE)
    {
        uint32 nextOffset = offset + static_cast<uint32>(_alignSize);

        *current = reinterpret_cast<void*>(static_cast<BYTE*>(_basePtr) + nextOffset);
        current = reinterpret_cast<void**>(*current);

        offset = nextOffset;
    }

    *current = nullptr;
    ::memset(_bitmap, 0, sizeof(_bitmap));
    _activeCount = 0;
}

// ChunkList

LocalChunkRef ChunkList::FindChunk()
{
    LocalChunkRef find = nullptr;
    uint32 maxCount = 0;

    for (auto& chunk : _chunks)
    {
        if (chunk == nullptr || chunk->IsFull())
            continue;

        uint32 currentCount = chunk->GetActiveCount();

        if (currentCount > maxCount) {
            maxCount = currentCount;
            find = chunk;
        }
    }

    return find;
}

int32 ChunkList::AddChunk()
{
    LocalChunkRef newChunk = GLocalMemoryManager->PopChunk(_type);
    newChunk->SetOwner(shared_from_this());

    int32 index;

    if (_emptyIndex.empty() == false)
    {
        index = _emptyIndex.top();
        _emptyIndex.pop();
        newChunk->SetIndex(index);
        _chunks[index] = newChunk;
    }
    else
    {
        index = static_cast<int32>(_chunks.size());
        newChunk->SetIndex(index);
        _chunks.push_back(newChunk);
    }

    return index;
}

void ChunkList::ChunkEmpty(int32 index)
{
    _chunks[index] = nullptr;
    _emptyIndex.push(index);
}

// LocalMemoryManager

LocalMemoryManager::LocalMemoryManager()
{
    InitializeSListHead(&_sListHeader);
}

LocalMemoryManager::~LocalMemoryManager()
{
    while (MemoryHeader* header = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_sListHeader)))
    {
        void* ptr = MemoryHeader::AttachHeader(header, header->_allocSize);
        S1_Delete<LocalChunk>(static_cast<LocalChunk*>(ptr));
    }
}

LocalChunkRef LocalMemoryManager::PopChunk(const string& type)
{
    MemoryHeader* header = static_cast<MemoryHeader*>(InterlockedPopEntrySList(&_sListHeader));

    if (header) {
        void* ptr = MemoryHeader::AttachHeader(header, header->_allocSize);
    
        _reserveCount.fetch_sub(1);
        _useCount.fetch_add(1);

        //cout << "Pop Chunk " << type << " ! remain " << _reserveCount.load() << endl;

        return LocalChunkRef(static_cast<LocalChunk*>(ptr), PushGlobal);
    }

    //cout << "Pop Chunk " << type << " ! remain " << _reserveCount.load() << endl;

    _useCount.fetch_add(1);
    return LocalChunkRef(S1_New<LocalChunk>(), PushGlobal);

    //{
    //    WRITE_LOCK;
    //    if (_localChunks.empty() == false)
    //    {
    //        LocalChunk* localChunk = _localChunks.back();
    //        _localChunks.pop_back();

    //        cout << "Pop Chunk ! remain " << GetSize() << endl;

    //        return LocalChunkRef(localChunk, PushGlobal);
    //    }
    //}

    //return LocalChunkRef(S1_New<LocalChunk>(), PushGlobal);
}

void LocalMemoryManager::PushChunk(LocalChunk* chunk)
{
    if (_reserveCount.load() >= MAX_CHUNK_LIMIT)
    {
        S1_Delete<LocalChunk>(chunk);
        _useCount.fetch_sub(1);

        return;
    }

    MemoryHeader* header = MemoryHeader::DetachHeader(chunk);

    InterlockedPushEntrySList(&_sListHeader, header);

    _useCount.fetch_sub(1);
    _reserveCount.fetch_add(1);

    //WRITE_LOCK;
    //_localChunks.push_back(chunk);
    //cout << "Push Chunk ! remain " << _reserveCount.load() << endl;
}

void LocalMemoryManager::PushGlobal(LocalChunk* buffer)
{
    //cout << buffer->GetIndex() << " Chunk PushGlobal !" << endl;
    GLocalMemoryManager->PushChunk(buffer);
}