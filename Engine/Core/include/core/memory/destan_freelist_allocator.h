#pragma once
#include <core/defines.h>
#include <atomic>

namespace destan::core
{
	struct Free_Block
	{
		u64 size;
		std::atomic<Free_Block*> next;
	};

	class Freelist_Allocator
	{
	public:
		Freelist_Allocator(u64 size);
		~Freelist_Allocator();

		void* Allocate(u64 size, u64 alignment);
		void Free(void* ptr);
		void Reset();
		void Coalesce();

	private:
		void* m_memory;
		std::atomic<Free_Block*> m_free_list;
	};
}