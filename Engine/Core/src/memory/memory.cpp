#include <core/destan_pch.h>
#include <core/memory/memory.h>

namespace destan::core::memory
{
	// Initialize static members using atomic types for lock-free access
	std::atomic<Memory_Init_State> Memory::s_init_state{Memory_Init_State::Uninitialized};
	std::atomic<destan_u64> Memory::s_total_allocated{0};
	std::atomic<destan_u64> Memory::s_total_freed{0};
	std::atomic<destan_u64> Memory::s_allocation_count{0};

	// Mutex for operations that require exclusive access
	// We only use this when atomic operations aren't sufficient
	static std::mutex s_memory_mutex;

	// Debug tracking variables
#ifdef DESTAN_DEBUG
	static std::atomic<destan_u64> s_next_allocation_id{1};
	static constexpr destan_u32 FOOTER_GUARD_PATTERN = 0xCDCDCDCD;
#endif

	// Thread-local allocator data
	// This is a key optimization for high-performance
	// Each thread gets its own small allocator, reducing contention
	thread_local struct Thread_Local_Allocator
	{
		// Small memory blocks for quick allocation (no locks needed)
		static constexpr destan_u64 BLOCK_SIZE = 4096; // 4KB Blocks
		static constexpr destan_u64 MAX_BLOCKS = 16;   // Max 16 blocks per thread

		struct Block
		{
			char data[BLOCK_SIZE];
			destan_u64 offset = 0;
		};

		Block* blocks[MAX_BLOCKS] = {nullptr};
		destan_u64 block_count = 0;
		destan_u64 allocations = 0;

		// Cleanup on thread exit
		~Thread_Local_Allocator()
		{
			for (destan_u64 i = 0; i < block_count; i++)
			{
				if (blocks[i])
				{
					std::free(blocks[i]);
					blocks[i] = nullptr;
				}
			}
			block_count = 0;
		}

		// Try to allocate from thread-local storage
		void* Allocate(destan_u64 size, destan_u64 alignment)
		{
			// If allocation is too large, don't use thread-local storage
			if (size > BLOCK_SIZE / 2)
			{
				return nullptr;
			}

			// Try to find a block with enough space
			for (destan_u64 i = 0; i < block_count; i++)
			{
				Block* block = blocks[i];

				// Calculate aligned address
				destan_u64 current = reinterpret_cast<destan_u64>(block->data + block->offset);
				destan_u64 aligned = (current + alignment - 1) & ~(alignment - 1);
				destan_u64 adjustment = aligned - current;

				// Check if there's enough space
				if (block->offset + adjustment + size <= BLOCK_SIZE)
				{
					block->offset += adjustment + size;
					allocations++;
					return reinterpret_cast<void*>(aligned);
				}
			}

			// No suitable block found, allocate a new one if possible
			if (block_count < MAX_BLOCKS)
			{
				Block* new_block = static_cast<Block*>(std::malloc(sizeof(Block)));
				if (!new_block)
				{
					DESTAN_LOG_ERROR("Thread-local allocator ran out of memory!");
					return nullptr;
				}

				new_block->offset = 0;
				blocks[block_count++] = new_block;

				// Now allocate from the new block
				destan_u64 current = reinterpret_cast<destan_u64>(new_block->data);
				destan_u64 aligned = (current + alignment - 1) & ~(alignment - 1);
				destan_u64 adjustment = aligned - current;

				new_block->offset = adjustment + size;
				allocations++;
				return reinterpret_cast<void*>(aligned);
			}

			DESTAN_LOG_ERROR("Could not allocate from thread-local storage!");
			return nullptr;
		}

		// We don't actually free anything from thread-local storage
		// This is a key optimization for high-performance - we reuse the same memory
		// within the tread. This approach is commonly used in high-performance systems
		void Free(void* ptr)
		{
			// Check if the pointer is within any of the blocks
			for (destan_u64 i = 0; i < block_count; i++)
			{
				if (ptr >= blocks[i]->data && ptr < blocks[i]->data + BLOCK_SIZE)
				{
					// Do nothing - we don't actually free anything
					allocations--;
					return;
				}
			}

			DESTAN_LOG_ERROR("Trying to free a pointer that was not allocated by this thread!");
		}

		// Reset all blocks (keep them allocated but mark as empty)
		void Reset()
		{
			for (destan_u64 i = 0; i < block_count; i++)
			{
				blocks[i]->offset = 0;
			}
			allocations = 0;
		}

	} *s_thread_local_allocator = nullptr;


