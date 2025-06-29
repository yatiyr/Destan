#pragma once
#include <core/memory/memory.h>


namespace ds::core::memory
{
	/**
	 * Arena Allocator (Linear Allocator)
	 * 
	 * This allocator reserves a contiguous block of memory and allocates 
	 * linearly with minimal overhead. Perfect for temporary allocations
	 * that have the same lifetime, such as per-frame operations
	 * 
	 * Key characteristics:
	 * - Very fast allocations (nearly O(1))
	 * - No support for individual deallocations
	 * - Bulk reset operation
	 * - Low fragmentation
	 * - Configurable alignment	
	 */
	class Arena_Allocator
	{
	public:
		/**
		 * Create anarena allocator with the specified size
		 * 
		 * @param size_bytes Total size of the memory arena in bytes
		 * @param name Optional name for debugging purposes
		 */
		Arena_Allocator(ds_u64 size_bytes, const char* name = "Arena");

		/**
		 * Destructor releases the entire memory block
		 */
		~Arena_Allocator();

		// We disable copying to prevent double-free issues
		Arena_Allocator(const Arena_Allocator&) = delete;
		Arena_Allocator& operator=(const Arena_Allocator&) = delete;

		// Move operations	
		Arena_Allocator(Arena_Allocator&& other) noexcept;
		Arena_Allocator& operator=(Arena_Allocator&& other) noexcept;

		/**
		 * Allocates memory from the arena
		 * 
		 * @param size Size of the allocation in bytes
		 * @param alignment Memory alignment (defaults to DEFAULT_ALIGNMENT)
		 * @return Pointer to the allocated memory or nullptr if allocation failed
		 */
		void* Allocate(ds_u64 size, ds_u64 alignment = DEFAULT_ALIGNMENT);

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
		 * Allocates an array of objects of type T
		 * @tparam T Type of objects in the array
		 * @param count Number of objects to allocate
		 * @return Pointer to the first object in the array or nullptr if allocation failed
		 */
		template<typename T>
		T* Create_Array(ds_u64 count)
		{
			// Calculate total size with alignment considerations
			ds_u64 total_size = sizeof(T) * count;
			void* memory = Allocate(total_size, alignof(T));
			if (!memory)
			{
				return nullptr;
			}

			// Construct each object in the array
			T* array = static_cast<T*>(memory);
			for (ds_u64 i = 0; i < count; i++)
			{
				new(&array[i]) T();
			}

			return array;
		}

		/**
		 * Deallocate does nothing in the arena allocator
		 * Memory is only reclaimed when the entire arena is reset
		 * 
		 * @param ptr Pointer to previously allocated memory (ignored)
		 */
		void Deallocate(void* ptr)
		{
			// Do nothing
		}

		/**
		 * Resets the arena allocator to its initial empty state
		 * This frees all allocations at once
		 *
		 */
		void Reset();

		/**
		 * Returns the total size of the arena in bytes
		 */
		ds_u64 Get_Size() const
		{ 
			return m_size;
		}

		/**
		 * Returns the current used size in bytes
		 */
		ds_u64 Get_Used_Size() const
		{
			return static_cast<ds_u64>(m_current_pos - m_start_pos);
		}

		/**
		 * Returns the remaining free size in bytes
		 */
		ds_u64 Get_Free_Size() const
		{
			return m_size - Get_Used_Size();
		}

		/**
		 * Returns the allocation count
		 */
		ds_u64 Get_Allocation_Count() const
		{
			return m_allocation_count;
		}

		/**
		 * Returns the memory utilization percentage
		 */
		ds_f32 Get_Utilization() const
		{
			return static_cast<ds_f32>(Get_Used_Size()) / static_cast<ds_f32>(m_size) * 100.0f;
		}

		/**
		 * Returns the name of this allocator
		 */
		const ds_char* Get_Name() const
		{
			return m_name;
		}

#ifdef DS_DEBUG
		/**
		 * Debug version of Allocate that tracks the allocation
		 */
		void* Allocate_Debug(ds_u64 size, ds_u64 alignment, const char* file, int line);

		/**
		 * Dumps the current state of the arena for debugging
		 */
		void Dump_Stats() const;
#endif

	private:
		// Memory block management
		void* m_memory_block = nullptr;        // Start of the memory block
		ds_char* m_start_pos = nullptr;    // Start position for allocations
		ds_char* m_current_pos = nullptr;  // Current allocation position
		ds_char* m_end_pos = nullptr;      // End of the memory block

		// Arena properties
		ds_u64 m_size = 0;                 // Total size of the memory block in bytes
		ds_u64 m_allocation_count = 0;     // Number of allocations

		// Fixed-size buffer for name
		static constexpr ds_u64 MAX_NAME_LENGTH = 64;
		char m_name[MAX_NAME_LENGTH];

#ifdef DS_DEBUG
		// Debug tracking for allocations
		struct Allocation_Info
		{
			void* ptr;               // Pointer to the allocated memory
			ds_u64 size;		 // Size of the allocation
			const ds_char* file; // Source file where allocation occured
			ds_i32 line;         // Source line where allocation occured
		};

		// Track allocations in debug mode
		static constexpr ds_u64 MAX_DEBUG_ALLOCATIONS = 1024;
		Allocation_Info m_debug_allocations[MAX_DEBUG_ALLOCATIONS];
		ds_u64 m_debug_allocation_count = 0;
#endif
	};

// Helper macros for arena allocator
#ifdef DS_DEBUG

	#define DS_ARENA_ALLOC(arena, size, alignment) \
        (arena)->Allocate_Debug((size), (alignment), __FILE__, __LINE__)
	#else
	#define DS_ARENA_ALLOC(arena, size, alignment) \
        (arena)->Allocate((size), (alignment))
#endif

#define DS_ARENA_CREATE(arena, type, ...) \
    (arena)->Create<type>(__VA_ARGS__)

#define DS_ARENA_CREATE_ARRAY(arena, type, count) \
    (arena)->Create_Array<type>(count)
}