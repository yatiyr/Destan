#pragma once
#include <core/defines.h>

namespace destan::core
{
	class Memory_Manager
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void* Allocate(u64 size, const char* tag = "Default");
		static void Deallocate(void* ptr);

		static u64 Get_Allocated_Size(void* ptr);

		static void Report_Memory_Usage();
	};
} // namespace destan::core