	void Memory::Initialize()
	{
		// Try atomically change state from Uninitialized to Initializing
		Memory_Init_State expected = Memory_Init_State::Uninitialized;
		if (!s_init_state.compare_exchange_strong(expected, Memory_Init_State::Initializing,
			std::memory_order_acquire, std::memory_order_relaxed))
		{
			return; // Already initialized or in the process of initialization
		}

		// Reset statistics - using atomic store operations
		s_total_allocated.store(0, std::memory_order_relaxed);
		s_total_freed.store(0, std::memory_order_relaxed);
		s_allocation_count.store(0, std::memory_order_relaxed);

		// Initialize tracking systems
		Initialize_Tracking();

		// Initialize thread-local storage
		Initialize_Thread_Local_Storage();

		// State transition to Initialized - full memory barrier
		s_init_state.store(Memory_Init_State::Initialized, std::memory_order_release);
	}

	void Memory::Shutdown()
	{
		// Try atomically change state from Initializedto Shutting_Down
		Memory_Init_State expected = Memory_Init_State::Initialized;
		if (!s_init_state.compare_exchange_strong(expected, Memory_Init_State::Shutting_Down,
			std::memory_order_acquire, std::memory_order_relaxed))
		{
			return; // Not initialized or already shutting down
		}

		// Check for leaks before shutting down
		Check_Memory_Leaks();

		// Shutdown tracking systems
		Shutdown_Tracking();

		// Shutdown thread-local storage
		Shutdown_Thread_Local_Storage();

		// State transition to Uninitialized - full memory barrier
		s_init_state.store(Memory_Init_State::Uninitialized, std::memory_order_release);
	}

	bool Memory::Is_Initialized()
	{
		return s_init_state.load(std::memory_order_relaxed) == Memory_Init_State::Initialized;
	}

	void* Memory::Malloc(destan_u64 size, destan_u64 alignment)
	{
		// Validate inputs
		DESTAN_ASSERT(size > 0, "Malloc called with size = 0!");

		if (alignment == 0)
		{
			alignment = DEFAULT_ALIGNMENT;
		}

		// Ensure alignment is a power of 2
		if ((alignment & (alignment - 1)) != 0)
		{
			// Round up to the next power of 2
			DESTAN_LOG_WARN("Alignment is not a power of 2! Rounding up...");
			alignment--;
			alignment |= alignment >> 1;
			alignment |= alignment >> 2;
			alignment |= alignment >> 4;
			alignment |= alignment >> 8;
			alignment |= alignment >> 16;
			alignment++;
			return nullptr;
		}

		// First, try thread-local allocator for small allocations
		// This is our fast path with no locks
		void* ptr = Thread_Local_Malloc(size, alignment);
		if (ptr)
		{
			// Update global statistics atomically
			s_total_allocated.fetch_add(size, std::memory_order_relaxed);
			s_allocation_count.fetch_add(1, std::memory_order_relaxed);
			return ptr;
		}

#ifdef DESTAN_DEBUG
		// In debug mode, add space for header and footer
		const destan_u64 header_size = sizeof(Allocation_Header);
		const destan_u64 footer_size = sizeof(destan_u32); // Guard pattern at the end

		// Calculate total size with alignment considerations
		const destan_u64 total_size = header_size + Align_Size(size, alignment) + footer_size;

		// Allocate memory block
		void* memory_block = std::malloc(total_size);
		if (!memory_block)
		{
			DESTAN_LOG_ERROR("Failed to allocate memory!");
			return nullptr;
		}

		// Prepare the header
		Allocation_Header* header = reinterpret_cast<Allocation_Header*>(memory_block);
		header->size = size;
		header->alignment = static_cast<destan_u32>(alignment);
		header->file = ""; // Will be filled by Malloc_Debug if called
		header->line = 0; // Will be filled by Malloc_Debug if called
		header->allocation_id = s_next_allocation_id.fetch_add(1, std::memory_order_relaxed);
		header->guard_value = Allocation_Header::GUARD_PATTERN;

		// Calculate aligned user pointer
		void* user_ptr = reinterpret_cast<void*>(
			Align_Size(reinterpret_cast<destan_uiptr>(header) + header_size, alignment));

		// Prepare the footer
		destan_u32* footer = reinterpret_cast<destan_u32*>(
			reinterpret_cast<destan_u8*>(user_ptr) + size);
		*footer = FOOTER_GUARD_PATTERN;

#else
		// In release mode, just use aligned_alloc or equivalent
		void* user_ptr = std::aligned_alloc(alignment, Align_Size(size, alignment));
		if (!user_ptr)
		{
			DESTAN_LOG_ERROR("Failed to allocate memory!");
			return nullptr;
		}
#endif

		// Update statistics using atomics for lock-free operations
		s_total_allocated.fetch_add(size, std::memory_order_relaxed);
		s_allocation_count.fetch_add(1, std::memory_order_relaxed);

		return user_ptr;
	}

