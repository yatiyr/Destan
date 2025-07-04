#pragma once
#include <core/defines.h>
#include <atomic>


namespace ds::core::memory
{
	// Constants for memory alignment
	constexpr ds_u64 DEFAULT_ALIGNMENT = 16;    // Default alignment for memory allocations
	constexpr ds_u64 CACHE_LINE_SIZE   = 64;    // Common cache line size on modern CPUs
	constexpr ds_u64 SIMD_ALIGNMENT    = 32;    // For AVX/SIMD operations

	// Memory system initialization states
	enum class Memory_Init_State
	{
		Uninitialized,
		Initializing,
		Initialized,
		Shutting_Down
	};

	/*
	 * Core memory system class for providing low-level memory operations	
	 */
	class Memory
	{
	public:

		// Initialize memory system
		static void Initialize();

		// Shutdown memory system
		static void Shutdown();

		// Check if memory system is initialized
		static bool Is_Initialized();

		// Basic allocation functions
		static void* Malloc(ds_u64 size, ds_u64 alignment = DEFAULT_ALIGNMENT, bool thread_local_allocation = false);
		static void Free(void* ptr);

		// Advanced memory operations
		static void* Realloc(void* ptr, ds_u64 new_size, ds_u64 alignment = DEFAULT_ALIGNMENT);
		static void* Memmove(void* dest, const void* src, ds_u64 size);
		static void* Memcpy(void* dest, const void* src, ds_u64 size);
		static void* Memset(void* dest, ds_i32 value, ds_u64 size);

		// Memory comparison and information
		static int Memcmp(const void* ptr1, const void* ptr2, ds_u64 size);
		static ds_u64 Get_Allocation_Size(void* ptr);

		// Alignment utilities
		static ds_u64 Align_Size(ds_u64 size, ds_u64 alignment)
		{
			return (size + alignment - 1) & ~(alignment - 1);
		}

		static void* Align_Address(void* address, ds_u64 alignment)
		{
			return reinterpret_cast<void*>
			(
				(reinterpret_cast<ds_uiptr>(address) + alignment - 1) & ~(alignment - 1)
			);
		}

		// Thread-local allocator access
		static void* Thread_Local_Malloc(ds_u64 size, ds_u64 alignment = DEFAULT_ALIGNMENT);
		static bool Thread_Local_Free(void* ptr);

		// Tracking statistics (lock-free)
		static ds_u64 Get_Total_Allocated();
		static ds_u64 Get_Total_Freed();
		static ds_u64 Get_Allocation_Count();
		static ds_u64 Get_Current_Used_Memory();

		// Debug helpers
		static void Dump_Memory_Stats();
		static void Check_Memory_Leaks();

#ifdef DS_DEBUG
		// Debug-specific allocation functions
		static void* Malloc_Debug(ds_u64 size, ds_u64 alignment, const char* file, int line);
		static void Free_Debug(void* ptr);
#endif

	private:
		// Private methods for internal use
		static void Initialize_Tracking();
		static void Shutdown_Tracking();
		static void Initialize_Thread_Local_Storage();
		static void Shutdown_Thread_Local_Storage();

		// Allocation header structure (for tracking in debug mode)
#if DS_DEBUG
		struct Allocation_Header
		{
			ds_u64 size;               // Size of the allocation (excluding header)
			ds_u32 alignment;          // Alignment of the allocation
			ds_const_char* file;       // Source file where allocation occured
			ds_u32 line;               // Source line where allocation occured
			ds_u64 allocation_id;      // Unique ID for this allocation
			ds_u32 guard_value;        // Magic value to detect underflows

			// Guard pattern for memory corruption detection
			static constexpr ds_u32 GUARD_PATTERN = 0xFDFDFDFD;
		};

		// Get pointer to user memory from header
		static void* Get_User_Pointer(Allocation_Header* header);

		// Get header from user pointer
		static Allocation_Header* Get_Header(void* user_ptr);

		// Validate a pointer's header
		static bool Validate_Header(Allocation_Header* header);
#endif

		// Static state - using atomics for lock-free statistics tracking
		static std::atomic<Memory_Init_State> s_init_state;
		static std::atomic<ds_u64> s_total_allocated;
		static std::atomic<ds_u64> s_total_freed;
		static std::atomic<ds_u64> s_allocation_count;
	};

	// Helper macros for debug allocations
#ifdef DS_DEBUG
	// These wrap the memory functions to include source info
	#define DS_MALLOC(size, alignment) \
        ds::core::memory::Memory::Malloc_Debug((size), (alignment), __FILE__, __LINE__)

    #define DS_FREE(ptr) \
        ds::core::memory::Memory::Free_Debug((ptr))
#else
	// Release mode - direct calls with no overhead
    #define DS_MALLOC(size, alignment) \
        ds::core::memory::Memory::Malloc((size), (alignment))

    #define DS_FREE(ptr) \
        ds::core::memory::Memory::Free((ptr))
#endif

    // Thread-local allocation macros for highest performance
    #define DS_THREAD_LOCAL_MALLOC(size, alignment) \
        ds::memory::Memory::Thread_Local_Malloc((size), (alignment))

    #define DS_THREAD_FREE(ptr) \
        ds::memory::Memory::Thread_Local_Free((ptr))
} // namespace ds::core::memory