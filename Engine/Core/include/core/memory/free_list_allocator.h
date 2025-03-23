#pragma once
#include <core/memory/memory.h>
#include <core/defines.h>


namespace destan::core::memory
{
	/**
	 *  Free List Allocator
	 * 
	 * A general-purpose memory allocator that manages variable-sized
	 * allocations using a list of free blocks. Ideal for systems requiring
	 * frequent allocations and deallocations of varying sizes.
	 * 
	 * Key Characteristics:
	 * 
	 * - Handles variable-sized allocations efficiently
	 * - Prevents excessive external fragmentation
	 * - Supports different allocation strategies for optimization
	 * - Tracks memory usage and can coalesce adjacent free blocks
	 * - Thread-safe option available for concurrent access
	 */
	class Free_List_Allocator
	{
	public:
		/**
		 * Allocation strategy to use when searching for a suitable memory block
		 */
		enum class Allocation_Strategy
		{
			FIND_FIRST, // Use the first block that fits (fastest allocation, highest fragmentation)
			FIND_BEST,  // Use the smallest block that fits the requested size (slower allocation, lowest fragmentation)
			FIND_NEXT,  // Use the next block after the last allocation (good temporal locality)
		};

		/**
		 * Creates a free list allocator with the specified total size
		 *
		 * @param size_bytes Total size of the memory region in bytes
		 * @param strategy Allocation strategy to use
		 * @param name Optional name for debugging purposes
		 */
		Free_List_Allocator(destan_u64 size_bytes,
			Allocation_Strategy strategy = Allocation_Strategy::FIND_BEST,
			const destan_char* name = "Free_List");

		/**
		 * Destructor releases the entire memory region
		 */
		~Free_List_Allocator();

		// Disable copying to prevent double-free issues
		Free_List_Allocator(const Free_List_Allocator&) = delete;
		Free_List_Allocator& operator=(const Free_List_Allocator&) = delete;

		// Move operations
		Free_List_Allocator(Free_List_Allocator&& other) noexcept;
		Free_List_Allocator& operator=(Free_List_Allocator&& other) noexcept;

		/**
		 * Allocates memory of the specified size and alignment
		 *
		 * @param size Size of the allocation in bytes
		 * @param alignment Memory alignment (defaults to DEFAULT_ALIGNMENT)
		 * @return Pointer to the allocated memory or nullptr if allocation failed
		 */
		void* Allocate(destan_u64 size, destan_u64 alignment = DEFAULT_ALIGNMENT);

		/**
		 * Allocates and constructs an object of Type T
		 *
		 * @tparam T Type of object to construct
		 * @tparam Args Types of constructor arguments
		 * @param args Arguments to forward to the constructor
		 * @return Pointer to the constructed object or nullptr if allocation failed
		 */
		template<typename T, typename... Args>
		T* Create(Args&&... args)
		{
			void* memory = Allocate(sizeof(T), alignof(T));
			if (!memory)
			{
				return nullptr;
			}
			// Placement new to construct the object
			return new (memory) T(std::forward<Args>(args)...);
		}

		/**
		 * Deallocates memory, returning it to the free list
		 * May coalesce with adjacent free blocks to reduce fragmentation
		 *
		 * @param ptr Pointer to previously allocated memory
		 * @return True if deallocation succeeded, false if ptr was invalid
		 */
		bool Deallocate(void* ptr);

		/**
		 * Destroys an object and deallocates its memory
		 *
		 * @tparam T Type of object to destroy
		 * @param ptr Pointer to the object
		 * @return True if deallocation was successful, false if ptr was invalid
		 */
		template<typename T>
		bool Destroy(T* ptr)
		{
			if (!ptr)
			{
				return false;
			}

			// Call the destructor
			ptr->~T();

			// Deallocate the memory
			return Deallocate(ptr);
		}

		/**
		 * Changes the allocation strategy
		 *
		 * @param strategy New allocation strategy to use
		 */
		void Set_Strategy(Allocation_Strategy strategy);

		/**
		 * Returns the current allocation strategy
		 */
		Allocation_Strategy Get_Strategy() const { return m_strategy; }

		/**
		 * Resets the allocator, marking all memory as a single free block
		 * Note: This does not call destructors on any remaining objects!
		 */
		void Reset();

		/**
		 * Forces defragmentation of the free list
		 * This is an expensive operation that reorganizes memory to reduce fragmentation
		 *
		 * @return Number of blocks that were coalesced
		 */
		destan_u64 Defragment();

		/**
		 * Returns the total size of the memory region in bytes
		 */
		destan_u64 Get_Size() const { return m_size; }

		/**
		 * Returns the current used size in bytes
		 */
		destan_u64 Get_Used_Size();

		/**
		 * Returns the remaining free size in bytes
		 */
		destan_u64 Get_Free_Size();

		/**
		 * Returns the number of free blocks in the free list
		 */
		destan_u64 Get_Free_Block_Count() const { return m_free_block_count; }