	void Memory::Free(void* ptr)
	{
		if (!ptr)
		{
			return;
		}

		// Try thread-local free first
		Thread_Local_Free(ptr);

#ifdef DESTAN_DEBUG		
		// Get the header from the user pointer
		Allocation_Header* header = Get_Header(ptr);

		// If not found in thread-local storage and not a valid header,
		// it might be from a different allocator or corrupted
		if (!header)
		{
			DESTAN_LOG_ERROR("Trying to free a pointer that was not allocated by this allocator!");
			return;
		}

		// Validate the header
		if (!Validate_Header(header))
		{
			DESTAN_LOG_ERROR("Memory corruption detected while freeing the memory at {0}!", ptr);
		}

		// Validate the footer
		destan_u32* footer = reinterpret_cast<destan_u32*>(reinterpret_cast<destan_u8*>(ptr) + header->size);

		if (*footer != FOOTER_GUARD_PATTERN)
		{
			DESTAN_LOG_ERROR("Memory corruption detected while freeing the memory at {0}!", ptr);
		}

		// Clear the memory to help catch use-after-free bugs
		std::memset(ptr, 0xDD, header->size);

		// Update statistics atomically before freeing the memory
		s_total_freed.fetch_add(header->size, std::memory_order_relaxed);
		s_allocation_count.fetch_sub(1, std::memory_order_relaxed);

		// Free the actual memory block
		std::free(header);
#else
		// In release mode, just free the pointer
		std::free(ptr);

		// Since we don't know the size in release mode
		// we only decrement the allocation count
		s_allocation_count.fetch_sub(1, std::memory_order_relaxed);
#endif
	}

	void* Memory::Realloc(void* ptr, destan_u64 new_size, destan_u64 alignment)
	{
		if (new_size == 0)
		{
			Free(ptr);
			return nullptr;
		}

		if (!ptr)
		{
			return Malloc(new_size, alignment);
		}

#ifdef DESTAN_DEBUG
		// Get current allocation size
		destan_u64 current_size = Get_Allocation_Size(ptr);

		// Allocate new block
		void* new_ptr = Malloc(new_size, alignment);
		if (!new_ptr)
		{
			DESTAN_LOG_ERROR("Failed to reallocate memory!");
			return nullptr;
		}

		// Copy data from old to new
		std::memcpy(new_ptr, ptr, (current_size < new_size) ? current_size : new_size);

		// Free old memory
		Free(ptr);

		return new_ptr;
#else
		// In release mode, we don't have size information
		// so we need to allocate new memory and copy
		void* new_ptr = Malloc(new_size, alignment);
		if (!new_ptr)
		{
			DESTAN_LOG_ERROR("Failed to reallocate memory!");
			return nullptr;
		}

		// We don't know how much to copy, so we'll assume new_size
		// This could potentially lead to data loss if new_size is smaller than the original size
		std::memcpy(new_ptr, ptr, new_size);

		// Free old memory
		Free(ptr);

		return new_ptr;
#endif
	}

