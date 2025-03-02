#include <core/destan_pch.h>
#include <core/memory/destan_slab_allocator.h>

namespace destan::core
{
    Slab_Allocator::Slab_Allocator(u64 object_size, u64 objects_per_slab)
    {
        m_object_size = ALIGN_UP(object_size, alignof(std::max_align_t));
        m_objects_per_slab = objects_per_slab;
        m_slab_size = m_object_size * m_objects_per_slab;

        m_partial_slab_list.store(nullptr, std::memory_order_relaxed);
        m_full_slab_list.store(nullptr, std::memory_order_relaxed);
        m_empty_slab_list.store(nullptr, std::memory_order_relaxed);

        Allocate_Slab();
    }

    Slab_Allocator::~Slab_Allocator()
    {
        Slab* slab = m_partial_slab_list.load();
        while (slab)
        {
            Slab* next = slab->next;
            Destroy_Slab(slab);
            slab = next;
        }
    }


    void Slab_Allocator::Allocate_Slab()
    {
        Slab* new_slab = static_cast<Slab*>(Memory_Manager::Allocate(sizeof(Slab), "Slab Metadata"));
        new_slab->memory = Memory_Manager::Allocate(m_slab_size, "Slab Data");
        new_slab->free_count.store(m_objects_per_slab, std::memory_order_relaxed);
        new_slab->state.store(Slab_State::PARTIAL, std::memory_order_relaxed);
        new_slab->next = nullptr;

        // Initialize Free List
        void* first_block = new_slab->memory;
        char* current = static_cast<char*>(first_block);
        for (u64 i = 0; i < m_objects_per_slab - 1; i++)
        {
            *reinterpret_cast<void**>(current) = current + m_object_size;
            current += m_object_size;
        }
        *reinterpret_cast<void**>(current) = nullptr;

        new_slab->free_list.store(first_block, std::memory_order_release);

        // Add to partial slab list
        do
        {
            new_slab->next = m_partial_slab_list.load(std::memory_order_acquire);
        } while (!m_partial_slab_list.compare_exchange_weak(new_slab->next, new_slab, std::memory_order_release));
    }

    void* Slab_Allocator::Allocate()
    {
        Slab* slab = Find_Available_Slab();
        if (!slab)
        {
            Allocate_Slab();
            slab = Find_Available_Slab();
        }

        void* allocated_block;
        do
        {
            allocated_block = slab->free_list.load(std::memory_order_acquire);
            if (!allocated_block)
            {
                return nullptr;
            }
        } while (!slab->free_list.compare_exchange_weak(allocated_block, *reinterpret_cast<void**>(allocated_block), std::memory_order_release));

        slab->free_count.fetch_sub(1, std::memory_order_relaxed);

        if (slab->free_count.load() == 0)
        {
            slab->state.store(Slab_State::FULL, std::memory_order_release);
        }

        return allocated_block;
    }

    Slab_Allocator::Slab* Slab_Allocator::Find_Available_Slab()
    {
        Slab* slab = m_partial_slab_list.load(std::memory_order_acquire);
        while (slab)
        {
            if (slab->free_count.load(std::memory_order_relaxed) > 0)
            {
                return slab;
            }
            slab = slab->next;
        }
        return nullptr; // No available slab found
    }

    void Slab_Allocator::Reset()
    {
        Slab* slab = m_partial_slab_list.load();
        while (slab)
        {
            Slab* next = slab->next;
            Destroy_Slab(slab);
            slab = next;
        }
        m_partial_slab_list.store(nullptr, std::memory_order_release);
    }

    void Slab_Allocator::Destroy_Slab(Slab* slab)
    {
        if (!slab)
        {
            return;
        }

        Memory_Manager::Deallocate(slab->memory);
        Memory_Manager::Deallocate(slab);
    }

    void Slab_Allocator::Free(void* ptr)
    {
        if (!ptr)
        {
            return;
        }

        Slab* slab = m_partial_slab_list.load(std::memory_order_acquire);
        while (slab)
        {
            if (ptr >= slab->memory && ptr < static_cast<char*>(slab->memory) + m_slab_size)
            {
                void* prev_head;
                do
                {
                    prev_head = slab->free_list.load(std::memory_order_acquire);
                    *reinterpret_cast<void**>(ptr) = prev_head;
                } while (!slab->free_list.compare_exchange_weak(prev_head, ptr, std::memory_order_release));

                slab->free_count.fetch_add(1, std::memory_order_relaxed);

                if (slab->free_count.load() == m_objects_per_slab)
                {
                    slab->state.store(Slab_State::EMPTY, std::memory_order_release);
                }

                return;
            }
            slab = slab->next;
        }
    }
}