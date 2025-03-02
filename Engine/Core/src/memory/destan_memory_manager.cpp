#include <core/destan_pch.h>
#include <core/memory/destan_memory_manager.h>

namespace destan::core
{
	static u64 s_total_allocated = 0;
	static std::unordered_map<void*, u64> s_allocations;

	void Memory_Manager::Initialize()
	{
		DESTAN_LOG_INFO("[MEM] Memory Manager Initialized");
	}

	void Memory_Manager::Shutdown()
	{
		s_allocations.clear();
		Report_Memory_Usage();
		DESTAN_LOG_INFO("[MEM] Memory Manager Shutdown");
	}

	void* Memory_Manager::Allocate(u64 size, const char* tag)
	{
		void* ptr = malloc(size);
		if (!ptr)
		{
			DESTAN_LOG_ERROR("[MEM] Failed to allocate {0} bytes", size);
			return nullptr;
		}

		s_total_allocated += size;
		s_allocations[ptr] = size;
		DESTAN_LOG_INFO("[MEM] Allocated {0} bytes at {1} (Total: {2}) ({3})", size, ptr, s_total_allocated, tag); 

		return ptr;
	}

	void Memory_Manager::Deallocate(void* ptr)
	{
		if (!ptr) return;

		auto it = s_allocations.find(ptr);
		if (it != s_allocations.end())
		{
			s_total_allocated -= it->second;
			s_allocations.erase(it);
			std::free(ptr);
			DESTAN_LOG_INFO("[MEM] Freed {0} bytes", it->second);
		}
		else
		{
			DESTAN_LOG_ERROR("[MEM] Attempted to free invalid pointer {0}", ptr);
		}
	}

	u64 Memory_Manager::Get_Allocated_Size(void* ptr)
	{
		auto it = s_allocations.find(ptr);
		return (it != s_allocations.end()) ? it->second : 0;
	}

	void Memory_Manager::Report_Memory_Usage()
	{
		DESTAN_LOG_INFO("[MEM] Total allocated: {0}", s_total_allocated);
		if (!s_allocations.empty())
		{
			DESTAN_LOG_ERROR("[MEM] Warning! There are memory leaks!");
		}
	}
}