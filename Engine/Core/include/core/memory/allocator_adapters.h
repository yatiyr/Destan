#pragma once
#include <core/memory/allocator_interface.h>

namespace ds::core::memory
{
    // Default Allocator Adapter - Uses the core Memory system
    template<typename T>
    class Default_Allocator : public Allocator_Interface<T>
    {
    public:
        Default_Allocator() = default;

        T* allocate(ds_u64 n) override
        {
            return static_cast<T*>(Memory::Malloc(sizeof(T) * n, alignof(T)));
        }

        void deallocate(T* p, ds_u64 n) override
        {
            Memory::Free(p);
        }
    };

    // Arena Allocator Adapter
    template<typename T>
    class Arena_Allocator_Adapter : public Allocator_Interface<T>
    {
    private:
        Arena_Allocator& m_allocator;

    public:
        Arena_Allocator_Adapter(Arena_Allocator& allocator) : m_allocator(allocator) {}

        T* allocate(ds_u64 n) override
        {
            return static_cast<T*>(m_allocator.Allocate(sizeof(T) * n, alignof(T)));
        }

        void deallocate(T* p, ds_u64 n) override
        {
            // Arena allocator doesn't support individual deallocations
            // No operation needed
        }
    };

    // Pool Allocator Adapter
    template<typename T>
    class Pool_Allocator_Adapter : public Allocator_Interface<T>
    {
    private:
        Pool_Allocator& m_allocator;

    public:
        Pool_Allocator_Adapter(Pool_Allocator& allocator) : m_allocator(allocator)
        {
            // Verify that pool block size is sufficient for type T
            DS_ASSERT(allocator.Get_Block_Size() >= sizeof(T),
                "Pool block size too small for type T");
            DS_ASSERT(allocator.Get_Block_Alignment() >= alignof(T),
                "Pool block alignment insufficient for type T");
        }

        T* allocate(ds_u64 n) override
        {
            // Pool allocator only supports single object allocation
            DS_ASSERT(n == 1, "Pool allocator only supports single object allocation");
            return static_cast<T*>(m_allocator.Allocate());
        }

        void deallocate(T* p, ds_u64 n) override
        {
            m_allocator.Deallocate(p);
        }
    };

    // Stack Allocator Adapter
    template<typename T>
    class Stack_Allocator_Adapter : public Allocator_Interface<T>
    {
    private:
        Stack_Allocator& m_allocator;
        struct Allocation_Info
        {
            T* ptr;
            ds_u64 count;
        };
        std::vector<Allocation_Info> m_allocations; // Tracks allocations for LIFO enforcement

    public:
        Stack_Allocator_Adapter(Stack_Allocator& allocator) : m_allocator(allocator) {}

        T* allocate(ds_u64 n) override
        {
            T* result = static_cast<T*>(m_allocator.Allocate(sizeof(T) * n, alignof(T)));

            // Track this allocation
            m_allocations.push_back({ result, n });

            return result;
        }

        void deallocate(T* p, ds_u64 n) override
        {
            // Ensure LIFO ordering is maintained
            if (m_allocations.empty() || m_allocations.back().ptr != p)
            {
                DS_LOG_ERROR("Stack allocator deallocations must follow LIFO ordering");
                return;
            }

            // Pop the allocation
            m_allocations.pop_back();

            // Get the current marker
            Stack_Allocator::Marker marker = m_allocator.Get_Marker();

            // Free to marker (no specific marker, just indicates "free latest")
            m_allocator.Free_Latest();
        }
    };

    // Free List Allocator Adapter
    template<typename T>
    class Free_List_Allocator_Adapter : public Allocator_Interface<T>
    {
    private:
        Free_List_Allocator& m_allocator;

    public:
        Free_List_Allocator_Adapter(Free_List_Allocator& allocator) : m_allocator(allocator) {}

        T* allocate(ds_u64 n) override
        {
            return static_cast<T*>(m_allocator.Allocate(sizeof(T) * n, alignof(T)));
        }

        void deallocate(T* p, ds_u64 n) override
        {
            m_allocator.Deallocate(p);
        }
    };

    // Page Allocator Adapter
    template<typename T>
    class Page_Allocator_Adapter : public Allocator_Interface<T>
    {
    private:
        Page_Allocator& m_allocator;
        Page_Protection m_protection;
        Page_Flags m_flags;

    public:
        Page_Allocator_Adapter(
            Page_Allocator& allocator,
            Page_Protection protection = Page_Protection::READ_WRITE,
            Page_Flags flags = Page_Flags::COMMIT | Page_Flags::ZERO
        ) :
            m_allocator(allocator),
            m_protection(protection),
            m_flags(flags)
        {
        }

        T* allocate(ds_u64 n) override
        {
            // Page allocators typically work with larger blocks
            // We'll round up to the nearest page size if necessary
            ds_u64 size = sizeof(T) * n;
            ds_u64 page_size = m_allocator.Get_Page_Size();

            if (size < page_size)
            {
                size = page_size;
            }
            else
            {
                // Round up to nearest page
                size = ((size + page_size - 1) / page_size) * page_size;
            }

            return static_cast<T*>(m_allocator.Allocate(size, m_protection, m_flags));
        }

        void deallocate(T* p, ds_u64 n) override
        {
            m_allocator.Deallocate(p);
        }
    };

    // Streaming Allocator Adapter
    template<typename T>
    class Streaming_Allocator_Adapter : public Allocator_Interface<T>
    {
    private:
        Streaming_Allocator& m_allocator;
        Resource_Category m_category;
        std::unordered_map<T*, Resource_Handle> m_handles;

    public:
        Streaming_Allocator_Adapter(
            Streaming_Allocator& allocator,
            Resource_Category category = Resource_Category::GENERIC
        ) :
            m_allocator(allocator),
            m_category(category)
        {
        }

        T* allocate(ds_u64 n) override
        {
            // Create a unique resource path for this allocation
            static ds_u64 next_id = 1;
            std::string path = "memory://" + std::to_string(next_id++);

            // Request the resource
            Resource_Request request;
            request.resource_id = 0; // Generate a new ID
            request.path = path.c_str();
            request.category = m_category;
            request.priority = Resource_Priority::HIGH; // High priority for allocator requests
            request.access_mode = Access_Mode::READ_WRITE;
            request.callback = nullptr;
            request.user_data = nullptr;
            request.auto_unload = true;
            request.estimated_size = sizeof(T) * n;

            Resource_Handle handle = m_allocator.Request_Resource(request);

            // Wait for the resource to be loaded (not ideal in production, but necessary for this adapter)
            while (handle.state != Resource_State::RESIDENT)
            {
                m_allocator.Update(0.016f); // Update with 16ms
                handle.state = m_allocator.Get_Resource_Info(handle)->state;
            }

            // Access the resource
            T* data = static_cast<T*>(m_allocator.Access_Resource(handle));

            // Store the handle for deallocation
            m_handles[data] = handle;

            return data;
        }

        void deallocate(T* p, ds_u64 n) override
        {
            auto it = m_handles.find(p);
            if (it != m_handles.end())
            {
                // Release the resource
                m_allocator.Release_Resource(it->second);
                m_allocator.Unload_Resource(it->second);
                m_handles.erase(it);
            }
        }
    };
}