#pragma once

// Memory
/*
* 16바이트 정렬, 0x0...0 을 시작으로 데이터를 배치.
* 메모리 버스를 통해 효율적으로 데이터를 긁어옴
* ex) 0x0...3 을 시작으로 배치할경우 두번에 걸쳐서 가져오게됨.
*/
enum
{
    MEMORY_ALIGNMENT = 16
};

DECLSPEC_ALIGN(MEMORY_ALIGNMENT)
struct MemoryHeader : public SLIST_ENTRY
{
    MemoryHeader(int32 allocSize) : _allocSize(allocSize) { }

    static void* AttachHeader(MemoryHeader* header, int32 size)
    {
        new(header)MemoryHeader(size);

        // header 위치 다음의 data offset을 가리키도록
        return reinterpret_cast<void*>(++header);
    }

    static MemoryHeader* DetachHeader(void* ptr)
    {
        MemoryHeader* header = reinterpret_cast<MemoryHeader*>(ptr) - 1;

        return header;
    }

    // [0] SLIST_ENTRY
    int32 _allocSize;
};

DECLSPEC_ALIGN(MEMORY_ALIGNMENT)
class MemoryPool
{
public:
    MemoryPool(int32 allocSize);
    ~MemoryPool();

    void Push(MemoryHeader* ptr);
    MemoryHeader* Pop();

private:
    SLIST_HEADER _header;
    int32 _allocSize = 0;
    atomic<int32> _useCount = 0;
    atomic<int32> _reserveCount = 0;

    //TEMP
    int32 _maxReserveCount = 0;
};