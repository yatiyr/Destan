#pragma once
#include <core/memory/destan_memory_manager.h>
#include <vector>
#include <atomic>
#include <mutex>

namespace destan::core
{
    class Slab_Allocator
    {
    public:
        Slab_Allocator(u64 object_size, u64 objects_per_slab);
        ~Slab_Allocator();

        void* Allocate();
        void Free(void* ptr);
        void Reset();

    private:
        enum class Slab_State
        {
            FULL,
            PARTIAL,
            EMPTY
        };

        struct Slab
        {
            void* memory;
            std::atomic<void*> free_list;
            std::atomic<u64> free_count;
            std::atomic<Slab_State> state;
            Slab* next;
        };

        void Allocate_Slab();
        void Destroy_Slab(Slab* slab);
        Slab* Find_Available_Slab();

        std::atomic<Slab*> m_partial_slab_list;
        std::atomic<Slab*> m_full_slab_list;
        std::atomic<Slab*> m_empty_slab_list;

        u64 m_object_size;
        u64 m_objects_per_slab;
        u64 m_slab_size;
        std::mutex m_mutex;
    };
}