	void* Memory::Thread_Local_Malloc(destan_u64 size, destan_u64 alignment)
	{
		// Ensure thread-local allocator is initialized
		if (!s_thread_local_allocator)
		{
			s_thread_local_allocator = new Thread_Local_Allocator();
		}

		// Try to allocate from thread-local storage
		return s_thread_local_allocator->Allocate(size, alignment);
	}

	void Memory::Thread_Local_Free(void* ptr)
	{
		// If the thread-local allocator exists, try to free
		if (s_thread_local_allocator)
		{
			s_thread_local_allocator->Free(ptr);
		}
	}

	void* Memory::Memmove(void* dest, const void* src, destan_u64 size)
	{
		return std::memmove(dest, src, size);
	}

	void* Memory::Memcpy(void* dest, const void* src, destan_u64 size)
	{
		return std::memcpy(dest, src, size);
	}

	void* Memory::Memset(void* dest, destan_i32 value, destan_u64 size)
	{
		return std::memset(dest, value, size);
	}

	destan_i32 Memory::Memcmp(const void* ptr1, const void* ptr2, destan_u64 size)
	{
		return std::memcmp(ptr1, ptr2, size);
	}

	destan_u64 Memory::Get_Allocation_Size(void* ptr)
	{
		if (!ptr)
		{
			return 0;
		}

#ifdef DESTAN_DEBUG
		// In debug mode, we can get the exact size from the header
		Allocation_Header* header = Get_Header(ptr);

		// Validate the header first
		if (!header || !Validate_Header(header))
		{
			DESTAN_LOG_ERROR("Memory corruption detected while getting the size of the memory at {0}!", ptr);
			return 0;
		}

		return header->size;
#else
		// In release mode, we cannot get the exact size
		return 0;
#endif
	}

	destan_u64 Memory::Get_Total_Allocated()
	{
		// No lock needed - atomic operation
		return s_total_allocated.load(std::memory_order_relaxed);
	}

	destan_u64 Memory::Get_Total_Freed()
	{
		// No lock needed - atomic operation
		return s_total_freed.load(std::memory_order_relaxed);
	}

	destan_u64 Memory::Get_Allocation_Count()
	{
		// No lock needed - atomic operation
		return s_allocation_count.load(std::memory_order_relaxed);
	}

	destan_u64 Memory::Get_Current_Used_Memory()
	{
		// No lock needed - atomic operation
		return s_total_allocated.load(std::memory_order_relaxed) - s_total_freed.load(std::memory_order_relaxed);
	}

	void Memory::Dump_Memory_Stats()
	{
		// Use atomic loads to get consistent state without locking
		destan_u64 total_allocated = s_total_allocated.load(std::memory_order_relaxed);
		destan_u64 total_freed = s_total_freed.load(std::memory_order_relaxed);
		destan_u64 allocation_count = s_allocation_count.load(std::memory_order_relaxed);

		std::stringstream ss;
		ss << "========== Memory Stats ==========" << std::endl;
		ss << "Total Allocated: " << total_allocated << " bytes ("
		   << (total_allocated / 1024.0f / 1024.0f) << " MB)" << std::endl;
		ss << "Total Freed: " << total_freed << " bytes ("
		   << (total_freed / 1024.0f / 1024.0f) << " MB)" << std::endl;
		ss << "Current Used Memory: " << (total_allocated - total_freed)
		   << " bytes (" << ((total_allocated - total_freed) / 1024.0f / 1024.0f) << " MB)" << std::endl;
		ss << "Active Allocations: " << allocation_count << std::endl;

		// Thread-local allocator stats
		if (s_thread_local_allocator)
		{
			ss << "Thread-local Allocator:" << std::endl;
			ss << "  Active Blocks: " << s_thread_local_allocator->block_count << std::endl;
			ss << "  Active Allocations: " << s_thread_local_allocator->allocations << std::endl;
		}
		ss << "=================================" << std::endl;
		DESTAN_LOG_INFO("{}", ss.str());
	}

