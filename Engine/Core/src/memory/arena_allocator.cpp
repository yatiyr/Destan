#include <core/destan_pch.h>
#include <core/memory/arena_allocator.h>

namespace destan::core::memory
{

    Arena_Allocator::Arena_Allocator(destan_u64 size_bytes, const char* name)
        : m_size(size_bytes)
    {
        // Safely copy the name to our fixed buffer
        if (name)
        {
            destan_u64 name_length = 0;
            while (name[name_length] && name_length < MAX_NAME_LENGTH - 1)
            {
                m_name[name_length] = name[name_length];
                name_length++;
            }
            m_name[name_length] = '\0';
        }

        // Allocate the memory block using our core memory system
        m_memory_block = Memory::Malloc(size_bytes, CACHE_LINE_SIZE);

        // Initialize pointers
        m_start_pos = static_cast<destan_char*>(m_memory_block);
        m_current_pos = m_start_pos;
        m_end_pos = m_start_pos + size_bytes;

        // Initialize counters
        m_allocation_count = 0;

#ifdef DESTAN_DEBUG
        // Clear debug allocations
        m_debug_allocation_count = 0;

        // In debug mode, fill memory with a pattern to help identify uninitialized memory
        Memory::Memset(m_memory_block, 0xCD, size_bytes); // 0xCD = "Clean Dynamic memory"
        DESTAN_LOG_INFO("Arena allocator '{0}' created with {1} bytes", m_name, size_bytes);
#endif
    }

    Arena_Allocator::~Arena_Allocator()
    {
        // Free the entire memory block
        if (m_memory_block)
        {
#ifdef DESTAN_DEBUG
            // Check for memory leaks before freeing
            if (m_allocation_count > 0)
            {
                DESTAN_LOG_WARN("Arena '{0}' destroyed with {1} active allocations ({2} bytes)",
                    m_name, m_allocation_count, Get_Used_Size());
            }

            // Fill with pattern to catch use-after-free
            // 0xDD = "Dead Dynamic memory" - Microsoft's debug heap pattern for freed memory
            Memory::Memset(m_memory_block, 0xDD, m_size);
#endif
            Memory::Free(m_memory_block);
            m_memory_block = nullptr;
            m_start_pos = nullptr;
            m_current_pos = nullptr;
            m_end_pos = nullptr;
        }
    }

    Arena_Allocator::Arena_Allocator(Arena_Allocator&& other) noexcept
        : m_memory_block(other.m_memory_block)
        , m_start_pos(other.m_start_pos)
        , m_current_pos(other.m_current_pos)
        , m_end_pos(other.m_end_pos)
        , m_size(other.m_size)
        , m_allocation_count(other.m_allocation_count)
    {
        // Copy name
        Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

        // Transfer ownership, clear the source
        other.m_memory_block = nullptr;
        other.m_start_pos = nullptr;
        other.m_current_pos = nullptr;
        other.m_end_pos = nullptr;
        other.m_size = 0;
        other.m_allocation_count = 0;

#ifdef DESTAN_DEBUG
        // Move debug allocation info
        m_debug_allocation_count = other.m_debug_allocation_count;
        for (destan_u64 i = 0; i < m_debug_allocation_count; ++i)
        {
            m_debug_allocations[i] = other.m_debug_allocations[i];
        }
        other.m_debug_allocation_count = 0;
#endif
    }

    Arena_Allocator& Arena_Allocator::operator=(Arena_Allocator&& other) noexcept
    {
        if (this != &other)
        {
            // Free existing resources
            if (m_memory_block)
            {
                Memory::Free(m_memory_block);
            }

            // Transfer ownership
            m_memory_block = other.m_memory_block;
            m_start_pos = other.m_start_pos;
            m_current_pos = other.m_current_pos;
            m_end_pos = other.m_end_pos;
            m_size = other.m_size;
            m_allocation_count = other.m_allocation_count;

            // Copy name
            Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

            // Clear the source
            other.m_memory_block = nullptr;
            other.m_start_pos = nullptr;
            other.m_current_pos = nullptr;
            other.m_end_pos = nullptr;
            other.m_size = 0;
            other.m_allocation_count = 0;

#ifdef DESTAN_DEBUG
            // Move debug allocation info
            m_debug_allocation_count = other.m_debug_allocation_count;
            for (destan_u64 i = 0; i < m_debug_allocation_count; ++i)
            {
                m_debug_allocations[i] = other.m_debug_allocations[i];
            }
            other.m_debug_allocation_count = 0;
#endif
        }
        return *this;
    }

