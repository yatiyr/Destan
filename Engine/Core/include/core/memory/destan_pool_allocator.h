#pragma once
#include <core/defines.h>
#include <core/memory/destan_memory_manager.h>
#include <atomic>

namespace destan::core
{
	class Pool_Allocator
	{
	public:
		Pool_Allocator(u64 size, u64 object_count);
		~Pool_Allocator();

		void* Allocate();
		void Free(void* ptr);
		void Reset();

	private:
		void* m_memory;
		u64 m_object_size;
		u64 m_object_count;
		// This is a linked-list of free objects
		std::atomic<void*> m_free_list;
	};
}