		/**
		 * Returns the memory utilization percentage
		 */
		destan_f32 Get_Utilization()
		{
			return static_cast<destan_f32>(Get_Used_Size()) / static_cast<destan_f32>(m_size) * 100.0f;
		}

		/**
		 * Returns the name of this allocator
		 */
		const destan_char* Get_Name() const { return m_name; }

		/**
		 * Returns the largest free block size
		 */
		destan_u64 Get_Largest_Free_Block_Size();

#ifdef DESTAN_DEBUG
		/**
		 * Debug version of Allocate that tracks the allocation
		 */
		void* Allocate_Debug(destan_u64 size, destan_u64 alignment, const destan_char* file, int line);

		/**
		 * Dumps the current state of the free list for debugging
		 */
		void Dump_Stats();

		/**
		 * Displays a visual representation of memory fragmentation
		 */
		void Dump_Fragmentation_Map();
#endif
		// Internal structure for block headers
		struct Block_Header
		{
			destan_u64 size;           // Size of this block in bytes (including header)
			bool is_free;              // Whether this block is free or allocated
			Block_Header* next;        // Pointer to the next block in memory order
			Block_Header* prev;        // Pointer to the previous block in memory order

			// For the free list
			Block_Header* next_free;   // Next free block in the free list
			Block_Header* prev_free;   // Previous free block in the free list

#ifdef DESTAN_DEBUG
			// Debug properties
			destan_u64 allocation_id;      // Unique ID for this allocation
			const destan_char* file;       // Source file where allocation occured
			destan_i32 line;               // Source line where allocation occured
			static constexpr destan_u32 GUARD_PATTERN = 0xFDFDFDFD;  // Guard pattern
			destan_u32 guard_value;        // Magic value to detect memory corruption
#endif
		};
	private:

		// Find a suitable block using the current allocation strategy
		Block_Header* Find_Suitable_Block(destan_u64 size, destan_u64 alignment);

		// Specific block finding strategies
		Block_Header* Find_First_Fit(destan_u64 size, destan_u64 alignment);
		Block_Header* Find_Best_Fit(destan_u64 size, destan_u64 alignment);
		Block_Header* Find_Next_Fit(destan_u64 size, destan_u64 alignment);

		// Split a block if there's enough space left after the allocation
		Block_Header* Split_Block(Block_Header* block, destan_u64 size);

		// Remove a block from the free list
		void Remove_From_Free_List(Block_Header* block);

		// Add a block to the free list
		void Add_To_Free_List(Block_Header* block);

		// Coalesce adjacent free blocks to reduce fragmentation
		Block_Header* Coalesce(Block_Header* block);

		// Validates that a pointer was allocated by this allocator
		bool Validate_Pointer(void* ptr) const;

		// Get the header from a user pointer
		Block_Header* Get_Block_Header(void* ptr) const;

		// Debug validation helpers
#ifdef DESTAN_DEBUG
		bool Validate_Block(Block_Header* block) const;
		bool Validate_Free_List() const;
#endif

		// Memory management
		void* m_memory_region = nullptr;         // Start of the memory region
		destan_u64 m_size = 0;                   // Total size of the memory region

		// Block management
		Block_Header* m_first_block = nullptr;   // First block in memory order
		Block_Header* m_free_list = nullptr;     // Head of the free list
		Block_Header* m_last_allocated = nullptr; // Last allocated block (for FIND_NEXT strategy)
		destan_u64 m_free_block_count = 0;      // Number of free blocks

		// Allocation strategy
		Allocation_Strategy m_strategy = Allocation_Strategy::FIND_BEST;

		// Minimum block size to avoid excessive fragmentation
		static constexpr destan_u64 MIN_BLOCK_SIZE =
			sizeof(Block_Header) + sizeof(void*) * 2; // Header + minimum usable size

		std::mutex m_mutex;

		// Fixed-size buffer for name
		static constexpr destan_u64 MAX_NAME_LENGTH = 64;
		destan_char m_name[MAX_NAME_LENGTH];

#ifdef DESTAN_DEBUG
		// Debug tracking for allocations
		static std::atomic<destan_u64> s_next_allocation_id;
		destan_u64 m_allocation_count = 0;       // Number of active allocations
#endif

		// Thread safety helpers
		void Lock()
		{
#ifdef DESTAN_THREAD_SAFE
			m_mutex.lock();
#endif
		}

		void Unlock()
		{
#ifdef DESTAN_THREAD_SAFE
			m_mutex.unlock();
#endif
		}
	};

	// Helper macros for free list allocator
#ifdef DESTAN_DEBUG
#define DESTAN_FREE_LIST_ALLOC(allocator, size, alignment) \
		(allocator)->Allocate_Debug((size), (alignment), __FILE__, __LINE__)
#else
#define DESTAN_FREE_LIST_ALLOC(allocator, size, alignment) \
		(allocator)->Allocate((size), (alignment))
#endif

#define DESTAN_FREE_LIST_CREATE(allocator, type, ...) \
	(allocator)->Create<type>(__VA_ARGS__)

#define DESTAN_FREE_LIST_DESTROY(allocator, ptr) \
	(allocator)->Destroy(ptr)
}