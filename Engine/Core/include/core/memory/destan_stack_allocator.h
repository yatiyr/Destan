#pragma once
#include <core/defines.h>
#include <core/memory/destan_memory_manager.h>

namespace destan::core
{
	class Stack_Allocator
	{
	public:
		using Marker = u64;

		Stack_Allocator(u64 size);
		~Stack_Allocator();

		void* Allocate(u64 size);
		void Free(void* ptr);
		void Reset();

		Marker Get_Marker() const { return m_offset; }
		void Free_To_Marker(Marker marker);

		u64 Get_Used_Memory() const { return m_offset; }
		u64 Get_Total_Size() const { return m_size; }

	private:
		void* m_memory;
		u64 m_size;
		u64 m_offset;
	};
}