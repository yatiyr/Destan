#include <core/destan_pch.h>
#include <core/memory/stack_allocator.h>

namespace destan::core::memory
{
    Stack_Allocator::Stack_Allocator(destan_u64 size_bytes, const char* name)
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
        else
        {
            // Default name if none provided
            const destan_char* default_name = "Stack";
            destan_u64 i = 0;
            while (default_name[i] && i < MAX_NAME_LENGTH - 1)
            {
                m_name[i] = default_name[i];
                i++;
            }
            m_name[i] = '\0';
        }

        // Calculate total memory size needed
        destan_u64 total_size = size_bytes;

#ifdef DESTAN_DEBUG
        // Add space for debug tracking in debug builds
        destan_u64 debug_tracking_size = MAX_DEBUG_ALLOCATIONS * sizeof(Allocation_Info);
        destan_u64 debug_tracking_aligned_size = Memory::Align_Size(debug_tracking_size, CACHE_LINE_SIZE);
        total_size += debug_tracking_aligned_size;
#endif

        // Allocate the memory block with cache line alignment for optimal performance
        m_memory_block = Memory::Malloc(total_size, CACHE_LINE_SIZE);
        DESTAN_ASSERT(m_memory_block, "Failed to allocate memory for Stack Allocator");

        // Initialize pointers (using offsets to simplify calculations)
        m_start_pos = reinterpret_cast<destan_u64>(m_memory_block);
        m_current_pos = m_start_pos;
        m_end_pos = m_start_pos + size_bytes;

#ifdef DESTAN_DEBUG
        // Set up debug tracking array at the end of the stack
        m_debug_allocations = reinterpret_cast<Allocation_Info*>(
            reinterpret_cast<destan_char*>(m_memory_block) + size_bytes
            );

        // Initialize debug allocation count
        m_debug_allocation_count = 0;

        // Fill memory with pattern to help identify uninitialized memory
        Memory::Memset(m_memory_block, 0xCD, size_bytes); // 0xCD = "Clean Dynamic memory"

