#include <core/destan_pch.h>
#include <core/memory/free_list_allocator.h>

namespace destan::core::memory
{
#ifdef DESTAN_DEBUG
    // Initialize static member
    std::atomic<destan_u64> Free_List_Allocator::s_next_allocation_id{ 1 };
#endif

    Free_List_Allocator::Free_List_Allocator(destan_u64 size_bytes,
        Allocation_Strategy strategy,
        const destan_char* name)
        : m_strategy(strategy)
        , m_size(size_bytes)
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
            const destan_char* default_name = "Free_List";
            destan_u64 i = 0;
            while (default_name[i] && i < MAX_NAME_LENGTH - 1)
            {
                m_name[i] = default_name[i];
                i++;
            }
            m_name[i] = '\0';
        }

        // Ensure size is at least large enough for one block
        if (size_bytes < sizeof(Block_Header) + MIN_BLOCK_SIZE)
        {
            DESTAN_LOG_ERROR("Free List Allocator '{0}': size too small ({1} bytes), minimum is {2} bytes",
                m_name, size_bytes, sizeof(Block_Header) + MIN_BLOCK_SIZE);

            // Adjust size to minimum
            m_size = sizeof(Block_Header) + MIN_BLOCK_SIZE;
        }

        // Allocate memory region (align to cache line for better performance)
        m_memory_region = Memory::Malloc(m_size, CACHE_LINE_SIZE);
        DESTAN_ASSERT(m_memory_region, "Failed to allocate memory for Free List Allocator");

        // Initialize the first block to cover the entire region
        m_first_block = static_cast<Block_Header*>(m_memory_region);
        m_first_block->size = m_size;
        m_first_block->is_free = true;
        m_first_block->next = nullptr;
        m_first_block->prev = nullptr;
        m_first_block->next_free = nullptr;
        m_first_block->prev_free = nullptr;

        // The free list initially contains just the first block
        m_free_list = m_first_block;
        m_free_block_count = 1;

#ifdef DESTAN_DEBUG
        // Initialize debug values
        m_first_block->allocation_id = 0; // Not allocated yet
        m_first_block->file = nullptr;
        m_first_block->line = 0;
        m_first_block->guard_value = Block_Header::GUARD_PATTERN;
        m_allocation_count = 0;

        // Fill memory with pattern to help identify uninitialized memory
        destan_u64 user_size = m_size - sizeof(Block_Header);
        Memory::Memset(reinterpret_cast<destan_u8*>(m_memory_region) + sizeof(Block_Header),
            0xCD, user_size); // 0xCD = "Clean Dynamic memory"

        DESTAN_LOG_INFO("Free List Allocator '{0}' created with {1} bytes", m_name, m_size);
