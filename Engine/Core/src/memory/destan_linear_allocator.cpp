#include <core/destan_pch.h>
#include <core/memory/destan_linear_allocator.h>
#include <core/memory/destan_memory_manager.h>

namespace destan::core
{
	Linear_Allocator::Linear_Allocator(u64 size) : m_size(size), m_offset(0)
	{
		m_memory = Memory_Manager::Allocate(size, "Linear Allocator");
		if (!m_memory)
		{
			DESTAN_LOG_ERROR("[MEM-Linear_Allocator] Failed to allocate memory for linear allocator!");
		}
	}

	Linear_Allocator::~Linear_Allocator()
	{
		Memory_Manager::Deallocate(m_memory);
	}

	void* Linear_Allocator::Allocate(u64 size)
	{
		if (m_offset + size > m_size)
		{
			DESTAN_LOG_ERROR("[MEM-Linear_Allocator] Out of Memory!");
			return nullptr;
		}

		void* ptr = static_cast<char*>(m_memory) + m_offset;
		m_offset += size;
		return ptr;
	}

	void Linear_Allocator::Reset()
	{
		// Reset the offset to the beginning of the memory
		// but do not free the memory
		m_offset = 0;
	}
}