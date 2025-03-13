#include <core/destan_pch.h>
#include <core/memory/pool_allocator.h>


namespace destan::core::memory
{
    Pool_Allocator::Pool_Allocator(destan_u64 block_size, destan_u64 block_count, const destan_char* name)
        : m_block_size(std::max(block_size, sizeof(Free_Block)))
        , m_block_count(block_count)
        , m_free_blocks(block_count)
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
        else {
            // Default name if none provided
            const destan_char* default_name = "Pool";
            destan_u64 i = 0;
            while (default_name[i] && i < MAX_NAME_LENGTH - 1)
            {
                m_name[i] = default_name[i];
                i++;
            }
            m_name[i] = '\0';
        }

        // Set block alignment to DEFAULT_ALIGNMENT or higher if required
        m_block_alignment = DEFAULT_ALIGNMENT;

        // Calculate padded block size to ensure proper alignment
        m_padded_block_size = Memory::Align_Size(m_block_size, m_block_alignment);

        // Calculate total memory size needed
        destan_u64 blocks_size = m_padded_block_size * block_count;
        destan_u64 total_size = blocks_size;

#ifdef DESTAN_DEBUG
        // Add space for debug tracking in debug builds
        destan_u64 debug_tracking_size = block_count * sizeof(Allocation_Info);
        destan_u64 debug_tracking_aligned_size = Memory::Align_Size(debug_tracking_size, CACHE_LINE_SIZE);
        total_size += debug_tracking_aligned_size;
#endif

        // Allocate the pool memory with cache line alignment for optimal performance
        m_memory_pool = Memory::Malloc(total_size, CACHE_LINE_SIZE);
        DESTAN_ASSERT(m_memory_pool, "Failed to allocate memory for Pool Allocator");

        // Initialize the free block linked list
        Free_Block* current = static_cast<Free_Block*>(m_memory_pool);
        m_free_list = current;

        // Chain all blocks together
        for (destan_u64 i = 0; i < block_count - 1; ++i)
        {
            current->next = reinterpret_cast<Free_Block*>(reinterpret_cast<destan_char*>(current) + m_padded_block_size);
            current = current->next;
        }

        // Last block's next pointer is null
        current->next = nullptr;

#ifdef DESTAN_DEBUG
        // Set up debug tracking array at the end of the pool
        m_debug_blocks = reinterpret_cast<Allocation_Info*>(static_cast<destan_char*>(m_memory_pool) + blocks_size);

        // Initialize debug tracking info
        for (destan_u64 i = 0; i < block_count; ++i)
        {
            void* block_ptr = static_cast<destan_char*>(m_memory_pool) + (i * m_padded_block_size);
            // Use placement new to properly initialize the struct
            new(&m_debug_blocks[i]) Allocation_Info{ block_ptr, nullptr, 0, false };
        }

        // Fill memory with a pattern to help identify uninitialized memory
        Memory::Memset(m_memory_pool, 0xCD, blocks_size); // 0xCD = "Clean Dynamic memory"

