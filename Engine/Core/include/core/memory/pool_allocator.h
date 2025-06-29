#include <core/ds.h>
#include <core/memory/memory.h>

namespace ds::core::memory
{

	/**
	 * Pool Allocator
	 * 
	 * A memory allocator optimized for efficient allocation and deallocation
	 * of fixed-size blocks. Perfect for game objects like entities, particles,
	 * or components that have uniform sizes and are frequently created/destroyed
	 * 
	 * Key characteristics:
	 * - Near constant-time allocation and deallocation (O(1))
	 * - Zero fragmentation for fixed-size allocations
	 * - Memory reuse without additional allocations
	 * - Thread-safe options available
	 */
	class Pool_Allocator
	{
	public:
		/**
		 * Creates a pool allocator with fixed-sized blocks
		 * 
		 * @param block_size Size of each block in the pool
		 * @param block_count Number of blocks to pre-allocate
		 * @param name Optional name for debugging purposes
		 * @param alignment
		 */
		Pool_Allocator(ds_u64 block_size, ds_u64 block_count, const ds_char* name = "Pool", ds_u64 alignment = 16);

		/**
		 * Destructor copying to prevent double-freeing
		 */
		~Pool_Allocator();

		// Disable copying to prevent double-freeing
		Pool_Allocator(const Pool_Allocator&) = delete;
		Pool_Allocator& operator=(const Pool_Allocator&) = delete;

		// Move operators
		Pool_Allocator(Pool_Allocator&& other) noexcept;
		Pool_Allocator& operator=(Pool_Allocator&& other) noexcept;

		/**
		 * Allocates a block from the pool
		 * 
		 * @return Pointer to allocated block, or nullptr if pool is full
         */
		void* Allocate();

		/**
		 * Allocates and constructs an object in the pool
		 * 
		 * @tparam T Type of object to construct
		 * @tparam Args Constructor argument types
		 * @param args Arguments to forward to the constructor
		 * @return Pointer to the constructed object, or nullptr if pool is full
		 */
		template<typename T, typename... Args>
		T* Create(Args&&... args)
		{
			DS_ASSERT(sizeof(T) <= m_block_size, "Object size exceeds block size");
			DS_ASSERT(alignof(T) <= m_block_alignment, "Object alignment exceeds block alignment");

			void* memory = Allocate();
			if (!memory)
			{
				return nullptr;
			}
			return new(memory) T(std::forward<Args>(args)...);
		}

		/**
		 * Deallocates a block, returning it to the pool
		 *
		 * @param ptr Pointer to the block to deallocate
		 * @return True if deallocation was successful, false if ptr was invalid
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
		 * Resets the pool, making all blocks available again
		 * Note: This does not call destructors!
		 */
		void Reset();

		/**
		 * Returns the block size in bytes
		 */
		ds_u64 Get_Block_Size() const
		{
			return m_block_size;
		}

		/**
		 * Returns the block alignment in bytes
		 */
		ds_u64 Get_Block_Alignment() const
		{
			return m_block_alignment;
		}

		/**
		 * Returns the number of blocks in the pool
		 */
		ds_u64 Get_Block_Count() const
		{
			return m_block_count;
		}

		/**
		 * Returns the number of free blocks
		 */
		ds_u64 Get_Free_Block_Count() const
		{
			return m_free_blocks;
		}

		/**
		 * Returns the number of allocated blocks
		 */
		ds_u64 Get_Allocated_Block_Count() const
		{
			return m_block_count - m_free_blocks;
		}

		/**
		 * Returns the pool utilization percentage (0-100)  
		 */
		float Get_Utilization() const
		{
			return (static_cast<ds_f32>(Get_Allocated_Block_Count()) / static_cast<ds_f32>(m_block_count)) * 100.0f;
		}

		/**
		 *  Returns the name of this allocator
		 */
		const ds_char* Get_Name() const
		{
			return m_name;
		}

#ifdef DS_DEBUG
		/**
		 * Debug version of Allocate that tracks the allocation
		 */
		void* Allocate_Debug(const char* file, int line);

		/**
		 * Dumps the current state of the pool for debugging
		 */
		void Dump_Stats();
#endif

	private:
		// Internal structure representing a free block in the pool
		struct Free_Block {
			Free_Block* next; // Pointer to the next free block
		};

		// Validates a pointer belongs to this pool
		bool Is_Address_In_Pool(void* ptr) const;

		// Gets the index of a block from its address
		ds_u64 Get_Block_Index(void* ptr) const;

		// Locks the pool for thread-safe operations (if enabled)
		void Lock();

		// Unlocks the pool after thread-safe operations
		void Unlock();

		// Memory block management
		void* m_memory_pool = nullptr;     // Start of the memory pool
		ds_u64 m_block_size = 0;       // Size of each block in bytes
		ds_u64 m_padded_block_size = 0; // Block size with padding for alignment
		ds_u64 m_block_alignment = 0;   // Alignment of blocks
		ds_u64 m_block_count = 0;      // Total number of blocks
		std::atomic<ds_u64> m_free_blocks{ 0 }; // Number of free blocks

		// Free list management
		Free_Block* m_free_list = nullptr; // Head of the free list

		// Fixed-size buffer for name
		static constexpr ds_u64 MAX_NAME_LENGTH = 64;
		char m_name[MAX_NAME_LENGTH];

#ifdef DS_DEBUG
		// Debug tracking for allocations
		struct Allocation_Info 
		{
			void* ptr;           // Pointer to allocated block
			const char* file;    // Source file
			int line;            // Source line
			bool allocated;      // Whether this block is currently allocated
		};

		// Track allocations in debug mode
		Allocation_Info* m_debug_blocks = nullptr;

		// For thread safety
		std::mutex m_mutex;
#endif
	};

	// Helper macros for pool allocator
#ifdef DS_DEBUG
#define DS_POOL_ALLOC(pool) \
        (pool)->Allocate_Debug(__FILE__, __LINE__)
#else
#define DS_POOL_ALLOC(pool) \
        (pool)->Allocate()
#endif

#define DS_POOL_CREATE(pool, type, ...) \
    (pool)->Create<type>(__VA_ARGS__)

#define DS_POOL_DESTROY(pool, ptr) \
    (pool)->Destroy(ptr)

} // namespace ds::core::memory