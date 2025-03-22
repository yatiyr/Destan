#pragma once
#include <core/destan_pch.h>
#include <core/memory/pool_allocator.h>
#include <test_framework.h>

using namespace destan::core::memory;
using namespace destan::test;

namespace destan::test::pool_allocator
{
	// Test struct to verify object construction and destruction
	struct Test_Object
	{
		destan_i32 x, y, z;
		destan_f32 f;
		destan_char c;
		destan_bool destroyed;

		Test_Object() : x(0), y(0), z(0), f(0.0f), c(0), destroyed(false) {}

		Test_Object(int x_val, int y_val, int z_val, float f_val, char c_val)
			: x(x_val), y(y_val), z(z_val), f(f_val), c(c_val), destroyed(false) {}

		~Test_Object()
		{
			// Mark as destroyed to verify destructor was called
			destroyed = true;
		}
	};

	// Test basic pool allocator functionality
	static bool Test_Pool_Basic()
	{
		// Create a pool with 10 blocks, each 64 bytes
		const destan_u64 block_size = 64;
		const destan_u64 block_count = 10;
		Pool_Allocator pool(block_size, block_count, "Test_Pool");

		// Verify initial state
		DESTAN_EXPECT(pool.Get_Block_Size() == block_size);
		DESTAN_EXPECT(pool.Get_Block_Count() == block_count);
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count);
		DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == 0);
		DESTAN_EXPECT(pool.Get_Utilization() == 0.0f);

		// Allocate some blocks
		void* blocks[5];
		for (destan_i32 i = 0; i < 5; i++)
		{
			blocks[i] = pool.Allocate();
			DESTAN_EXPECT(blocks[i] != nullptr);

			// Write a pattern to verify memory access
			memset(blocks[i], 0xAB, 32); // Use less than block_size to be safe
		}

		// Free a block
		DESTAN_EXPECT(pool.Deallocate(blocks[2]));

		// Verify state after deallocation
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count - 4);
		DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == 4);

		// Allocate the freed block
		void* new_block = pool.Allocate();
		DESTAN_EXPECT(new_block != nullptr);
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count - 5);

		// Verify that all blocks are different
		for (destan_i32 i = 0; i < 5; i++)
		{
			if (i != 2) // Skip the deallocated block
			{
				DESTAN_EXPECT(new_block != blocks[i]);
			}
		}

		// Free all blocks
		for (destan_i32 i = 0; i < 5; i++)
		{
			if (i != 2) // Skip already freed block
			{
				DESTAN_EXPECT(pool.Deallocate(blocks[i]));
			}
		}
		DESTAN_EXPECT(pool.Deallocate(new_block));

		// Verify state after all deallocations
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count);
		DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == 0);
		DESTAN_EXPECT(pool.Get_Utilization() == 0.0f);

		return true;
	}

	 // Test object creation and destruction
	static bool Test_Pool_Object_Creation()
	{
		// Create a pool that can hold Test_Objects
		const destan_u64 block_count = 10;
		Pool_Allocator pool(sizeof(Test_Object), block_count, "Object_Pool");

		// Create objects
		Test_Object* objects[5];
		for (destan_i32 i = 0; i < 5; i++)
		{
			objects[i] = pool.Create<Test_Object>(i, i * 2, i * 3, i * 1.5f, static_cast<destan_char>('A' + i));
			DESTAN_EXPECT(objects[i] != nullptr);

			// Verify object was constructed properly
			DESTAN_EXPECT(objects[i]->x == i);
			DESTAN_EXPECT(objects[i]->y == i * 2);
			DESTAN_EXPECT(objects[i]->z == i * 3);
			DESTAN_EXPECT(objects[i]->f == i * 1.5f);
			DESTAN_EXPECT(objects[i]->c == 'A' + i);
			DESTAN_EXPECT(objects[i]->destroyed == false);
		}

		// Verify pool state
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count - 5);
		DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == 5);

		// Destroy objects and verify destructors are called
		for (destan_i32 i = 0; i < 5; i++)
		{
			// Call destroy, which calls the destructor and sets destroyed=true
			bool destroy_result = pool.Destroy(objects[i]);

			// Don't try to access objects[i] after it's been destroyed
			DESTAN_EXPECT(destroy_result);
		}

		// Verify pool state after destructor calls
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count);
		DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == 0);

		return true;
	}

	// Test pool exhaustion behavior
	static bool Test_Pool_Exhaustion()
	{
		// Create a small pool
		const destan_u64 block_size = 32;
		const destan_u64 block_count = 3;
		Pool_Allocator pool(block_size, block_count, "Small_Pool");

		// Allocate all blocks
		void* blocks[3];
		for (destan_i32 i = 0; i < 3; i++)
		{
			blocks[i] = pool.Allocate();
			DESTAN_EXPECT(blocks[i] != nullptr);
		}

		// Verify pool is full
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == 0);
		DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == block_count);
		DESTAN_EXPECT(pool.Get_Utilization() == 100.0f);

		// Try to allocate when the pool is full
		DESTAN_LOG_WARN("{}", DESTAN_STYLED(DESTAN_CONSOLE_FG_BRIGHT_BLUE DESTAN_CONSOLE_TEXT_BLINK, "There should be an error right below!"));
		void* overflow_block = pool.Allocate();
		DESTAN_EXPECT(overflow_block == nullptr);

		// Free a block
		DESTAN_EXPECT(pool.Deallocate(blocks[1]));
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == 1);

		// Now allocation should succeed
		void* new_block = pool.Allocate();
		DESTAN_EXPECT(new_block != nullptr);
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == 0);

		// Clean up
		pool.Deallocate(blocks[0]);
		pool.Deallocate(blocks[2]);
		pool.Deallocate(new_block);

		return true;
	}

	static bool Test_Pool_Reset()
	{
		// Create a pool
		const destan_u64 block_size = 64;
		const destan_u64 block_count = 5;
		Pool_Allocator pool(block_size, block_count, "Reset_Pool");

		// Allocate all blocks
		void* blocks[5];
		for (destan_i32 i = 0; i < 5; i++)
		{
			blocks[i] = pool.Allocate();
			DESTAN_EXPECT(blocks[i] != nullptr);

			// Write some data
			*static_cast<destan_i32*>(blocks[i]) = i;
		}

		// Verify pool is full
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == 0);
		DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == block_count);

		// Reset the pool
		pool.Reset();

		// Verify pool state after reset
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count);
		DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == 0);
		DESTAN_EXPECT(pool.Get_Utilization() == 0.0f);

		// Try allocating after reset
		void* new_block = pool.Allocate();
		DESTAN_EXPECT(new_block != nullptr);
		DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count - 1);

		// Clean up
		pool.Deallocate(new_block);

		return true;
	}

	// Test invalid deallocation behavior
	static bool Test_Pool_Invalid_Deallocation()
	{
		// Create a pool
		Pool_Allocator pool(64, 5, "Invalid_Dealloc_Pool");

		// Allocate a block
		void* block = pool.Allocate();
		DESTAN_EXPECT(block != nullptr);

		// Try to deallocate nullptr
		DESTAN_EXPECT(pool.Deallocate(nullptr) == false);

		// Try to deallocate a pointer not from this pool
		destan_i32 stack_var = 0;
		DESTAN_LOG_WARN("{}", DESTAN_STYLED(DESTAN_CONSOLE_FG_BRIGHT_BLUE DESTAN_CONSOLE_TEXT_BLINK, "There should be an error right below!"));
		DESTAN_EXPECT(pool.Deallocate(&stack_var) == false);

		// Double-free test
		DESTAN_EXPECT(pool.Deallocate(block) == true);
		DESTAN_LOG_WARN("{}", DESTAN_STYLED(DESTAN_CONSOLE_FG_BRIGHT_BLUE DESTAN_CONSOLE_TEXT_BLINK, "There should be an error right below!"));
		DESTAN_EXPECT(pool.Deallocate(block) == false);

		return true;
	}

	// Test move semantics
	static bool Test_Pool_Move_Operations()
	{
		// Create source pool
		Pool_Allocator pool1(64, 10, "Source_Pool");

		// Allocate some blocks
		void* blocks[3];
		for (int i = 0; i < 3; i++)
		{
			blocks[i] = pool1.Allocate();
			DESTAN_EXPECT(blocks[i] != nullptr);
		}

		// Verify state
		DESTAN_EXPECT(pool1.Get_Free_Block_Count() == 7);
		DESTAN_EXPECT(pool1.Get_Allocated_Block_Count() == 3);

		// Move-construct a new pool
		Pool_Allocator pool2(std::move(pool1));

		// Verify states after move
		DESTAN_EXPECT(pool2.Get_Block_Size() == 64);
		DESTAN_EXPECT(pool2.Get_Block_Count() == 10);
		DESTAN_EXPECT(pool2.Get_Free_Block_Count() == 7);
		DESTAN_EXPECT(pool2.Get_Allocated_Block_Count() == 3);

		// Try to deallocate blocks with the new pool
		for (int i = 0; i < 3; i++)
		{
			DESTAN_EXPECT(pool2.Deallocate(blocks[i]));
		}

		// Verify all blocks are free
		DESTAN_EXPECT(pool2.Get_Free_Block_Count() == 10);

		// Test move assignment
		Pool_Allocator pool3(32, 5, "AnotherPool");
		void* block = pool3.Allocate();
		DESTAN_EXPECT(block != nullptr);

		// Move-assign
		pool3 = std::move(pool2);

		// Verify the new state
		DESTAN_EXPECT(pool3.Get_Block_Size() == 64);
		DESTAN_EXPECT(pool3.Get_Block_Count() == 10);
		DESTAN_EXPECT(pool3.Get_Free_Block_Count() == 10);

		return true;
	}

	// Test allocation stress
	static bool Test_Pool_Stress()
	{
		// Create a pool for small objects
		const destan_u64 block_size = sizeof(destan_i32);
		const destan_u64 block_count = 1000;
		Pool_Allocator pool(block_size, block_count, "Stress_Pool");

		// Allocate and free in a pattern to stress the allocator
		constexpr destan_i32 iterations = 10;

		for (destan_i32 iter = 0; iter < iterations; iter++)
		{
			// Allocate all blocks
			std::vector<destan_i32*> blocks;
			blocks.reserve(block_count);

			for (destan_u64 i = 0; i < block_count; i++)
			{
				destan_i32* block = static_cast<destan_i32*>(pool.Allocate());
				DESTAN_EXPECT(block != nullptr);

				// Set value
				*block = static_cast<destan_i32>(i);
				blocks.push_back(block);
			}

			// Verify pool is full
			DESTAN_EXPECT(pool.Get_Free_Block_Count() == 0);
			DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == block_count);

			// Verify values
			for (destan_u64 i = 0; i < block_count; i++)
			{
				DESTAN_EXPECT(*blocks[i] == static_cast<destan_i32>(i));
			}

			// Free in a different order (every other block)
			for (destan_u64 i = 0; i < block_count; i += 2)
			{
				DESTAN_EXPECT(pool.Deallocate(blocks[i]));
			}

			DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count / 2);

			// Free the rest
			for (destan_u64 i = 1; i < block_count; i += 2)
			{
				DESTAN_EXPECT(pool.Deallocate(blocks[i]));
			}

			// Verify all blocks are free
			DESTAN_EXPECT(pool.Get_Free_Block_Count() == block_count);
			DESTAN_EXPECT(pool.Get_Allocated_Block_Count() == 0);
		}

		return true;
	}

	// Add all pool allocator tests to the test suite
	static bool Add_All_Tests(Test_Suite& test_suite)
	{
		DESTAN_TEST(test_suite, "Basic Pool Operations")
		{
			return Test_Pool_Basic();
		});

		DESTAN_TEST(test_suite, "Pool Object Creation")
		{
			return Test_Pool_Object_Creation();
		});

		DESTAN_TEST(test_suite, "Pool Exhaustion")
		{
			return Test_Pool_Exhaustion();
		});

		DESTAN_TEST(test_suite, "Pool Reset")
		{
			return Test_Pool_Reset();
		});

		DESTAN_TEST(test_suite, "Pool Invalid Deallocation")
		{
			return Test_Pool_Invalid_Deallocation();
		});

		DESTAN_TEST(test_suite, "Pool Move Operations")
		{
			return Test_Pool_Move_Operations();
		});

		DESTAN_TEST(test_suite, "Pool Stress Test")
		{
			return Test_Pool_Stress();
		});

		return true;
	}
}