        DESTAN_LOG_INFO("Pool allocator '{0}' created with {1} blocks, {2} bytes each, {3} bytes total (+ {4} bytes debug info)",
            m_name, block_count, m_block_size, blocks_size, debug_tracking_aligned_size);
#else
        // In release builds, just log basic information
        DESTAN_LOG_INFO("Pool allocator '{0}' created with {1} blocks, {2} bytes each, {3} bytes total",
            m_name, block_count, m_block_size, total_size);
#endif
    }

    Pool_Allocator::~Pool_Allocator()
    {
        // Free the pool memory
        if (m_memory_pool)
        {
#ifdef DESTAN_DEBUG
            // Check for leaks before freeing
            destan_u64 allocated = Get_Allocated_Block_Count();
            if (allocated > 0)
            {
                DESTAN_LOG_WARN("Pool '{0}' destroyed with {1} active allocations", m_name, allocated);
            }

            // Fill with pattern to catch use-after-free
            destan_u64 total_size = m_padded_block_size * m_block_count;
            Memory::Memset(m_memory_pool, 0xDD, total_size); // 0xDD = "Dead Dynamic memory"

            // No need to separately free debug blocks as they are part of our main allocation
            m_debug_blocks = nullptr;
#endif

            Memory::Free(m_memory_pool);
            m_memory_pool = nullptr;
            m_free_list = nullptr;
        }
    }

    Pool_Allocator::Pool_Allocator(Pool_Allocator&& other) noexcept
        : m_memory_pool(other.m_memory_pool)
        , m_block_size(other.m_block_size)
        , m_padded_block_size(other.m_padded_block_size)
        , m_block_alignment(other.m_block_alignment)
        , m_block_count(other.m_block_count)
        , m_free_list(other.m_free_list)
    {
        // Copy name
        Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

        // Move the free block count atomically
        m_free_blocks.store(other.m_free_blocks.load(std::memory_order_relaxed), std::memory_order_relaxed);

        // Transfer ownership, clear the source
        other.m_memory_pool = nullptr;
        other.m_free_list = nullptr;
        other.m_block_count = 0;
        other.m_free_blocks.store(0, std::memory_order_relaxed);

#ifdef DESTAN_DEBUG
        // Move debug blocks
        m_debug_blocks = other.m_debug_blocks;
        other.m_debug_blocks = nullptr;
#endif
    }

    Pool_Allocator& Pool_Allocator::operator=(Pool_Allocator&& other) noexcept
    {
        if (this != &other)
        {
            // Free existing resources
            if (m_memory_pool)
            {
                Memory::Free(m_memory_pool);
                // No separate deletion for debug blocks - they're part of m_memory_pool
            }

            // Transfer ownership
            m_memory_pool = other.m_memory_pool;
            m_block_size = other.m_block_size;
            m_padded_block_size = other.m_padded_block_size;
            m_block_alignment = other.m_block_alignment;
            m_block_count = other.m_block_count;
            m_free_list = other.m_free_list;

            // Copy name
            Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

            // Move the free block count atomically
            m_free_blocks.store(other.m_free_blocks.load(std::memory_order_relaxed), std::memory_order_relaxed);

            // Clear the source
            other.m_memory_pool = nullptr;
            other.m_free_list = nullptr;
            other.m_block_count = 0;
            other.m_free_blocks.store(0, std::memory_order_relaxed);

#ifdef DESTAN_DEBUG
            // Debug blocks are part of the main memory allocation, so they moved with it
            // We just need to calculate their position in the new memory
            if (m_memory_pool)
            {
                destan_u64 blocks_size = m_padded_block_size * m_block_count;
                m_debug_blocks = reinterpret_cast<Allocation_Info*>(static_cast<destan_char*>(m_memory_pool) + blocks_size);
            }
            else
            {
                m_debug_blocks = nullptr;
            }
            other.m_debug_blocks = nullptr;
#endif
        }
        return *this;
    }

    void* Pool_Allocator::Allocate()
    {
        // Lock for thread safety in debug mode
        Lock();

        // Check if we have free blocks
        if (!m_free_list)
        {
            DESTAN_LOG_ERROR("Pool '{0}' allocation failed: pool is full ({1} blocks)",
                m_name, m_block_count);
            Unlock();
            return nullptr;
        }

        // Take a block from the free list
        Free_Block* block = m_free_list;
        m_free_list = block->next;

        // Update free block count atomically
        m_free_blocks.fetch_sub(1, std::memory_order_relaxed);

#ifdef DESTAN_DEBUG
        // Find and mark the block as allocated in debug info
        destan_u64 block_index = Get_Block_Index(block);
        if (block_index < m_block_count)
        {
            m_debug_blocks[block_index].allocated = true;
        }
#endif

        Unlock();
        return block;
    }

    bool Pool_Allocator::Deallocate(void* ptr)
    {
        // Handle null pointer
        if (!ptr)
        {
            return false;
        }

        // Lock for thread safety in debug mode
        Lock();

        // Validate the pointer belongs to our pool
        if (!Is_Address_In_Pool(ptr))
        {
            DESTAN_LOG_ERROR("Pool '{0}' deallocation failed: pointer {1} not from this pool",
                m_name, ptr);
            Unlock();
            return false;
        }

        // Check if address is at a valid block boundary
        if ((reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(m_memory_pool)) % m_padded_block_size != 0)
        {
            DESTAN_LOG_ERROR("Pool '{0}' deallocation failed: pointer {1} not aligned to block boundary",
                m_name, ptr);
            Unlock();
            return false;
        }

#ifdef DESTAN_DEBUG
        // Update debug state
        destan_u64 block_index = Get_Block_Index(ptr);
        if (block_index < m_block_count)
        {
            if (!m_debug_blocks[block_index].allocated)
            {
                DESTAN_LOG_ERROR("Pool '{0}' double-free detected at {1}", m_name, ptr);
                Unlock();
                return false;
            }
            m_debug_blocks[block_index].allocated = false;

            // Fill with pattern to help catch use-after-free bugs
            Memory::Memset(ptr, 0xDD, m_block_size);
        }
#endif

        // Add the block back to the free list
        Free_Block* block = static_cast<Free_Block*>(ptr);
        block->next = m_free_list;
        m_free_list = block;

        // Update free block count atomically
        m_free_blocks.fetch_add(1, std::memory_order_relaxed);

        Unlock();
        return true;
    }


    void Pool_Allocator::Reset()
    {
        // Lock for thread safety in debug mode
        Lock();

        // Re-initialize the free block linked list
        Free_Block* current = static_cast<Free_Block*>(m_memory_pool);
        m_free_list = current;

        // Chain all blocks together
        for (destan_u64 i = 0; i < m_block_count - 1; ++i)
        {
            current->next = reinterpret_cast<Free_Block*>(
                reinterpret_cast<destan_char*>(current) + m_padded_block_size
                );
            current = current->next;
        }

        // Last block's next pointer is null
        current->next = nullptr;

        // Reset free block count atomically
        m_free_blocks.store(m_block_count, std::memory_order_relaxed);

#ifdef DESTAN_DEBUG
        // Reset all debug blocks
        for (destan_u64 i = 0; i < m_block_count; ++i)
        {
            m_debug_blocks[i].allocated = false;
        }

        // Fill memory with pattern to help identify uninitialized memory
        destan_u64 total_size = m_padded_block_size * m_block_count;
        Memory::Memset(m_memory_pool, 0xCD, total_size);

        DESTAN_LOG_INFO("Pool '{0}' reset: all {1} blocks now available", m_name, m_block_count);
#endif

        Unlock();
    }

    bool Pool_Allocator::Is_Address_In_Pool(void* ptr) const
    {
        // Calculate pool bounds
        uintptr_t pool_start = reinterpret_cast<uintptr_t>(m_memory_pool);
        uintptr_t pool_end = pool_start + (m_block_count * m_padded_block_size);
        uintptr_t ptr_addr = reinterpret_cast<uintptr_t>(ptr);

        // Check if the pointer is within pool bounds
        return (ptr_addr >= pool_start && ptr_addr < pool_end);
    }

    destan_u64 Pool_Allocator::Get_Block_Index(void* ptr) const
    {
        // Calculate the block index from the pointer
        uintptr_t pool_start = reinterpret_cast<uintptr_t>(m_memory_pool);
        uintptr_t ptr_addr = reinterpret_cast<uintptr_t>(ptr);

        return (ptr_addr - pool_start) / m_padded_block_size;
    }

    void Pool_Allocator::Lock()
    {
#ifdef DESTAN_DEBUG
        m_mutex.lock();
#endif
    }

    void Pool_Allocator::Unlock()
    {
#ifdef DESTAN_DEBUG
        m_mutex.unlock();
#endif
    }


