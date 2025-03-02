#include <core/destan_pch.h>
#include <core/memory/destan_freelist_allocator.h>
#include <core/memory/destan_memory_manager.h>

namespace destan::core
{
	// Merge after 10 free blocks
	constexpr u64 COALESCING_THRESHOLD = 10;
	std::atomic<u64> free_count = 0;

	Freelist_Allocator::Freelist_Allocator(u64 size)
	{
		// Allocate a large chunk of memory
		m_memory = Memory_Manager::Allocate(size, "Freelist Allocator");
		if (!m_memory)
		{
			DESTAN_LOG_ERROR("[MEM-Freelist_Allocator] Failed to allocate memory!");
			throw std::bad_alloc();
		}

		// first free block is the entire memory
		m_free_list.store(static_cast<Free_Block*>(m_memory), std::memory_order_relaxed);
		m_free_list.load(std::memory_order_relaxed)->size = size;
		m_free_list.load(std::memory_order_relaxed)->next = nullptr;
	}

	Freelist_Allocator::~Freelist_Allocator()
	{
		Memory_Manager::Deallocate(m_memory);
	}

	void* Freelist_Allocator::Allocate(u64 size, u64 alignment)
	{
		size = ALIGN_UP(size, alignment);

		Free_Block* current = m_free_list.load(std::memory_order_acquire);

		while (current)
		{
			void* aligned_address = std::align(alignment, size, reinterpret_cast<void*&>(current), current->size);
			if (aligned_address)
			{
				Free_Block* next = current->next.load(std::memory_order_acquire);

				if (current->size == size)
				{
					// If the block is an exact match, remove it from the freelist
					if (m_free_list.compare_exchange_weak(current, next, std::memory_order_release))
					{
						return aligned_address;
					}
					continue; // If CAS fails, retry
				}

				// If the block is larger than required, split it
				void* allocated = aligned_address;
				Free_Block* new_block = reinterpret_cast<Free_Block*>(reinterpret_cast<char*>(aligned_address) + size);
				new_block->size = current->size - size;

				// Load the next pointer safely
				Free_Block* expected_next = next;
				new_block->next.store(expected_next, std::memory_order_release);

				// Update current->next atomically
				if (!current->next.compare_exchange_weak(expected_next, new_block, std::memory_order_release))
				{
					continue; // If CAS fails, retry
				}

				return allocated;
			}

			current = current->next.load(std::memory_order_acquire);
		}

		DESTAN_LOG_ERROR("[MEM-Freelist_Allocator] Out of Memory!");
		return nullptr;
	}

	void Freelist_Allocator::Free(void* ptr)
	{
		if (!ptr)
		{
			return;
		}

		Free_Block* block = static_cast<Free_Block*>(ptr);
		Free_Block* current = m_free_list.load(std::memory_order_acquire);

		do
		{
			block->next = current;
		} while (!m_free_list.compare_exchange_weak(current, block, std::memory_order_release));

		if (++free_count >= COALESCING_THRESHOLD)
		{
			Coalesce();
			free_count = 0;
		}
	}

	void Freelist_Allocator::Coalesce()
	{
		Free_Block* current = m_free_list.load(std::memory_order_acquire);
		Free_Block* prev = nullptr;

		while (current)
		{
			Free_Block* next = current->next;

			if (next && reinterpret_cast<char*>(current) + current->size == reinterpret_cast<char*>(next))
			{
				Free_Block* next_next = next->next;
				current->size += next->size;

				// we need to use cas to update the next pointer
				while (!current->next.compare_exchange_weak(next, next_next, std::memory_order_release))
				{
					// Load again, another thread might have updated the next pointer
					next = current->next;
				}
				continue;
			}

			prev = current;
			current = next;
		}
	}

	void Freelist_Allocator::Reset()
	{
		m_free_list.store(static_cast<Free_Block*>(m_memory), std::memory_order_relaxed);
		m_free_list.load(std::memory_order_relaxed)->size = Memory_Manager::Get_Allocated_Size(m_memory);
		m_free_list.load(std::memory_order_relaxed)->next = nullptr;
		free_count = 0;
	}
}