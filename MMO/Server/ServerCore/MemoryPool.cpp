#include "pch.h"
#include "MemoryPool.h"


MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
    ::InitializeSListHead(&_header);

    if (allocSize <= 1024)
        _maxReserveCount = 10000; // 1KB 이하: 10,000개 (최대 약 10MB)
    else if (allocSize <= 2048)
        _maxReserveCount = 5000;  // 2KB 이하: 5,000개 (최대 약 10MB)
    else
        _maxReserveCount = 2000;  // 4KB 이하: 2,000개 (최대 약 8MB)
}

MemoryPool::~MemoryPool()
{
    while (MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header)))
    {
        ::_aligned_free(memory);
    }
}

void MemoryPool::Push(MemoryHeader* ptr)
{
    _useCount.fetch_sub(1);

    if (_reserveCount.load() >= _maxReserveCount)
    {
        ::_aligned_free(ptr);
        return;
    }

    ::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));

    _reserveCount.fetch_add(1);
}

MemoryHeader* MemoryPool::Pop()
{
    MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header));

    if (memory == nullptr)
    {
        memory = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, MEMORY_ALIGNMENT));
    }
    else
    {
        _reserveCount.fetch_sub(1);
    }

    _useCount.fetch_add(1);

    return memory;
}