#endif
    }

    Free_List_Allocator::~Free_List_Allocator()
    {
        // Free the entire memory region
        if (m_memory_region)
        {
#ifdef DESTAN_DEBUG
            // Check for memory leaks before freeing
            if (m_allocation_count > 0)
            {
                DESTAN_LOG_WARN("Free List Allocator '{0}' destroyed with {1} active allocations ({2} bytes)",
                    m_name, m_allocation_count, Get_Used_Size());

                // List all active allocations
                Block_Header* current = m_first_block;
                while (current)
                {
                    if (!current->is_free)
                    {
                        DESTAN_LOG_WARN("  Leaked allocation: {0} bytes at {1} ({2}:{3})",
                            current->size - sizeof(Block_Header),
                            static_cast<void*>(reinterpret_cast<destan_u8*>(current) + sizeof(Block_Header)),
                            current->file ? current->file : "unknown",
                            current->line);
                    }
                    current = current->next;
                }
            }

            // Fill memory with pattern to help catch use-after-free
            Memory::Memset(m_memory_region, 0xDD, m_size); // 0xDD = "Dead Dynamic memory"
#endif

            Memory::Free(m_memory_region);
            m_memory_region = nullptr;
            m_first_block = nullptr;
            m_free_list = nullptr;
            m_last_allocated = nullptr;
            m_size = 0;
            m_free_block_count = 0;
        }
    }

    Free_List_Allocator::Free_List_Allocator(Free_List_Allocator&& other) noexcept
        : m_memory_region(other.m_memory_region)
        , m_size(other.m_size)
        , m_first_block(other.m_first_block)
        , m_free_list(other.m_free_list)
        , m_last_allocated(other.m_last_allocated)
        , m_free_block_count(other.m_free_block_count)
        , m_strategy(other.m_strategy)
    {
        // Copy name
        Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

        // Transfer ownership
        other.m_memory_region = nullptr;
        other.m_first_block = nullptr;
        other.m_free_list = nullptr;
        other.m_last_allocated = nullptr;
        other.m_size = 0;
        other.m_free_block_count = 0;

#ifdef DESTAN_DEBUG
        // Move allocation count
        m_allocation_count = other.m_allocation_count;
        other.m_allocation_count = 0;
#endif
    }

    Free_List_Allocator& Free_List_Allocator::operator=(Free_List_Allocator&& other) noexcept
    {
        if (this != &other)
        {
            // Free existing resources
            if (m_memory_region)
            {
                Memory::Free(m_memory_region);
            }

            // Transfer resources
            m_memory_region = other.m_memory_region;
            m_size = other.m_size;
            m_first_block = other.m_first_block;
            m_free_list = other.m_free_list;
            m_last_allocated = other.m_last_allocated;
            m_free_block_count = other.m_free_block_count;
            m_strategy = other.m_strategy;

            // Copy name
            Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

            // Clear other's resources
            other.m_memory_region = nullptr;
            other.m_first_block = nullptr;
            other.m_free_list = nullptr;
            other.m_last_allocated = nullptr;
            other.m_size = 0;
            other.m_free_block_count = 0;

#ifdef DESTAN_DEBUG
            // Move allocation count
            m_allocation_count = other.m_allocation_count;
            other.m_allocation_count = 0;
#endif
        }
        return *this;
    }

    void* Free_List_Allocator::Allocate(destan_u64 size, destan_u64 alignment)
    {
        // Handle zero-size allocation
        if (size == 0)
        {
            DESTAN_LOG_WARN("Free List Allocator '{0}': Attempted to allocate 0 bytes", m_name);
            return nullptr;
        }

        // Ensure minimum allocation size to avoid excessive fragmentation
        if (size < sizeof(void*) * 2)
        {
            size = sizeof(void*) * 2;
        }

        // Ensure valid alignment (must be power of 2)
        if (alignment == 0)
        {
            alignment = DEFAULT_ALIGNMENT;
        }

        // Verify alignment is a power of 2
        DESTAN_ASSERT((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2");

        // Thread safety
        Lock();

        // Find a suitable block
        Block_Header* block = Find_Suitable_Block(size, alignment);
        if (!block)
        {
            // No suitable block found
            DESTAN_LOG_ERROR("Free List Allocator '{0}': Failed to allocate {1} bytes (alignment {2})",
                m_name, size, alignment);
            Unlock();
            return nullptr;
        }

        // Calculate required padding for alignment
        destan_u8* aligned_address = reinterpret_cast<destan_u8*>(
            Memory::Align_Address(
                reinterpret_cast<destan_u8*>(block) + sizeof(Block_Header),
                alignment
            )
            );

        destan_u64 padding = aligned_address - (reinterpret_cast<destan_u8*>(block) + sizeof(Block_Header));
        destan_u64 required_size = size + padding;

        // If there's enough space, split the block
        if (block->size - sizeof(Block_Header) >= required_size + MIN_BLOCK_SIZE + sizeof(Block_Header))
        {
            // Split the block, keeping track of the allocation block
            Block_Header* allocation_block = block;
            Block_Header* remainder_block = Split_Block(block, required_size + sizeof(Block_Header));

            // Update the free list
            Remove_From_Free_List(allocation_block);
            Add_To_Free_List(remainder_block);

            // Update the block for allocation
            block = allocation_block;
        }
        else
        {
            // Use the entire block
            Remove_From_Free_List(block);
        }

        // Mark the block as allocated
        block->is_free = false;

        // Update last allocated for FIND_NEXT strategy
        m_last_allocated = block;

#ifdef DESTAN_DEBUG
        // Update debug information
        block->allocation_id = s_next_allocation_id.fetch_add(1, std::memory_order_relaxed);
        m_allocation_count++;
#endif

        // Calculate user pointer with proper alignment
        void* result = aligned_address;

        Unlock();
        return result;
    }

    bool Free_List_Allocator::Deallocate(void* ptr)
    {
        if (!ptr)
        {
            return false;
        }

        Lock();

        // Find the block header from the user pointer
        Block_Header* block = Get_Block_Header(ptr);
        if (!block || block->is_free)
        {
            DESTAN_LOG_ERROR("Free List Allocator '{0}': Invalid pointer for deallocation: {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

#ifdef DESTAN_DEBUG
        // Validate block before deallocation
        if (!Validate_Block(block))
        {
            DESTAN_LOG_ERROR("Free List Allocator '{0}': Memory corruption detected at {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        // Update debug stats
        m_allocation_count--;

        // Fill memory with pattern to help catch use-after-free
        destan_u64 user_size = block->size - sizeof(Block_Header);
        Memory::Memset(ptr, 0xDD, user_size); // 0xDD = "Dead Dynamic memory"
#endif

        // Mark the block as free
        block->is_free = true;

        // Add to free list
        Add_To_Free_List(block);

        // Try to coalesce with adjacent blocks
        Coalesce(block);

        Unlock();
        return true;
    }

    void Free_List_Allocator::Set_Strategy(Allocation_Strategy strategy)
    {
        Lock();
        m_strategy = strategy;
        Unlock();
    }

    void Free_List_Allocator::Reset()
    {
        Lock();

        // Reset to a single free block
        m_first_block->size = m_size;
        m_first_block->is_free = true;
        m_first_block->next = nullptr;
        m_first_block->prev = nullptr;
        m_first_block->next_free = nullptr;
        m_first_block->prev_free = nullptr;

        // Reset the free list
        m_free_list = m_first_block;
        m_free_block_count = 1;
        m_last_allocated = nullptr;

#ifdef DESTAN_DEBUG
        // Reset debug counters
        m_allocation_count = 0;

        // Fill memory with pattern to help identify uninitialized memory
        destan_u64 user_size = m_size - sizeof(Block_Header);
        Memory::Memset(reinterpret_cast<destan_u8*>(m_memory_region) + sizeof(Block_Header),
            0xCD, user_size); // 0xCD = "Clean Dynamic memory"

        DESTAN_LOG_INFO("Free List Allocator '{0}' reset", m_name);
#endif

        Unlock();
    }

    destan_u64 Free_List_Allocator::Defragment()
    {
        Lock();

        destan_u64 coalesced_count = 0;
        Block_Header* current = m_first_block;

        // Simply coalesce all adjacent free blocks
        while (current && current->next)
        {
            if (current->is_free && current->next->is_free)
            {
                Block_Header* next_block = current->next;

                // Remove the next block from free list
                Remove_From_Free_List(next_block);

                // Combine blocks
                current->size += next_block->size;
                current->next = next_block->next;

                if (next_block->next)
                {
                    next_block->next->prev = current;
                }

                coalesced_count++;

                // No increment of current here, we want to check if we can coalesce more
            }
            else
            {
                current = current->next;
            }
        }

        if (coalesced_count > 0)
        {
            DESTAN_LOG_INFO("Free List Allocator '{0}': Defragmented {1} blocks",
                m_name, coalesced_count);
        }

        Unlock();
        return coalesced_count;
    }

    destan_u64 Free_List_Allocator::Get_Used_Size()
    {
        Lock();

        destan_u64 used_size = 0;
        Block_Header* current = m_first_block;

        while (current)
        {
            if (!current->is_free)
            {
                used_size += current->size;
            }
            current = current->next;
        }

        Unlock();
        return used_size;
    }

    destan_u64 Free_List_Allocator::Get_Free_Size()
    {
        return m_size - Get_Used_Size();
    }

    destan_u64 Free_List_Allocator::Get_Largest_Free_Block_Size()
    {
        Lock();

        destan_u64 largest_size = 0;
        Block_Header* current = m_free_list;

        while (current)
        {
            if (current->size > largest_size)
            {
                largest_size = current->size;
            }
            current = current->next_free;
        }

        Unlock();

        return largest_size > sizeof(Block_Header) ? largest_size - sizeof(Block_Header) : 0;
    }

    // Private helper methods
    Free_List_Allocator::Block_Header* Free_List_Allocator::Find_Suitable_Block(destan_u64 size, destan_u64 alignment)
    {
        // Ensure we account for alignment in size requirements
        destan_u64 max_adjustment = alignment - 1 + sizeof(Block_Header);
        destan_u64 adjusted_size = size + max_adjustment;

        // Use the appropriate strategy
        switch (m_strategy)
        {
        case Allocation_Strategy::FIND_FIRST:
            return Find_First_Fit(adjusted_size, alignment);

        case Allocation_Strategy::FIND_BEST:
            return Find_Best_Fit(adjusted_size, alignment);

        case Allocation_Strategy::FIND_NEXT:
            return Find_Next_Fit(adjusted_size, alignment);

        default:
            DESTAN_LOG_ERROR("Free List Allocator '{0}': Unknown allocation strategy", m_name);
            return Find_First_Fit(adjusted_size, alignment);
        }
    }

    //--------------------------------------------------------------------
    // Block Finding Strategies
    //--------------------------------------------------------------------
    Free_List_Allocator::Block_Header* Free_List_Allocator::Find_First_Fit(destan_u64 size, destan_u64 alignment)
    {
        Block_Header* current = m_free_list;

        while (current)
        {
            // Calculate aligned address
            destan_u8* block_start = reinterpret_cast<destan_u8*>(current) + sizeof(Block_Header);
            destan_u8* aligned_address = reinterpret_cast<destan_u8*>(
                Memory::Align_Address(block_start, alignment)
                );

            // Calculate adjustment needed for alignment
            destan_u64 adjustment = aligned_address - block_start;

            // Check if block is large enough including alignment adjustment
            if (current->size >= sizeof(Block_Header) + adjustment + size)
            {
                return current;
            }

            current = current->next_free;
        }

        return nullptr; // No suitable block found
    }

    Free_List_Allocator::Block_Header* Free_List_Allocator::Find_Best_Fit(destan_u64 size, destan_u64 alignment)
    {
        Block_Header* current = m_free_list;
        Block_Header* best_fit = nullptr;
        destan_u64 best_size = std::numeric_limits<destan_u64>::max();

        while (current)
        {
            // Calculate aligned address
            destan_u8* block_start = reinterpret_cast<destan_u8*>(current) + sizeof(Block_Header);
            destan_u8* aligned_address = reinterpret_cast<destan_u8*>(
                Memory::Align_Address(block_start, alignment)
                );

            // Calculate adjustment needed for alignment
            destan_u64 adjustment = aligned_address - block_start;

            // Check if block is large enough including alignment adjustment
            if (current->size >= sizeof(Block_Header) + adjustment + size)
            {
                // If this block is smaller than the current best fit, update best fit
                if (current->size < best_size)
                {
                    best_fit = current;
                    best_size = current->size;

                    // If we find a perfect fit, use it immediately
                    if (best_size == size + adjustment)
                    {
                        break;
                    }
                }
            }

            current = current->next_free;
        }

        return best_fit;
    }

    Free_List_Allocator::Block_Header* Free_List_Allocator::Find_Next_Fit(destan_u64 size, destan_u64 alignment)
    {
        // If we don't have a last allocated block or it's at the end, start from the beginning
        if (!m_last_allocated || !m_last_allocated->next)
        {
            return Find_First_Fit(size, alignment);
        }

        // Start search from the block after the last allocation
        Block_Header* start = m_last_allocated->next;
        Block_Header* current = start;

        // First, search from the last allocation to the end
        do
        {
            if (current->is_free)
            {
                // Calculate aligned address
                destan_u8* block_start = reinterpret_cast<destan_u8*>(current) + sizeof(Block_Header);
                destan_u8* aligned_address = reinterpret_cast<destan_u8*>(
                    Memory::Align_Address(block_start, alignment)
                    );

                // Calculate adjustment needed for alignment
                destan_u64 adjustment = aligned_address - block_start;

                // Check if block is large enough including alignment adjustment
                if (current->size >= sizeof(Block_Header) + adjustment + size)
                {
                    return current;
                }
            }

            current = current->next;
        } while (current && current != start);

        // If we didn't find a block, search from the beginning to the last allocation
        current = m_first_block;
        while (current && current != start)
        {
            if (current->is_free)
            {
                // Calculate aligned address
                destan_u8* block_start = reinterpret_cast<destan_u8*>(current) + sizeof(Block_Header);
                destan_u8* aligned_address = reinterpret_cast<destan_u8*>(
                    Memory::Align_Address(block_start, alignment)
                    );

                // Calculate adjustment needed for alignment
                destan_u64 adjustment = aligned_address - block_start;

                // Check if block is large enough including alignment adjustment
                if (current->size >= sizeof(Block_Header) + adjustment + size)
                {
                    return current;
                }
            }

            current = current->next;
        }

        return nullptr; // No suitable block found
    }

    //--------------------------------------------------------------------
    // Block Management
    //--------------------------------------------------------------------

    Free_List_Allocator::Block_Header* Free_List_Allocator::Split_Block(Block_Header* block, destan_u64 size)
    {
        // Check if we can actually split the block
        if (block->size < size + sizeof(Block_Header) + MIN_BLOCK_SIZE)
        {
            return nullptr;
        }

        // Calculate the position of the new block
        destan_u8* block_start = reinterpret_cast<destan_u8*>(block);
        destan_u8* new_block_start = block_start + size;
        Block_Header* new_block = reinterpret_cast<Block_Header*>(new_block_start);

        // Calculate the size of the new block
        destan_u64 new_block_size = block->size - size;

        // Initialize the new block
        new_block->size = new_block_size;
        new_block->is_free = true;
        new_block->next = block->next;
        new_block->prev = block;
        new_block->next_free = nullptr;
        new_block->prev_free = nullptr;

        // Update links
        if (block->next)
        {
            block->next->prev = new_block;
        }
        block->next = new_block;

        // Update the size of the original block
        block->size = size;

#ifdef DESTAN_DEBUG
        // Initialize debug values for the new block
        new_block->allocation_id = 0; // Not allocated yet
        new_block->file = nullptr;
        new_block->line = 0;
        new_block->guard_value = Block_Header::GUARD_PATTERN;

        // Fill the new block's memory with pattern
        destan_u64 user_size = new_block_size - sizeof(Block_Header);
        Memory::Memset(new_block_start + sizeof(Block_Header), 0xCD, user_size); // 0xCD = "Clean Dynamic memory"
#endif

        // Track free block count
        m_free_block_count++;

        return new_block;
    }

    void Free_List_Allocator::Remove_From_Free_List(Block_Header* block)
    {
        if (!block->is_free)
        {
            return;
        }

        // Update the previous free block
        if (block->prev_free)
        {
            block->prev_free->next_free = block->next_free;
        }
        else if (m_free_list == block)
        {
            // This is the head of the free list
            m_free_list = block->next_free;
        }

        // Update the next free block
        if (block->next_free)
        {
            block->next_free->prev_free = block->prev_free;
        }

        // Clear free list pointers
        block->next_free = nullptr;
        block->prev_free = nullptr;

        // Decrement free block count
        m_free_block_count--;
    }

    void Free_List_Allocator::Add_To_Free_List(Block_Header* block)
    {
        if (!block || !block->is_free)
        {
            return;
        }

        // Add to the head of the free list for simplicity
        block->next_free = m_free_list;
        block->prev_free = nullptr;

        if (m_free_list)
        {
            m_free_list->prev_free = block;
        }

        m_free_list = block;

        // Increment free block count
        m_free_block_count++;
    }

    Free_List_Allocator::Block_Header* Free_List_Allocator::Coalesce(Block_Header* block)
    {
        if (!block || !block->is_free)
        {
            return block;
        }

        // Try to coalesce with the next block
        if (block->next && block->next->is_free)
        {
            Block_Header* next_block = block->next;

            // Remove both blocks from free list
            Remove_From_Free_List(block);
            Remove_From_Free_List(next_block);

            // Combine blocks
            block->size += next_block->size;
            block->next = next_block->next;

            if (next_block->next)
            {
                next_block->next->prev = block;
            }

            // Add the coalesced block back to free list
            Add_To_Free_List(block);

#ifdef DESTAN_DEBUG
            DESTAN_LOG_TRACE("Free List Allocator '{0}': Coalesced block with next block", m_name);
#endif
        }

        // Try to coalesce with the previous block
        if (block->prev && block->prev->is_free)
        {
            Block_Header* prev_block = block->prev;

            // Remove both blocks from free list
            Remove_From_Free_List(block);
            Remove_From_Free_List(prev_block);

            // Combine blocks
            prev_block->size += block->size;
            prev_block->next = block->next;

            if (block->next)
            {
                block->next->prev = prev_block;
            }

            // Add the coalesced block back to free list
            Add_To_Free_List(prev_block);

            // Update our return value to the previous block
            block = prev_block;

#ifdef DESTAN_DEBUG
            DESTAN_LOG_TRACE("Free List Allocator '{0}': Coalesced block with previous block", m_name);
#endif
        }

        return block;
    }

    //--------------------------------------------------------------------
    // Memory Validation
    //--------------------------------------------------------------------

    bool Free_List_Allocator::Validate_Pointer(void* ptr) const
    {
        if (!ptr)
        {
            return false;
        }

        // Check if the pointer is within our memory region
        destan_u8* memory_start = static_cast<destan_u8*>(m_memory_region);
        destan_u8* memory_end = memory_start + m_size;
        destan_u8* user_ptr = static_cast<destan_u8*>(ptr);

        if (user_ptr < memory_start || user_ptr >= memory_end)
        {
            return false;
        }

        // Find the block that contains this pointer
        Block_Header* block = Get_Block_Header(ptr);

        // Validate the block
        if (!block || block->is_free)
        {
            return false;
        }

        return true;
    }

    Free_List_Allocator::Block_Header* Free_List_Allocator::Get_Block_Header(void* ptr) const
    {
        if (!ptr)
        {
            return nullptr;
        }

        // Cast to byte pointer for pointer arithmetic
        destan_u8* user_ptr = static_cast<destan_u8*>(ptr);

        // Scan all blocks to find the one containing this pointer
        Block_Header* current = m_first_block;
        while (current)
        {
            // Calculate block bounds
            destan_u8* block_start = reinterpret_cast<destan_u8*>(current);
            destan_u8* block_end = block_start + current->size;

            // Check if the pointer is within this block
            if (user_ptr >= block_start + sizeof(Block_Header) && user_ptr < block_end)
            {
                return current;
            }

            current = current->next;
        }

        return nullptr;
    }

#ifdef DESTAN_DEBUG
    //--------------------------------------------------------------------
    // Debug Helpers
    //--------------------------------------------------------------------

    bool Free_List_Allocator::Validate_Block(Block_Header* block) const
    {
        if (!block)
        {
            return false;
        }

        // Check guard pattern
        if (block->guard_value != Block_Header::GUARD_PATTERN)
        {
            DESTAN_LOG_ERROR("Free List Allocator '{0}': Memory corruption detected in block header", m_name);
            return false;
        }

        // Check if block is within our memory region
        destan_u8* memory_start = static_cast<destan_u8*>(m_memory_region);
        destan_u8* memory_end = memory_start + m_size;
        destan_u8* block_start = reinterpret_cast<destan_u8*>(block);
        destan_u8* block_end = block_start + block->size;

        if (block_start < memory_start || block_end > memory_end)
        {
            DESTAN_LOG_ERROR("Free List Allocator '{0}': Block is outside memory region", m_name);
            return false;
        }

        // Validate block size
        if (block->size < sizeof(Block_Header) || block->size > m_size)
        {
            DESTAN_LOG_ERROR("Free List Allocator '{0}': Invalid block size: {1}", m_name, block->size);
            return false;
        }

        // Validate block links
        if (block->next)
        {
            destan_u8* next_block_start = reinterpret_cast<destan_u8*>(block->next);

            // Next block should be after this block
            if (next_block_start != block_start + block->size)
            {
                DESTAN_LOG_ERROR("Free List Allocator '{0}': Block links are corrupted", m_name);
                return false;
            }

            // Next block should have correct prev pointer
            if (block->next->prev != block)
            {
                DESTAN_LOG_ERROR("Free List Allocator '{0}': Block links are corrupted", m_name);
                return false;
            }
        }

        if (block->prev)
        {
            destan_u8* prev_block_start = reinterpret_cast<destan_u8*>(block->prev);
            destan_u8* prev_block_end = prev_block_start + block->prev->size;

            // This block should be after prev block
            if (block_start != prev_block_end)
            {
                DESTAN_LOG_ERROR("Free List Allocator '{0}': Block links are corrupted", m_name);
                return false;
            }
        }

        return true;
    }

    bool Free_List_Allocator::Validate_Free_List() const
    {
        Block_Header* current = m_free_list;
        destan_u64 count = 0;

        // Check forward links
        while (current)
        {
            // Each block in the free list should be marked as free
            if (!current->is_free)
            {
                DESTAN_LOG_ERROR("Free List Allocator '{0}': Block in free list is not marked as free", m_name);
                return false;
            }

            // Validate the block itself
            if (!Validate_Block(current))
            {
                return false;
            }

            // Check next_free link
            if (current->next_free && current->next_free->prev_free != current)
            {
                DESTAN_LOG_ERROR("Free List Allocator '{0}': Free list links are corrupted", m_name);
                return false;
            }

            current = current->next_free;
            count++;

            // Detect cycles
            if (count > m_free_block_count)
            {
                DESTAN_LOG_ERROR("Free List Allocator '{0}': Cycle detected in free list", m_name);
                return false;
            }
        }

        // Check if counted blocks match tracked count
        if (count != m_free_block_count)
        {
            DESTAN_LOG_ERROR("Free List Allocator '{0}': Free block count mismatch: counted {1}, tracked {2}",
                m_name, count, m_free_block_count);
            return false;
        }

        return true;
    }

    void* Free_List_Allocator::Allocate_Debug(destan_u64 size, destan_u64 alignment, const destan_char* file, int line)
    {
        void* ptr = Allocate(size, alignment);

        if (ptr)
        {
            // Find the block and update debug info
            Block_Header* block = Get_Block_Header(ptr);
            if (block)
            {
                block->file = file;
                block->line = line;
            }
        }

        return ptr;
    }

    void Free_List_Allocator::Dump_Stats()
    {
        Lock();

        std::stringstream ss;
        ss << "===== Free List Allocator '" << m_name << "' Stats =====" << std::endl;
        ss << "Size: " << m_size << " bytes (" << (m_size / 1024.0f) << " KB)" << std::endl;
        ss << "Used: " << Get_Used_Size() << " bytes (" << (Get_Used_Size() / 1024.0f) << " KB)" << std::endl;
        ss << "Free: " << Get_Free_Size() << " bytes (" << (Get_Free_Size() / 1024.0f) << " KB)" << std::endl;
        ss << "Free Blocks: " << m_free_block_count << std::endl;
        ss << "Largest Free Block: " << Get_Largest_Free_Block_Size() << " bytes" << std::endl;
        ss << "Utilization: " << Get_Utilization() << "%" << std::endl;
        ss << "Allocation Strategy: ";

        switch (m_strategy)
        {
        case Allocation_Strategy::FIND_FIRST:
            ss << "First Fit";
            break;
        case Allocation_Strategy::FIND_BEST:
            ss << "Best Fit";
            break;
        case Allocation_Strategy::FIND_NEXT:
            ss << "Next Fit";
            break;
        default:
            ss << "Unknown";
            break;
        }
        ss << std::endl;

        // In debug mode, show detailed allocation information
        ss << "\nAllocations: " << m_allocation_count << std::endl;

        if (m_allocation_count > 0)
        {
            ss << "--------------------------------------------------" << std::endl;
            ss << "   Size   |    Address    | Source Location" << std::endl;
            ss << "--------------------------------------------------" << std::endl;

            const destan_u64 MAX_ALLOCS_TO_SHOW = 20; // Limit for readability
            destan_u64 allocs_shown = 0;

            Block_Header* current = m_first_block;
            while (current && allocs_shown < MAX_ALLOCS_TO_SHOW)
            {
                if (!current->is_free)
                {
                    destan_u8* user_ptr = reinterpret_cast<destan_u8*>(current) + sizeof(Block_Header);
                    destan_u64 user_size = current->size - sizeof(Block_Header);

                    ss << "  " << std::setw(7) << user_size << " | "
                        << std::hex << std::setw(12) << static_cast<void*>(user_ptr) << std::dec << " | ";

                    if (current->file)
                    {
                        ss << current->file << ":" << current->line;
                    }
                    else
                    {
                        ss << "unknown location";
                    }

                    ss << std::endl;
                    allocs_shown++;
                }

                current = current->next;
            }

            if (m_allocation_count > MAX_ALLOCS_TO_SHOW)
            {
                ss << "... and " << (m_allocation_count - MAX_ALLOCS_TO_SHOW)
                    << " more allocations" << std::endl;
            }
        }

        ss << "==============================================";
        DESTAN_LOG_INFO("{}", ss.str());

        Unlock();
    }

    void Free_List_Allocator::Dump_Fragmentation_Map()
    {
        Lock();

        std::stringstream ss;
        ss << "Memory Fragmentation Map for '" << m_name << "':" << std::endl;
        ss << "[";

        // Create a visual representation of memory fragmentation
        const int MAP_WIDTH = 80; // Width of the visual map
        const destan_u64 BYTES_PER_CHAR = m_size / MAP_WIDTH;

        destan_u8* memory_start = static_cast<destan_u8*>(m_memory_region);

        for (int i = 0; i < MAP_WIDTH; i++)
        {
            destan_u64 pos = i * BYTES_PER_CHAR;
            destan_u8* current_pos = memory_start + pos;

            // Find the block containing this position
            Block_Header* current = m_first_block;
            bool found = false;

            while (current && !found)
            {
                destan_u8* block_start = reinterpret_cast<destan_u8*>(current);
                destan_u8* block_end = block_start + current->size;

                if (current_pos >= block_start && current_pos < block_end)
                {
                    // Position is within this block
                    ss << (current->is_free ? "." : "#");
                    found = true;
                }

                current = current->next;
            }

            if (!found)
            {
                // Should never happen if our block list is correct
                ss << "?";
            }
        }

        ss << "]" << std::endl;
        ss << "Legend: # = Allocated, . = Free" << std::endl;

        DESTAN_LOG_INFO("{}", ss.str());

        Unlock();
    }
#endif
}
    