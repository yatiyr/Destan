#include <core/ds_pch.h>
#include <core/defines.h>
#include <core/memory/memory.h>
#include <test_framework.h>

#include <arena_allocator_tests.h>
#include <pool_allocator_tests.h>
#include <stack_allocator_tests.h>
#include <free_list_allocator_tests.h>
#include <page_allocator_tests.h>
#include <streaming_allocator_tests.h>

using namespace ds::core::memory;
using namespace ds::test;
using namespace ds;

// Test the basic memory functions
bool Test_Memory_Basic()
{
	// Initialize the memory system
	Memory::Initialize();

	// Allocate memory
	const ds_u64 size = 1024;
	void* memory = Memory::Malloc(size);

	// Verify allocation succeeded
	DS_EXPECT(memory != nullptr);

	// Write to memory
	Memory::Memset(memory, 0xAB, size);

	// Verify memory was written correctly
	ds_u8* bytes = static_cast<ds_u8*>(memory);
	for (ds_u64 i = 0; i < size; i++)
	{
		DS_EXPECT(bytes[i] == 0xAB);
	}

	// Allocate another block
	void* memory2 = Memory::Malloc(size);
	DS_EXPECT(memory2 != nullptr);
	DS_EXPECT(memory2 != memory);

	// Copy memory from first block to second
	Memory::Memcpy(memory2, memory, size);

	// Verify memory was copied correctly
	ds_u8* bytes2 = static_cast<ds_u8*>(memory2);
	for (ds_u64 i = 0; i < size; i++)
	{
		DS_EXPECT(bytes2[i] == 0xAB);
	}

	// Change the second memory block
	Memory::Memset(memory2, 0xCD, size);

	// Verify the blocks are different now
	for (ds_u64 i = 0; i < size; i++)
	{
		DS_EXPECT(bytes[i] == 0xAB);
		DS_EXPECT(bytes2[i] == 0xCD);
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
	const ds_u64 alignments[] = { 4, 8, 16, 32, 64, 128, 256 };
	
	for (auto alignment : alignments)
	{
		void* memory = Memory::Malloc(1024, alignment);
		DS_EXPECT(memory != nullptr);

		// Verify the memory is properly aligned
		ds_uiptr address = reinterpret_cast<ds_uiptr>(memory);
		DS_EXPECT((address % alignment) == 0);
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
	const ds_u64 initial_size = 128;
	ds_u8* memory = static_cast<ds_u8*>(Memory::Malloc(initial_size));
	DS_EXPECT(memory != nullptr);

	// Fill with a pattern
	for (ds_u64 i = 0; i < initial_size; i++)
	{
		memory[i] = static_cast<ds_u8>(i % 256);
	}

	// Reallocate to a larger size
	const ds_u64 new_size = 256;
	ds_u8* new_memory = static_cast<ds_u8*>(Memory::Realloc(memory, new_size));
	DS_EXPECT(new_memory != nullptr);

	// Verify the original data is preserved
	for (ds_u64 i = 0; i < initial_size; i++)
	{
		DS_EXPECT(new_memory[i] == static_cast<ds_u8>(i % 256));
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
	const ds_i32 allocation_count = 1000;
	void* allocations[allocation_count];

	for (ds_i32 i = 0; i < allocation_count; i++)
	{
		// Random size between 16 and 1024 bytes
		ds_u64 size = 16 + (rand() % 1008);
		allocations[i] = Memory::Malloc(size);
		DS_EXPECT(allocations[i] != nullptr);
	}

	// Free in random order
	for (ds_i32 i = 0; i < allocation_count; i++)
	{
		ds_i32 index = rand() % allocation_count;
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

		DS_TEST(memory_tests, "Basic Memory Operations")
		{
			return Test_Memory_Basic();
		});

		DS_TEST(memory_tests, "Memory Alignment")
		{
			return Test_Memory_Alignment();
		});

		DS_TEST(memory_tests, "Memory Reallocation")
		{
			return Test_Memory_Realloc();
		});

		DS_TEST(memory_tests, "Memory Stress Test")
		{
			return Test_Memory_Stress();
		});

		ds::test::arena_allocator::Add_All_Tests(memory_tests);
		ds::test::pool_allocator::Add_All_Tests(memory_tests);
		ds::test::stack_allocator::Add_All_Tests(memory_tests);
		ds::test::free_list_allocator::Add_All_Tests(memory_tests);
		ds::test::page_allocator::Add_All_Tests(memory_tests);
		ds::test::streaming_allocator::Add_All_Tests(memory_tests);

		bool result = memory_tests.Run_All();

		// Cleaning up the mess during the tests
		ds::test::streaming_allocator::Delete_Test_Files();

		return result;
	});
}
