#include <core/destan_pch.h>
#include <core/memory/destan_pool_allocator.h>

namespace destan::core
{
	Pool_Allocator::Pool_Allocator(u64 object_size, u64 object_count)
		: m_object_count(object_count)
	{
		// Every object is alligned properly
		m_object_size = ALIGN_UP(object_size, alignof(std::max_align_t));

		m_memory = Memory_Manager::Allocate(m_object_size * m_object_count, "Pool Allocator");
		if (!m_memory)
		{
			DESTAN_LOG_ERROR("[MEM-Pool_Allocator] Failed to allocate memory for pool allocator!");
			throw std::bad_alloc();
		}

		// Initialize free list with reset
		Reset();
	}

	Pool_Allocator::~Pool_Allocator()
	{
		Memory_Manager::Deallocate(m_memory);
	}

	void* Pool_Allocator::Allocate()
	{
		void* old_head;
		void* new_head;

		do
		{
			old_head = m_free_list.load(std::memory_order_acquire);
			if (!old_head)
			{
				return nullptr;
			}

			// new head is the old head
			new_head = *reinterpret_cast<void**>(old_head);
		} while (!m_free_list.compare_exchange_weak(old_head, new_head, std::memory_order_release));

		if (!m_free_list)
		{
			return nullptr;
		}

		return old_head;
	}

	void Pool_Allocator::Free(void* ptr)
	{
		if (!ptr)
		{
			return;
		}

		void* old_head;
		do
		{
			old_head = m_free_list.load(std::memory_order_acquire);
			*reinterpret_cast<void**>(ptr) = old_head;
		} while (!m_free_list.compare_exchange_weak(old_head, ptr, std::memory_order_release));
	}

	void Pool_Allocator::Reset()
	{
		memset(m_memory, 0, m_object_size * m_object_count);
		m_free_list.store(m_memory, std::memory_order_relaxed);

		char* current = static_cast<char*>(m_memory);

		for (u64 i = 0; i < m_object_count - 1; i++)
		{
			*reinterpret_cast<void**>(current) = current + m_object_size;
			current += m_object_size;
		}
		*reinterpret_cast<void**>(current) = nullptr;
	}
}