    void* Arena_Allocator::Allocate(destan_u64 size, destan_u64 alignment)
    {
        // Handle zero-size allocation
        if (size == 0)
        {
            DESTAN_LOG_WARN("Arena '{0}': Attempted to allocate 0 bytes", m_name);
            return nullptr;
        }

        // Ensure valid alignment (must be power of 2)
        if (alignment == 0)
        {
            alignment = DEFAULT_ALIGNMENT;
        }

        // Verify alignment is a power of 2
        DESTAN_ASSERT((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2");

        // Calculate aligned address
        char* aligned_pos = reinterpret_cast<char*>(
            Memory::Align_Address(m_current_pos, alignment)
            );

        // Check if we have enough space
        if (aligned_pos + size > m_end_pos)
        {
            // Out of memory
#ifdef DESTAN_DEBUG
            DESTAN_LOG_ERROR("Arena '{0}' allocation failed: requested {1} bytes with {2} alignment, "
                "but only {3} bytes available",
                m_name, size, alignment, static_cast<destan_u64>(m_end_pos - m_current_pos));
#endif
            return nullptr;
        }

        // Update current position
        void* result = aligned_pos;
        m_current_pos = aligned_pos + size;
        m_allocation_count++;

        // Return the aligned address
        return result;
    }

    void Arena_Allocator::Reset(bool destruct_objects)
    {
#ifdef DESTAN_DEBUG
        if (destruct_objects && m_debug_allocation_count > 0)
        {
            // Call destructors in reverse order (LIFO)
            for (destan_u64 i = m_debug_allocation_count; i > 0; --i)
            {
                // Note: We don't actually call destructors because we don't track types
                // This is where a more advanced system could store type information
                // and call the appropriate destructors

                // Clear the memory to catch use-after-free
                Memory::Memset(m_debug_allocations[i - 1].ptr, 0xDD, m_debug_allocations[i - 1].size);
            }

            // Clear debug allocations
            m_debug_allocation_count = 0;
        }

        // Fill memory with pattern to help identify use-after-free
        Memory::Memset(m_memory_block, 0xCD, m_size);
        DESTAN_LOG_INFO("Arena '{0}' reset: freed {1} allocations, {2} bytes",
            m_name, m_allocation_count, Get_Used_Size());
#endif

        // Reset current position to start
        m_current_pos = m_start_pos;

        // Reset allocation count
        m_allocation_count = 0;
    }

#ifdef DESTAN_DEBUG
    void* Arena_Allocator::Allocate_Debug(destan_u64 size, destan_u64 alignment, const char* file, int line)
    {
        // Perform the allocation
        void* ptr = Allocate(size, alignment);

        // Track the allocation for debugging
        if (ptr && m_debug_allocation_count < MAX_DEBUG_ALLOCATIONS)
        {
            m_debug_allocations[m_debug_allocation_count++] =
            {
                ptr, size, file, line
            };
        }
        else if (ptr && m_debug_allocation_count >= MAX_DEBUG_ALLOCATIONS)
        {
            DESTAN_LOG_WARN("Arena '{0}': Debug allocation tracking limit reached ({1})",
                m_name, MAX_DEBUG_ALLOCATIONS);
        }

        return ptr;
    }

    void Arena_Allocator::Dump_Stats() const
    {
        std::stringstream ss;
        ss << "===== Arena Allocator '" << m_name << "' Stats =====" << std::endl;
        ss << "Size: " << m_size << " bytes (" << (m_size / 1024.0f) << " KB)" << std::endl;
        ss << "Used: " << Get_Used_Size() << " bytes (" << (Get_Used_Size() / 1024.0f) << " KB)" << std::endl;
        ss << "Free: " << Get_Free_Size() << " bytes (" << (Get_Free_Size() / 1024.0f) << " KB)" << std::endl;
        ss << "Utilization: " << Get_Utilization() << "%" << std::endl;
        ss << "Allocation Count: " << m_allocation_count << std::endl;

        // In debug mode, show detailed allocation information
        if (m_debug_allocation_count > 0)
        {
            ss << "\nDetailed Allocations:" << std::endl;
            ss << "--------------------------------------------------" << std::endl;
            ss << "   Size   |    Address    | Source Location" << std::endl;
            ss << "--------------------------------------------------" << std::endl;

            const destan_u64 MAX_ALLOCATIONS_TO_SHOW = 20; // Limit for readability
            const destan_u64 allocations_to_show =
                (m_debug_allocation_count <= MAX_ALLOCATIONS_TO_SHOW) ?
                m_debug_allocation_count : MAX_ALLOCATIONS_TO_SHOW;

            for (destan_u64 i = 0; i < allocations_to_show; ++i)
            {
                const auto& alloc = m_debug_allocations[i];
                ss << "  " << std::setw(7) << alloc.size << " | "
                    << std::hex << std::setw(12) << alloc.ptr << std::dec << " | "
                    << alloc.file << ":" << alloc.line << std::endl;
            }

            if (m_debug_allocation_count > MAX_ALLOCATIONS_TO_SHOW)
            {
                ss << "... and " << (m_debug_allocation_count - MAX_ALLOCATIONS_TO_SHOW)
                    << " more allocations" << std::endl;
            }
        }

        ss << "==============================================";
        DESTAN_LOG_INFO(ss.str());
    }
#endif

}