#ifdef DESTAN_DEBUG
    void* Pool_Allocator::Allocate_Debug(const char* file, int line)
    {
        // Perform the allocation
        void* ptr = Allocate();

        // Update debug info if allocation succeeded
        if (ptr)
        {
            Lock();
            destan_u64 block_index = Get_Block_Index(ptr);
            if (block_index < m_block_count) {
                m_debug_blocks[block_index].file = file;
                m_debug_blocks[block_index].line = line;
            }
            Unlock();
        }

        return ptr;
    }

    void Pool_Allocator::Dump_Stats()
    {
        Lock();

        std::stringstream ss;
        ss << "===== Pool Allocator '" << m_name << "' Stats =====" << std::endl;
        ss << "Block Size: " << m_block_size << " bytes (padded to " << m_padded_block_size << " bytes)" << std::endl;
        ss << "Block Count: " << m_block_count << std::endl;
        ss << "Free Blocks: " << Get_Free_Block_Count() << std::endl;
        ss << "Used Blocks: " << Get_Allocated_Block_Count() << std::endl;
        ss << "Utilization: " << Get_Utilization() << "%" << std::endl;

        // In debug mode, show detailed allocation information
        if (m_debug_blocks && Get_Allocated_Block_Count() > 0)
        {
            ss << "\nAllocated Blocks:" << std::endl;
            ss << "--------------------------------------------------" << std::endl;
            ss << "  Block # |    Address    | Source Location" << std::endl;
            ss << "--------------------------------------------------" << std::endl;

            const destan_u64 MAX_BLOCKS_TO_SHOW = 20; // Limit for readability
            destan_u64 blocks_shown = 0;

            for (destan_u64 i = 0; i < m_block_count && blocks_shown < MAX_BLOCKS_TO_SHOW; ++i)
            {
                if (m_debug_blocks[i].allocated)
                {
                    ss << "  " << std::setw(7) << i << " | "
                        << std::hex << std::setw(12) << m_debug_blocks[i].ptr << std::dec << " | ";

                    if (m_debug_blocks[i].file)
                    {
                        ss << m_debug_blocks[i].file << ":" << m_debug_blocks[i].line;
                    }
                    else
                    {
                        ss << "unknown location";
                    }

                    ss << std::endl;
                    blocks_shown++;
                }
            }

            destan_u64 allocated = Get_Allocated_Block_Count();
            if (allocated > MAX_BLOCKS_TO_SHOW)
            {
                ss << "... and " << (allocated - MAX_BLOCKS_TO_SHOW)
                    << " more allocated blocks" << std::endl;
            }
        }

        ss << "==============================================";
        DESTAN_LOG_INFO("{}", ss.str());

        Unlock();
    }
#endif

} // namespace destan::core::memory