	void Memory::Check_Memory_Leaks()
	{
		// Atomic load for consistent state
		destan_u64 allocation_count = s_allocation_count.load(std::memory_order_relaxed);

		if (allocation_count > 0)
		{
			destan_u64 total_allocated = s_total_allocated.load(std::memory_order_relaxed);
			destan_u64 total_freed = s_total_freed.load(std::memory_order_relaxed);

			std::stringstream ss;
			ss << "MEMORY LEAK DETECTED: " << allocation_count
			   << " allocations still active" << std::endl;
			ss << "Leaked memory: " << (total_allocated - total_freed)
			   << " bytes" << std::endl;
		}
	}

	void Memory::Initialize_Tracking() {
		// Initialize tracking systems
		// In a more complex implementation, this would initialize
		// data structures for advanced memory tracking
	}

	void Memory::Shutdown_Tracking() {
		// Shutdown tracking systems
		// In a more complex implementation, this would clean up
		// tracking data structures
	}

	void Memory::Initialize_Thread_Local_Storage() {
		// Nothing to do here, thread-local storage is initialized on first use
	}

	void Memory::Shutdown_Thread_Local_Storage() {
		// Nothing to do here, thread-local storage is cleaned up when threads exit
	}

#ifdef DESTAN_DEBUG
	void* Memory::Get_User_Pointer(Allocation_Header* header)
	{
		// Calculate the user pointer based on the header
		const destan_u64 header_size = sizeof(Allocation_Header);
		return reinterpret_cast<void*>(Align_Size(reinterpret_cast<destan_uiptr>(header) + header_size, header->alignment));
	}

	Memory::Allocation_Header* Memory::Get_Header(void* user_ptr)
	{
		// Search backwards for the header
		destan_uiptr max_backwards_offset = sizeof(Allocation_Header) + DEFAULT_ALIGNMENT;
		destan_uiptr user_addr = reinterpret_cast<destan_uiptr>(user_ptr);

		// Start from a reasonable offset and search backwards
		for (destan_uiptr offset = 0; offset <= max_backwards_offset; offset += sizeof(void*))
		{
			if (offset > user_addr)
			{
				break; // Prevent underflow
			}

			destan_uiptr potential_header_addr = user_addr - offset;
			Allocation_Header* potential_header = reinterpret_cast<Allocation_Header*>(potential_header_addr);

			// Check if this looks like a valid pointer
			if (potential_header->guard_value == Allocation_Header::GUARD_PATTERN)
			{
				// Calculate what the user pointer should be
				void* calculated_user_ptr = Get_User_Pointer(potential_header);

				// If calculated_user_ptr matches user_ptr, we found the header
				if (calculated_user_ptr == user_ptr)
				{
					return potential_header;
				}
			}
		}

		// Couldn't find a valid header
		return nullptr;
	}

	bool Memory::Validate_Header(Allocation_Header* header)
	{
		if (!header)
		{
			return false;
		}

		// Check guard value
		if (header->guard_value != Allocation_Header::GUARD_PATTERN)
		{
			return false;
		}

		// Additional validation could be done here
		return true;
	}

	void* Memory::Malloc_Debug(destan_u64 size, destan_u64 alignment, const char* file, int line)
	{
		void* ptr = Malloc(size, alignment);
		if (ptr)
		{
			// Update debug info in the header
			Allocation_Header* header = Get_Header(ptr);
			if (header)
			{
				header->file = file;
				header->line = line;
			}
		}

		return ptr;
	}

	void Memory::Free_Debug(void* ptr)
	{
		// Additional debug checks could be done here
		Free(ptr);
	}
#endif

} // namespace destan::core::memory