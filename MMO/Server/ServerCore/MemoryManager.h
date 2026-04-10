#pragma once
#include "MemoryPool.h"

/*
* 할당/해제 오버헤드 & 외부 단편화를 줄이기 위한 MemoryManager 및 메모리 풀링
* cache locality 및 메모리 연속성 확보
*/

// Allocator
class MyAllocator
{
public:
    static void* Alloc(uint32 size);
    static void Release(void* ptr);
};

class PoolAllocator
{
public:
    static void* Alloc(int32 size);
    static void Release(void* ptr);
};

// Type 별 할당기 (placement new / delete 명시적 호출)
// 메모리 할당 후 메모리 위에 생성자 호출
template<typename Type, typename... Args>
Type* S1_New(Args&&... args)
{
    Type* memory = static_cast<Type*>(PoolAllocator::Alloc(sizeof(Type)));
    new(memory)Type(std::forward<Args>(args)...);

    return memory;
}

template<typename Type>
void S1_Delete(Type* ptr)
{
    ptr->~Type();
    PoolAllocator::Release(ptr);
}

template<typename Type, typename... Args>
shared_ptr<Type> MakeShared(Args&&... args)
{
    return shared_ptr<Type> { S1_New<Type>(forward<Args>(args)...), S1_Delete<Type> };
}

// 메모리 매니저

class MemoryManager
{
    enum
    {
        // 사이즈 별 풀 크기
        POOL_COUNT = (1024 / 32) + (1024 / 128) + (2048 / 256),
        MAX_ALLOC_SIZE = 4096
    };

public:
    MemoryManager();
    ~MemoryManager();

    void* Alloc(uint32 size);
    void Release(void* ptr);

private:
    vector<MemoryPool*> _pools;
    MemoryPool* _poolTable[MAX_ALLOC_SIZE + 1];

};

class ChunkList;

template<typename Type>
void UpdateHelper(void* ptr)
{
    static_cast<Type*>(ptr)->Update();
}

using UpdateFunc = void(*)(void*);

class LocalChunk : public enable_shared_from_this<LocalChunk>
{
    enum LOCAL_CHUNK_SIZE
    {
        CHUNK_SIZE = 0x10000
    };

public:
    LocalChunk();
    ~LocalChunk();

    void Init(const size_t typeSize, const size_t alignment, UpdateFunc func);
    bool IsFull() { return _header == nullptr; }
    uint32 GetActiveCount() { return _activeCount; }
    void SetOwner(ChunkListRef owner) { _owner = owner; }
    int32 GetIndex() { return _index; }
    void SetIndex(int32 index) { _index = index; }

    void Update() {
        if (_activeCount == 0)
            return;

        for (uint32 i = 0; i < BITMAP_SIZE; ++i)
        {
            uint64 mask = _bitmap[i];
            if (mask == 0)
                continue;

            while (mask > 0)
            {
                unsigned long relativeIdx;

                if (_BitScanForward64(&relativeIdx, mask))
                {
                    uint32 absoluteIdx = (i * 64) + relativeIdx;

                    void* ptr = static_cast<BYTE*>(_basePtr) + (absoluteIdx * _alignSize);

                    if (_updateFunc)
                        _updateFunc(ptr);

                    mask &= (~1ULL << relativeIdx);
                }
            }
        }
    }

    template<typename Type, typename... Args>
    Type* Alloc(Args&&... args) {
        void* ptr = _header;
        if (ptr == nullptr)
            return nullptr;
        
        ptrdiff_t offset = static_cast<BYTE*>(ptr) - static_cast<BYTE*>(_basePtr);
        uint32 idx = static_cast<uint32>(offset / _alignSize);
        _bitmap[idx / 64] |= (1ULL << (idx % 64));

        // _header 역참조로 다음 체인 연결
        _header = static_cast<void**>(*_header);

        ++_activeCount;

        return new(ptr)Type(std::forward<Args>(args)...);
    }
    
    template<typename Type>
    void Release(Type* ptr){
        ptrdiff_t offset = reinterpret_cast<BYTE*>(ptr) - static_cast<BYTE*>(_basePtr);
        uint32 idx = static_cast<uint32>(offset / _alignSize);
        _bitmap[idx / 64] &= ~(1ULL << (idx % 64));

        ptr->~Type();

        // ptr 주소를 가리키는 next를 새로운 헤더로
        void** next = reinterpret_cast<void**>(ptr);
        *next = _header;
        _header = next;
        
        if (--_activeCount == 0) {
            if (auto owner = _owner.lock())
                _owner.lock()->ChunkEmpty(_index);
        }
    }

