#include <core/destan_pch.h>
#include <core/defines.h>
#include <core/memory/memory.h>
#include <test_framework.h>

#include <arena_allocator_tests.h>

using namespace destan::core::memory;
using namespace destan::test;
using namespace destan;

// Test the basic memory functions
bool Test_Memory_Basic()
{
	// Initialize the memory system
	Memory::Initialize();

	// Allocate memory
	const destan_u64 size = 1024;
	void* memory = Memory::Malloc(size);

	// Verify allocation succeeded
	DESTAN_EXPECT(memory != nullptr);

	// Write to memory
	Memory::Memset(memory, 0xAB, size);

	// Verify memory was written correctly
	destan_u8* bytes = static_cast<destan_u8*>(memory);
	for (destan_u64 i = 0; i < size; i++)
	{
		DESTAN_EXPECT(bytes[i] == 0xAB);
	}

	// Allocate another block
	void* memory2 = Memory::Malloc(size);
	DESTAN_EXPECT(memory2 != nullptr);
	DESTAN_EXPECT(memory2 != memory);

	// Copy memory from first block to second
	Memory::Memcpy(memory2, memory, size);

	// Verify memory was copied correctly
	destan_u8* bytes2 = static_cast<destan_u8*>(memory2);
	for (destan_u64 i = 0; i < size; i++)
	{
		DESTAN_EXPECT(bytes2[i] == 0xAB);
	}

	// Change the second memory block
	Memory::Memset(memory2, 0xCD, size);

	// Verify the blocks are different now
	for (destan_u64 i = 0; i < size; i++)
	{
		DESTAN_EXPECT(bytes[i] == 0xAB);
		DESTAN_EXPECT(bytes2[i] == 0xCD);
	}

	// Free the memory blocks
	Memory::Free(memory);
	Memory::Free(memory2);

	// Dump memory stats for verification
	Memory::Dump_Memory_Stats();

	// Check for memory leaks
	Memory::Check_Memory_Leaks();

	// Shutdown memory system
	Memory::Shutdown();

	return true;
}

// Test memory alignment
bool Test_Memory_Alignment()
{
	Memory::Initialize();

	// Test different alignments
	const destan_u64 alignments[] = { 4, 8, 16, 32, 64, 128, 256 };
	
	for (auto alignment : alignments)
	{
		void* memory = Memory::Malloc(1024, alignment);
		DESTAN_EXPECT(memory != nullptr);

		// Verify the memory is properly aligned
		destan_uiptr address = reinterpret_cast<destan_uiptr>(memory);
		DESTAN_EXPECT((address % alignment) == 0);
		Memory::Free(memory);
		Memory::Dump_Memory_Stats();
	}	

	Memory::Shutdown();
	return true;
};

// Test memory reallocation
bool Test_Memory_Realloc()
{
	Memory::Initialize();

	// Allocate initial memory
	const destan_u64 initial_size = 128;
	destan_u8* memory = static_cast<destan_u8*>(Memory::Malloc(initial_size));
	DESTAN_EXPECT(memory != nullptr);

	// Fill with a pattern
	for (destan_u64 i = 0; i < initial_size; i++)
	{
		memory[i] = static_cast<destan_u8>(i % 256);
	}

	// Reallocate to a larger size
	const destan_u64 new_size = 256;
	destan_u8* new_memory = static_cast<destan_u8*>(Memory::Realloc(memory, new_size));
	DESTAN_EXPECT(new_memory != nullptr);

	// Verify the original data is preserved
	for (destan_u64 i = 0; i < initial_size; i++)
	{
		DESTAN_EXPECT(new_memory[i] == static_cast<destan_u8>(i % 256));
	}

	// Free the memory
	Memory::Free(new_memory);
	Memory::Dump_Memory_Stats();

	Memory::Shutdown();

	return true;
}

// Test memory stress
bool Test_Memory_Stress()
{
	Memory::Initialize();

	// Allocate many small blocks
	const destan_i32 allocation_count = 1000;
	void* allocations[allocation_count];

	for (destan_i32 i = 0; i < allocation_count; i++)
	{
		// Random size between 16 and 1024 bytes
		destan_u64 size = 16 + (rand() % 1008);
		allocations[i] = Memory::Malloc(size);
		DESTAN_EXPECT(allocations[i] != nullptr);
	}

	// Free in random order
	for (destan_i32 i = 0; i < allocation_count; i++)
	{
		destan_i32 index = rand() % allocation_count;
		// Swap with the last one if not already freed
		if (allocations[index] != nullptr)
		{
			Memory::Free(allocations[index]);
			allocations[index] = nullptr;
		}
		else
		{
			// Try again if this one was already freed
			i--;
		}
	}

	Memory::Dump_Memory_Stats();
	Memory::Shutdown();

	return true;
}

// Run all memory tests
int main(int argc, char** argv)
{
	return Test_Runner::Run_Tests([]()
	{
		Test_Suite memory_tests("Core Memory Tests");

		DESTAN_TEST(memory_tests, "Basic Memory Operations")
		{
			return Test_Memory_Basic();
		});

		DESTAN_TEST(memory_tests, "Memory Alignment")
		{
			return Test_Memory_Alignment();
		});

		DESTAN_TEST(memory_tests, "Memory Reallocation")
		{
			return Test_Memory_Realloc();
		});

		DESTAN_TEST(memory_tests, "Memory Stress Test")
		{
			return Test_Memory_Stress();
		});

		destan::test::arena_allocator::Add_All_Tests(memory_tests);

		return memory_tests.Run_All();
	});
}
