#include <core/destan_pch.h>
#include <core/memory/destan_stack_allocator.h>

namespace destan::core
{
	Stack_Allocator::Stack_Allocator(u64 size) : m_size(size), m_offset(0)
	{
		m_memory = Memory_Manager::Allocate(size, "Stack Allocator");
		if (!m_memory)
		{
			DESTAN_LOG_ERROR("[MEM-Stack_Allocator] Failed to allocate memory for stack allocator!");
		}
	}

	Stack_Allocator::~Stack_Allocator()
	{
		Memory_Manager::Deallocate(m_memory);
	}

	void* Stack_Allocator::Allocate(u64 size)
	{
		if (m_offset + size > m_size)
		{
			DESTAN_LOG_ERROR("[MEM-Stack_Allocator] Out of Memory!");
			return nullptr;
		}

		void* ptr = static_cast<char*>(m_memory) + m_offset;
		m_offset += size;
		return ptr;
	}

	void Stack_Allocator::Free(void* ptr)
	{
		// Free only works for last allocated memory
		if (ptr == static_cast<char*>(m_memory) + (m_offset - sizeof(ptr)))
		{
			m_offset -= sizeof(ptr);
		}
		else
		{
			DESTAN_LOG_ERROR("[MEM-Stack_Allocator] Cannot free non-LIFO allocated memory!");
		}
	}

	void Stack_Allocator::Reset()
	{
		m_offset = 0;
	}

	void Stack_Allocator::Free_To_Marker(Marker marker)
	{
		if (marker <= m_offset)
		{
			m_offset = marker;
		}
		else
		{
			DESTAN_LOG_ERROR("[MEM-Stack_Allocator] Invalid marker value!");
		}
	}
}