    template<typename Type, typename... Args>
    std::shared_ptr<Type> AllocShared(Args&&... args)
    {
        Type* ptr = this->Alloc<Type>(std::forward<Args>(args)...);
        if (ptr == nullptr)
            return nullptr;

        auto self = shared_from_this();

        return std::shared_ptr<Type>(ptr, [self](Type* p) {
            self->Release<Type>(p);
            });
    }

private:
    void* _basePtr = nullptr;
    uint32 _activeCount = 0;
    int32 _index;
    weak_ptr<ChunkList> _owner;
    void** _header = nullptr;
    static constexpr uint32 BITMAP_SIZE = 1024 / 64;
    uint64 _bitmap[BITMAP_SIZE] = { 0, };
    size_t _alignSize = 0;

    UpdateFunc _updateFunc = nullptr;
};

/*          ChunkList - [Chunk][Chunk]...[Chunk]
    Room 내부 로직은 직렬성이 유지된다.
    Chunk라는 큰 메모리 블록을 ::_aligned_malloc으로 떼어온다.
    커스텀 할당자와 딜리터를 통해서 Global Manager로 Chunk 재사용로직
    
    사용법
    Room이 소유한 각각의 ObjectManager Init에서
    object별 ChunkListRef 를 생성, 생성 팩토리 함수에 등록
    예) ObjectRef { return _playerPool->AllocShared<Player>(); }
    반환되는 shared_ptr로 Room 내부에서 사용
    객체가 RemoveList에 등록이 되면 Update 마지막에 ClearRemoveList로
    참조가 0이되면 자동으로 Release 호출 -> _activeCount == 0 이면
    Chunk 를 GlobalManager로 PushGlobal해서 반납
    Chunk 내부 void** _header를 통해 Type별 할당 위치를 포인터 연산으로
    빠른 초기화 및 추가 메모리 없이 접근 가능
*/

class ChunkList : public enable_shared_from_this<ChunkList>
{
public:
    void ChunkEmpty(int32 index);

public:
    ChunkList() {}
    ChunkList(const string& type):_type(type) { }

    void Update() {
        for (auto& chunk : _chunks)
        {
            if (chunk == nullptr)
                continue;

            chunk->Update();
        }
    }

    template<typename Type, typename... Args>
    Type* Alloc(Args&&... args) {
        
        LocalChunkRef target = FindChunk();

        if (target == nullptr)
        {
            AddChunk();
            target = _chunks.back();
            target->Init(sizeof(Type), alignof(Type), [](void* p) {
                static_cast<Type*>(p)->Update(); });
        }

        return target->Alloc(std::forward<Args>(args)...);
    }

    template<typename Type, typename... Args>
    std::shared_ptr<Type> AllocShared(Args&&... args)
    {
        LocalChunkRef target = FindChunk();

        if (target == nullptr)
        {
            int32 index = AddChunk();
            target = _chunks[index];
            target->Init(sizeof(Type), alignof(Type), [](void* p) {
                static_cast<Type*>(p)->Update(); });
        }

        //Type* ptr = this->Alloc<Type>(std::forward<Args>(args)...);
        //if (ptr == nullptr)
        //    return nullptr;

        //auto self = shared_from_this();

        return target->template AllocShared<Type>(std::forward<Args>(args)...);
    }

    LocalChunkRef FindChunk();
    int32 AddChunk();

    pair<int32, int32> CheckList() {
        int32 chunkCount = 0;
        int32 objectCount = 0;
        for (auto& chunk : _chunks)
        {
            if (chunk == nullptr)
                continue;
            chunkCount++;
            objectCount += chunk->GetActiveCount();
        }

        return {chunkCount, objectCount};
    }

public:
    vector<LocalChunkRef> _chunks;
    stack<int32> _emptyIndex;

    //TEMP
    string _type = "";
};

class LocalMemoryManager
{
    enum {
        MAX_CHUNK_LIMIT = 10
    };

public:
    LocalMemoryManager();
    ~LocalMemoryManager();

public:
    LocalChunkRef PopChunk(const string& type);
    void PushChunk(LocalChunk* chunk);

    static void PushGlobal(LocalChunk* buffer);

private:
    DECLSPEC_ALIGN(16) SLIST_HEADER _sListHeader;
    atomic<int32> _useCount = 0;
    atomic<int32> _reserveCount = 0;
};