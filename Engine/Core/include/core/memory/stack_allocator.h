#pragma once
#include <core/memory/memory.h>
#include <core/defines.h>

namespace destan::core::memory
{
	/**
	 * Stack Allocator (LIFO Allocator)
	 * 
	 * A memory allocator that operates in a strictly last-in-first-out manner.
	 * Perfect for scenarios with nested operations or temporary allocations with
	 * predictable lifetimes, such as recursive algorithms or scope-based workspaces
	 * 
	 * Key characteristics:
	 * - Strict LIFO (Last-in-first-out) allocations/deallocation pattern
	 * - Fast rollback to previous markers
	 * - Near constant-time operations
	 * - Allows for scope-based memory management
	 * - No fragmentation within a single stack
	 */
	class Stack_Allocator
	{
	public:
		/**
	     * Marker represents a position in the stack
		 * Used for rolling back to previous positions
		 */
		using Marker = destan_u64;

		/**
		 * Creates a stack allocator with the specified size
		 *
		 * @param size_bytes Total size of the memory stack in bytes
		 * @param name Optional name for debugging purposes
		 */
		Stack_Allocator(destan_u64 size_bytes, const char* name = "Stack");

		/**
		 * Destructor - releases the entire memory stack
		 */
		~Stack_Allocator();

		// Disable copying to prevent double-freeing
		Stack_Allocator(const Stack_Allocator&) = delete;
		Stack_Allocator& operator=(const Stack_Allocator&) = delete;

		// Move operations
		Stack_Allocator(Stack_Allocator&& other) noexcept;
		Stack_Allocator& operator=(Stack_Allocator&& other) noexcept;

		/**
		 * Allocates memory from the stack
		 *
		 * @param size Size of the allocation in bytes
		 * @param alignment Memory alignment (defaults to DEFAULT_ALIGNMENT)
		 * @return Pointer to allocated memory, or nullptr if out of memory
		 */
		void* Allocate(destan_u64 size, destan_u64 alignment = DEFAULT_ALIGNMENT);


		/**
		 * Allocates and constructs an object on the stack
		 *
		 * @tparam T Type of object to construct
		 * @tparam Args Constructor argument types
		 * @param args Arguments to forward to the constructor
		 * @return Pointer to the constructed object, or nullptr if out of memory
		 */
		template<typename T, typename... Args>
		T* Create(Args&&... args) {
			void* memory = Allocate(sizeof(T), alignof(T));
			if (!memory)
			{
				return nullptr;
			}
			return new(memory) T(std::forward<Args>(args)...);
		}


		/**
		 * Allocates an array of objects on the stack
		 *
		 * @tparam T Type of objects in the array
		 * @param count Number of objects
		 * @return Pointer to the first object, or nullptr if out of memory
		 */
		template<typename T>
		T* Create_Array(destan_u64 count) {
			// Calculate total size with alignment considerations
			destan_u64 total_size = sizeof(T) * count;
			void* memory = Allocate(total_size, alignof(T));
			if (!memory)
			{
				return nullptr;
			}

			// Construct each object
			T* array = static_cast<T*>(memory);
			for (destan_u64 i = 0; i < count; ++i)
			{
				new(&array[i]) T();
			}

			return array;
		}

		/**
		 * Gets a marker for the current position in the stack
		 *
		 * @return A marker representing the current stack position
		 */
		Marker Get_Marker() const;

		/**
		 * Frees all allocations made after the specified marker
		 *
		 * @param marker A previous marker to roll back to
		 * @param destruct_objects If true, calls destructors on all freed objects (debug only)
		 */
		void Free_To_Marker(Marker marker, destan_bool destruct_objects = false);

		/**
		 * Explicitly frees the most recent allocation
		 * This enforces LIFO ordering - you can only free the most recently allocated memory
		 *
		 * @return True if successful, false if the stack is empty
		 */
		bool Free_Latest();

		/**
		 * Resets the stack to its initial empty state
		 * This frees all allocations at once
		 *
		 * @param destruct_objects If true, calls destructors on all allocated objects (debug only)
		 */
		void Reset(destan_bool destruct_objects = false);