        DESTAN_LOG_INFO("Stack allocator '{0}' created with {1} bytes (+ {2} bytes debug info)",
            m_name, size_bytes, debug_tracking_aligned_size);
#else
        // In release builds, just log basic information
        DESTAN_LOG_INFO("Stack allocator '{0}' created with {1} bytes",
            m_name, total_size);
#endif
    }

    Stack_Allocator::~Stack_Allocator()
    {
        // Free the memory block
        if (m_memory_block) {
#ifdef DESTAN_DEBUG
            // Check for leaks before freeing
            if (m_current_pos > m_start_pos)
            {
                DESTAN_LOG_WARN("Stack '{0}' destroyed with {1} bytes still allocated",
                    m_name, m_current_pos - m_start_pos);
            }

            // Fill with pattern to catch use-after-free
            destan_u64 stack_size = m_end_pos - m_start_pos;
            Memory::Memset(m_memory_block, 0xDD, stack_size); // 0xDD = "Dead Dynamic memory"
#endif

            Memory::Free(m_memory_block);
            m_memory_block = nullptr;
            m_start_pos = 0;
            m_current_pos = 0;
            m_end_pos = 0;
        }
    }

    Stack_Allocator::Stack_Allocator(Stack_Allocator&& other) noexcept
        : m_memory_block(other.m_memory_block)
        , m_start_pos(other.m_start_pos)
        , m_current_pos(other.m_current_pos)
        , m_end_pos(other.m_end_pos)
        , m_size(other.m_size)
    {
        // Copy name
        Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

        // Transfer ownership, clear the source
        other.m_memory_block = nullptr;
        other.m_start_pos = 0;
        other.m_current_pos = 0;
        other.m_end_pos = 0;
        other.m_size = 0;

#ifdef DESTAN_DEBUG
        // Debug allocations are part of the main memory allocation, so they moved with it
        // We just need to calculate their position in the new memory
        if (m_memory_block)
        {
            destan_u64 stack_size = m_end_pos - m_start_pos;
            m_debug_allocations = reinterpret_cast<Allocation_Info*>(
                reinterpret_cast<destan_char*>(m_memory_block) + stack_size
                );
            m_debug_allocation_count = other.m_debug_allocation_count;
        }
        else
        {
            m_debug_allocations = nullptr;
            m_debug_allocation_count = 0;
        }
        other.m_debug_allocations = nullptr;
        other.m_debug_allocation_count = 0;
#endif
    }

    Stack_Allocator& Stack_Allocator::operator=(Stack_Allocator&& other) noexcept
    {
        if (this != &other)
        {
            // Free existing resources
            if (m_memory_block)
            {
                Memory::Free(m_memory_block);
                // No separate deletion for debug allocations - they're part of m_memory_block
            }

            // Transfer ownership
            m_memory_block = other.m_memory_block;
            m_start_pos = other.m_start_pos;
            m_current_pos = other.m_current_pos;
            m_end_pos = other.m_end_pos;
            m_size = other.m_size;

            // Copy name
            Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

            // Clear the source
            other.m_memory_block = nullptr;
            other.m_start_pos = 0;
            other.m_current_pos = 0;
            other.m_end_pos = 0;
            other.m_size = 0;

#ifdef DESTAN_DEBUG
            // Debug allocations are part of the main memory allocation, so they moved with it
            // We just need to calculate their position in the new memory
            if (m_memory_block)
            {
                destan_u64 stack_size = m_end_pos - m_start_pos;
                m_debug_allocations = reinterpret_cast<Allocation_Info*>(
                    reinterpret_cast<char*>(m_memory_block) + stack_size
                    );
                m_debug_allocation_count = other.m_debug_allocation_count;
            }
            else
            {
                m_debug_allocations = nullptr;
                m_debug_allocation_count = 0;
            }
            other.m_debug_allocations = nullptr;
            other.m_debug_allocation_count = 0;
#endif
        }
        return *this;
    }


    void* Stack_Allocator::Allocate(destan_u64 size, destan_u64 alignment)
    {
#ifdef DESTAN_DEBUG
        Lock();
#endif

        // Handle zero-size allocation
        if (size == 0)
        {
#ifdef DESTAN_DEBUG
            DESTAN_LOG_WARN("Stack '{0}': Attempted to allocate 0 bytes", m_name);
            Unlock();
#endif
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
        destan_u64 aligned_pos = Align_Address(m_current_pos, alignment);

        // Check if we have enough space
        if (aligned_pos + size > m_end_pos)
        {
            // Out of memory
#ifdef DESTAN_DEBUG
            DESTAN_LOG_ERROR("Stack '{0}' allocation failed: requested {1} bytes with {2} alignment, "
                "but only {3} bytes available",
                m_name, size, alignment, m_end_pos - m_current_pos);
            Unlock();
#endif
            return nullptr;
        }

        // Save the current position for the return value
        destan_u64 result_pos = aligned_pos;

        // Update current position
        m_current_pos = aligned_pos + size;

        void* result_pos_void = reinterpret_cast<void*>(result_pos);

#ifdef DESTAN_DEBUG
        // Update debug info if allocation succeeded
        if (result_pos_void && m_debug_allocation_count < MAX_DEBUG_ALLOCATIONS)
        {
            // Track the allocation
            m_debug_allocations[m_debug_allocation_count++] =
            {
                reinterpret_cast<destan_u64>(result_pos_void),  // position
                size,                               // size
                alignment,                          // alignment
                "",                               // file
                -1                                // line
            };
        }
        else if (result_pos_void && m_debug_allocation_count >= MAX_DEBUG_ALLOCATIONS)
        {
            DESTAN_LOG_WARN("Stack '{0}': Debug allocation tracking limit reached ({1})",
                m_name, MAX_DEBUG_ALLOCATIONS);
        }
        Unlock();
#endif

        // Return the aligned address
        return reinterpret_cast<void*>(result_pos);
    }

    Stack_Allocator::Marker Stack_Allocator::Get_Marker() const
    {
        return m_current_pos;
    }

    void Stack_Allocator::Free_To_Marker(Marker marker, bool destruct_objects) {
#ifdef DESTAN_DEBUG
        Lock();

        // Validate marker is within our stack
        if (marker < m_start_pos || marker > m_current_pos)
        {
            DESTAN_LOG_ERROR("Stack '{0}': Invalid marker {1}", m_name, marker);
            Unlock();
            return;
        }

        if (destruct_objects && m_debug_allocation_count > 0)
        {
            // Call destructors in reverse order for objects allocated after this marker
            // We go backwards through debug allocations to maintain proper LIFO ordering
            for (destan_i64 i = static_cast<destan_i64>(m_debug_allocation_count) - 1; i >= 0; --i)
            {
                if (m_debug_allocations[i].pos >= marker)
                {
                    // This allocation was made after the marker
                    // Note: We don't actually call destructors because we don't track types
                    // This is where a more advanced system could store type information
                    // and call the appropriate destructors

                    // Clear the memory to catch use-after-free bugs
                    void* ptr = reinterpret_cast<void*>(m_debug_allocations[i].pos);
                    Memory::Memset(ptr, 0xDD, m_debug_allocations[i].size);

                    // Remove from debug tracking
                    if (i < static_cast<destan_i64>(m_debug_allocation_count) - 1) {
                        // Not the last element, move the last element into this slot
                        m_debug_allocations[i] = m_debug_allocations[m_debug_allocation_count - 1];
                    }
                    m_debug_allocation_count--;
                }
            }
        }

        // Fill freed memory with pattern in debug mode
        destan_u64 freed_size = m_current_pos - marker;
        if (freed_size > 0)
        {
            Memory::Memset(reinterpret_cast<void*>(marker), 0xCD, freed_size);
        }

        DESTAN_LOG_INFO("Stack '{0}' freed to marker: {1} bytes released", m_name, freed_size);
#endif

        // Reset current position to the marker
        m_current_pos = marker;

#ifdef DESTAN_DEBUG
        Unlock();
#endif
    }

    bool Stack_Allocator::Free_Latest()
    {
#ifdef DESTAN_DEBUG
        Lock();

        // Check if stack is empty
        if (m_current_pos == m_start_pos)
        {
            DESTAN_LOG_WARN("Stack '{0}': Cannot free latest allocation - stack is empty", m_name);
            Unlock();
            return false;
        }

        // Find latest allocation
        if (m_debug_allocation_count > 0)
        {
            // Find the allocation with the highest position (should be the most recent)
            destan_u64 latest_allocation_index = 0;
            destan_u64 highest_pos = 0;

            for (destan_u64 i = 0; i < m_debug_allocation_count; ++i)
            {
                if (m_debug_allocations[i].pos > highest_pos)
                {
                    highest_pos = m_debug_allocations[i].pos;
                    latest_allocation_index = i;
                }
            }

            // Return to the position of this allocation
            Marker marker = m_debug_allocations[latest_allocation_index].pos;

            // Clear the memory to catch use-after-free bugs
            Memory::Memset(reinterpret_cast<void*>(marker), 0xCD,
                m_debug_allocations[latest_allocation_index].size);

            // Remove from debug tracking (swap with last element)
            if (latest_allocation_index < m_debug_allocation_count - 1)
            {
                m_debug_allocations[latest_allocation_index] = m_debug_allocations[m_debug_allocation_count - 1];
            }
            m_debug_allocation_count--;

            // Update current position
            m_current_pos = marker;

            DESTAN_LOG_INFO("Stack '{0}' freed latest allocation: {1} bytes released",
                m_name, highest_pos + m_debug_allocations[latest_allocation_index].size - marker);

            Unlock();
            return true;
        }

        // If we get here in debug mode, we don't have debug info for any allocations
        // This would be strange, but we'll handle it by just resetting the stack
        DESTAN_LOG_WARN("Stack '{0}': No debug info available for latest allocation", m_name);
        Reset();

        Unlock();
        return false;
#else
        // In release mode, we don't track individual allocations
        // So we can't free just the latest one
        Reset();
        return false;
#endif
    }

    void Stack_Allocator::Reset(bool destruct_objects)
    {
#ifdef DESTAN_DEBUG
        Lock();

        if (destruct_objects && m_debug_allocation_count > 0)
        {
            // Call destructors in reverse order
            for (destan_i64 i = static_cast<destan_i64>(m_debug_allocation_count) - 1; i >= 0; --i)
            {
                // Note: We don't actually call destructors because we don't track types
                // This is where a more advanced system could store type information
                // and call the appropriate destructors

                // Clear the memory to catch use-after-free bugs
                void* ptr = reinterpret_cast<void*>(m_debug_allocations[i].pos);
                Memory::Memset(ptr, 0xDD, m_debug_allocations[i].size);
            }
        }

        // Reset debug allocation count
        m_debug_allocation_count = 0;

        // Fill memory with pattern to help identify uninitialized memory
        destan_u64 used_size = m_current_pos - m_start_pos;
        if (used_size > 0)
        {
            Memory::Memset(reinterpret_cast<void*>(m_start_pos), 0xCD, used_size);
        }

        DESTAN_LOG_INFO("Stack '{0}' reset: {1} bytes freed", m_name, used_size);
#endif

        // Reset current position to start
        m_current_pos = m_start_pos;

#ifdef DESTAN_DEBUG
        Unlock();
#endif
    }

    destan_u64 Stack_Allocator::Align_Address(destan_u64 address, destan_u64 alignment) const
    {
        return (address + alignment - 1) & ~(alignment - 1);
    }

#ifdef DESTAN_DEBUG
    void Stack_Allocator::Lock()
    {
        m_mutex.lock();
    }

    void Stack_Allocator::Unlock()
    {
        m_mutex.unlock();
    }

    void* Stack_Allocator::Allocate_Debug(destan_u64 size, destan_u64 alignment, const char* file, int line)
    {
        Lock();

        // Perform the allocation
        void* ptr = Allocate(size, alignment);

        // Update debug info if allocation succeeded
        if (ptr && m_debug_allocation_count < MAX_DEBUG_ALLOCATIONS)
        {
            // Track the allocation
            m_debug_allocations[m_debug_allocation_count++] =
            {
                reinterpret_cast<destan_u64>(ptr),  // position
                size,                               // size
                alignment,                          // alignment
                file,                               // file
                line                                // line
            };
        }
        else if (ptr && m_debug_allocation_count >= MAX_DEBUG_ALLOCATIONS)
        {
            DESTAN_LOG_WARN("Stack '{0}': Debug allocation tracking limit reached ({1})",
                m_name, MAX_DEBUG_ALLOCATIONS);
        }

        Unlock();
        return ptr;
    }

    void Stack_Allocator::Dump_Stats()
    {
        Lock();

        std::stringstream ss;
        ss << "===== Stack Allocator '" << m_name << "' Stats =====" << std::endl;
        ss << "Size: " << m_size << " bytes (" << (m_size / 1024.0f) << " KB)" << std::endl;
        ss << "Used: " << Get_Used_Size() << " bytes (" << (Get_Used_Size() / 1024.0f) << " KB)" << std::endl;
        ss << "Free: " << Get_Free_Size() << " bytes (" << (Get_Free_Size() / 1024.0f) << " KB)" << std::endl;
        ss << "Utilization: " << Get_Utilization() << "%" << std::endl;

        // In debug mode, show detailed allocation information
        if (m_debug_allocation_count > 0)
        {
            ss << "\nAllocations (from oldest to newest):" << std::endl;
            ss << "--------------------------------------------------" << std::endl;
            ss << "    Size   |    Address    | Source Location" << std::endl;
            ss << "--------------------------------------------------" << std::endl;

            // Sort allocations by position to show them in order of allocation
            struct SortedAlloc
            {
                destan_u64 index;
                destan_u64 pos;
            };

            SortedAlloc sorted[MAX_DEBUG_ALLOCATIONS];
            for (destan_u64 i = 0; i < m_debug_allocation_count; ++i)
            {
                sorted[i] = { i, m_debug_allocations[i].pos };
            }

            // Simple insertion sort by position
            for (destan_u64 i = 1; i < m_debug_allocation_count; ++i)
            {
                SortedAlloc key = sorted[i];
                destan_i64 j = static_cast<destan_i64>(i) - 1;

                while (j >= 0 && sorted[j].pos > key.pos) {
                    sorted[j + 1] = sorted[j];
                    j--;
                }
                sorted[j + 1] = key;
            }

            const destan_u64 MAX_ALLOCATIONS_TO_SHOW = 20; // Limit for readability
            const destan_u64 allocations_to_show =
                (m_debug_allocation_count <= MAX_ALLOCATIONS_TO_SHOW) ?
                m_debug_allocation_count : MAX_ALLOCATIONS_TO_SHOW;

            for (destan_u64 i = 0; i < allocations_to_show; ++i)
            {
                const auto& alloc = m_debug_allocations[sorted[i].index];
                ss << "  " << std::setw(8) << alloc.size << " | "
                    << std::hex << std::setw(12) << alloc.pos << std::dec << " | ";

                if (alloc.file) {
                    ss << alloc.file << ":" << alloc.line;
                }
                else {
                    ss << "unknown location";
                }

                ss << std::endl;
            }

            if (m_debug_allocation_count > MAX_ALLOCATIONS_TO_SHOW) {
                ss << "... and " << (m_debug_allocation_count - MAX_ALLOCATIONS_TO_SHOW)
                    << " more allocations" << std::endl;
            }
        }

        ss << "==============================================";
        DESTAN_LOG_INFO("{}", ss.str());

        Unlock();
    }
#endif
}