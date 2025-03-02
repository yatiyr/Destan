#pragma once
#include <core/defines.h>

namespace destan::core
{
	class Linear_Allocator
	{
	public:
		Linear_Allocator(u64 size);
		~Linear_Allocator();

		void* Allocate(u64 size);
		void Reset();

		u64 Get_Used_Memory() const { return m_offset; }
		u64 Get_Total_Size() const { return m_size; }

	private:
		void* m_memory;
		u64 m_size;
		u64 m_offset;
	};
}