		/**
		 * Returns the total size of the stack in bytes
		 */
		destan_u64 Get_Size() const { return m_size; }


		/**
		 * Returns the current used size in bytes
		 */
		destan_u64 Get_Used_Size() const { return m_current_pos - m_start_pos; }

		/**
		 * Returns the remaining free size in bytes
		 */
		destan_u64 Get_Free_Size() const { return m_size - Get_Used_Size(); }

		/**
		 * Returns the memory utilization percentage (0-100)
		 */
		destan_f32 Get_Utilization() const
		{
			return (static_cast<destan_f32>(Get_Used_Size()) / static_cast<destan_f32>(m_size)) * 100.0f;
		}

		/**
		 * Returns the name of this allocator
		 */
		const destan_char* Get_Name() const { return m_name; }

#ifdef DESTAN_DEBUG
		/**
		 * Debug version of Allocate that tracks the allocation
		 */
		void* Allocate_Debug(destan_u64 size, destan_u64 alignment, const destan_char* file, int line);

		/**
		 * Dumps the current state of the stack for debugging
		 */
		void Dump_Stats();
#endif

	private:
		// Helper methods to enforce alignment rules
		destan_u64 Align_Address(destan_u64 address, destan_u64 alignment) const;

		// Memory block management
		void* m_memory_block = nullptr;  // Start of the memory block
		destan_u64 m_start_pos = 0;     // Start position (as an offset)
		destan_u64 m_current_pos = 0;   // Current allocation position (as an offset)
		destan_u64 m_end_pos = 0;       // End of the memory block (as an offset)
		destan_u64 m_size = 0;          // Total size in bytes

		// Fixed-size buffer for name
		static constexpr destan_u64 MAX_NAME_LENGTH = 64;
		destan_char m_name[MAX_NAME_LENGTH];

#ifdef DESTAN_DEBUG
		// Debug tracking for allocations
		struct Allocation_Info {
			destan_u64 pos;         // Position in the stack
			destan_u64 size;        // Size of allocation
			destan_u64 alignment;   // Alignment of allocation
			const destan_char* file;       // Source file
			int line;               // Source line
		};

		// Track allocations in debug mode
		static constexpr destan_u64 MAX_DEBUG_ALLOCATIONS = 1024;
		destan_u64 m_debug_allocation_count = 0;
		Allocation_Info* m_debug_allocations = nullptr;

		// For thread safety
		std::mutex m_mutex;

		// Locks for thread safety in debug mode
		void Lock();
		void Unlock();
#endif
	};

	// Helper macros for stack allocator
#ifdef DESTAN_DEBUG
#define DESTAN_STACK_ALLOC(stack, size, alignment) \
        (stack)->Allocate_Debug((size), (alignment), __FILE__, __LINE__)
#else
#define DESTAN_STACK_ALLOC(stack, size, alignment) \
        (stack)->Allocate((size), (alignment))
#endif

#define DESTAN_STACK_CREATE(stack, type, ...) \
    (stack)->Create<type>(__VA_ARGS__)

#define DESTAN_STACK_CREATE_ARRAY(stack, type, count) \
    (stack)->Create_Array<type>(count)

// Helper class for automatic scope-based memory management
	class Stack_Scope {
	public:
		Stack_Scope(Stack_Allocator& stack)
			: m_stack(stack), m_marker(stack.Get_Marker()) {
		}

		~Stack_Scope() {
			m_stack.Free_To_Marker(m_marker);
		}

	private:
		Stack_Allocator& m_stack;
		Stack_Allocator::Marker m_marker;
	};

// Convenience macro for creating stack scopes
#define DESTAN_STACK_SCOPE(stack) \
    destan::core::memory::Stack_Scope DESTAN_CONCAT(stack_scope_, __LINE__)(stack)

// Helper for the above macro
#define DESTAN_CONCAT(a, b) DESTAN_CONCAT_IMPL(a, b)
#define DESTAN_CONCAT_IMPL(a, b) a##b
} //namespace::destan::core::memory