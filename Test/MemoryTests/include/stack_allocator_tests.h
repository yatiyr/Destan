#pragma once
#include <core/ds_pch.h>
#include <core/memory/stack_allocator.h>
#include <test_framework.h>

using namespace ds::core::memory;
using namespace ds::test;

namespace ds::test::stack_allocator
{
	// Test struct for object creation and destruction
	struct Test_Object
	{
		ds_i32 x, y, z;
		ds_f32 f;
		ds_char c;
		static ds_i32 destruction_count;

		Test_Object() : x(0), y(0), z(0), f(0.0f), c(0) {}

		Test_Object(ds_i32 x_val, ds_i32 y_val, ds_i32 z_val, ds_f32 f_val, ds_char c_val)
			: x(x_val), y(y_val), z(z_val), f(f_val), c(c_val) {
		}

		~Test_Object()
		{
			// Increment destruction counter
			destruction_count++;
		}
	};

	// Initialize static counter
	ds_i32 Test_Object::destruction_count = 0;

	// Test basic stack allocator functionality
	static ds_bool Test_Stack_Basic()
	{
		// Create a stack allocator with 1KB size
		const ds_u64 stack_size = 1024;
		Stack_Allocator stack(stack_size, "Test_Stack");

		// Verify initial state
		DS_EXPECT(stack.Get_Size() == stack_size);
		DS_EXPECT(stack.Get_Used_Size() == 0);
		DS_EXPECT(stack.Get_Free_Size() == stack_size);
		DS_EXPECT(stack.Get_Utilization() == 0.0f);

		// Allocate some memory
		const ds_u64 alloc_size = 128;
		void* mem1 = stack.Allocate(alloc_size);

		// Verify allocation
		DS_EXPECT(mem1 != nullptr);
		DS_EXPECT(stack.Get_Used_Size() > 0);
		DS_EXPECT(stack.Get_Used_Size() <= alloc_size + DEFAULT_ALIGNMENT); // Account for alignment
		DS_EXPECT(stack.Get_Free_Size() == stack_size - stack.Get_Used_Size());

		// Write to memory
		memset(mem1, 0xAB, alloc_size);

		// Allocate second block
		void* mem2 = stack.Allocate(alloc_size);
		DS_EXPECT(mem2 != nullptr);
		DS_EXPECT(mem2 > mem1); // Should be at a higher address

		// Verify stack state
		ds_u64 used_after_second = stack.Get_Used_Size();
		DS_EXPECT(used_after_second > alloc_size);

		// Get a marker at current position
		Stack_Allocator::Marker marker = stack.Get_Marker();

		// Allocate a third block
		void* mem3 = stack.Allocate(alloc_size);
		DS_EXPECT(mem3 != nullptr);
		DS_EXPECT(stack.Get_Used_Size() > used_after_second);

		// Free back to the marker (should free only mem3)
		stack.Free_To_Marker(marker);

		// Verify we're back to the state after the second allocation
		DS_EXPECT(stack.Get_Used_Size() == used_after_second);

		// Allocate again - should reuse the space freed
		void* mem3b = stack.Allocate(alloc_size);
		DS_EXPECT(mem3b != nullptr);
		DS_EXPECT(mem3b == mem3); // Should be the same address

		// Reset the entire stack
		stack.Reset();

		// Verify stack is empty
		DS_EXPECT(stack.Get_Used_Size() == 0);
		DS_EXPECT(stack.Get_Free_Size() == stack_size);

		return true;
	}

	// Test object creation and deletion
	static ds_bool Test_Stack_Object_Creation()
	{
		// Create a stack allocator
		Stack_Allocator stack(4096, "Object_Stack");

		// Reset destruction counter
		Test_Object::destruction_count = 0;

		// Create a single object
		Test_Object* obj = stack.Create<Test_Object>(1, 2, 3, 4.0f, 'A');
		DS_EXPECT(obj != nullptr);
		DS_EXPECT(obj->x == 1);
		DS_EXPECT(obj->y == 2);
		DS_EXPECT(obj->z == 3);
		DS_EXPECT(obj->f == 4.0f);
		DS_EXPECT(obj->c == 'A');

		// Create an array of objects
		const ds_u64 array_count = 10;
		Test_Object* obj_array = stack.Create_Array<Test_Object>(array_count);
		DS_EXPECT(obj_array != nullptr);

		// Verify they're default initialized
		for (ds_u64 i = 0; i < array_count; ++i)
		{
			DS_EXPECT(obj_array[i].x == 0);
			DS_EXPECT(obj_array[i].y == 0);
			DS_EXPECT(obj_array[i].z == 0);
			DS_EXPECT(obj_array[i].f == 0.0f);
			DS_EXPECT(obj_array[i].c == 0);
		}

		// Get a marker before creating another object
		Stack_Allocator::Marker marker = stack.Get_Marker();

		// Create one more object
		Test_Object* obj2 = stack.Create<Test_Object>(5, 6, 7, 8.0f, 'B');
		DS_EXPECT(obj2 != nullptr);

		// Manually call the destructor for obj2 before freeing memory
		obj2->~Test_Object();

		// Verify manual destruction
		DS_EXPECT(Test_Object::destruction_count == 1);

		// Free to the marker - memory is freed but destructors are not called
		stack.Free_To_Marker(marker);

		// Manually call destructors for remaining objects
		obj->~Test_Object();
		for (ds_u64 i = 0; i < array_count; ++i)
		{
			obj_array[i].~Test_Object();
		}

		// Verify all objects were manually destroyed
		DS_EXPECT(Test_Object::destruction_count == 1 + 1 + array_count); // obj2 + obj + array

		// Reset everything - memory is freed but no additional destructors are called
		stack.Reset();

		// Destruction count should remain the same since we've already manually called all destructors
		DS_EXPECT(Test_Object::destruction_count == 1 + 1 + array_count);

		return true;
	}

	// Test stack scope helper
	static ds_bool Test_Stack_Scope()
	{
		// Create a stack allocator
		Stack_Allocator stack(4096, "ScopeStack");

		// Reset destruction counter
		Test_Object::destruction_count = 0;

		// Create objects outside any scope
		Test_Object* obj1 = stack.Create<Test_Object>(1, 2, 3, 4.0f, 'A');
		DS_EXPECT(obj1 != nullptr);

		{
			// Create a stack scope - this should automatically free memory when leaving the scope
			DS_STACK_SCOPE(stack);

			// Allocate objects inside the scope
			Test_Object* obj2 = stack.Create<Test_Object>(5, 6, 7, 8.0f, 'B');
			DS_EXPECT(obj2 != nullptr);

			// Create more objects
			Test_Object* obj3 = stack.Create<Test_Object>(9, 10, 11, 12.0f, 'C');
			DS_EXPECT(obj3 != nullptr);

			// Manually call destructors before leaving scope
			obj2->~Test_Object();
			obj3->~Test_Object();

			// Verify destruction count
			DS_EXPECT(Test_Object::destruction_count == 2);
		}
		// Scope ends here - memory is freed by DS_STACK_SCOPE

		// Verify obj1 is still accessible
		DS_EXPECT(obj1->x == 1);

		// Manually call destructor for obj1
		obj1->~Test_Object();

		// Verify destruction count
		DS_EXPECT(Test_Object::destruction_count == 3);

		// Reset the stack - this frees memory but doesn't call destructors
		stack.Reset();

		// Destruction count should remain unchanged
		DS_EXPECT(Test_Object::destruction_count == 3);

		return true;
	}

	// Test allocation alignment
	static ds_bool Test_Stack_Alignment()
	{
		// Create a stack allocator
		Stack_Allocator stack(4096, "AlignmentStack");

		// Test different alignments
		const ds_u64 alignments[] = { 4, 8, 16, 32, 64, 128, 256 };
		const ds_u64 size = 32;

		for (auto alignment : alignments)
		{
			void* ptr = stack.Allocate(size, alignment);
			DS_EXPECT(ptr != nullptr);

			// Verify alignment
			ds_uiptr address = reinterpret_cast<ds_uiptr>(ptr);
			DS_EXPECT((address % alignment) == 0);
		}

		stack.Reset();

		return true;
	}

	// Test out-of-memory conditions
	static ds_bool Test_Stack_Out_Of_Memory()
	{
		// Create a small stack
		const ds_u64 small_size = 256;
		Stack_Allocator stack(small_size, "SmallStack");

		// Allocate almost all memory
		void* mem1 = stack.Allocate(small_size - 64);
		DS_EXPECT(mem1 != nullptr);

		// Small allocation should succeed
		void* mem2 = stack.Allocate(32);
		DS_EXPECT(mem2 != nullptr);

		// This allocation should fail
		DS_LOG_WARN("{}", DS_STYLED(DS_CONSOLE_FG_BRIGHT_BLUE DS_CONSOLE_TEXT_BLINK, "There should be an error right below!"));
		void* mem3 = stack.Allocate(64);
		DS_EXPECT(mem3 == nullptr);

		// Reset the stack
		stack.Reset();

		// Now allocation should succeed
		mem3 = stack.Allocate(64);
		DS_EXPECT(mem3 != nullptr);

		stack.Reset();

		return true;
	}

	// Test Free_Latest functionality
	static ds_bool Test_Stack_Free_Latest()
	{
		// Create a stack allocator
		Stack_Allocator stack(1024, "LatestStack");

		// Allocate three blocks
		void* mem1 = stack.Allocate(64);
		void* mem2 = stack.Allocate(128);
		void* mem3 = stack.Allocate(256);

		DS_EXPECT(mem1 != nullptr);
		DS_EXPECT(mem2 != nullptr);
		DS_EXPECT(mem3 != nullptr);

		// Get current used size
		ds_u64 used_before_free = stack.Get_Used_Size();

		// Free the latest allocation (mem3)
		DS_EXPECT(stack.Free_Latest());

		// Verify mem3 was freed
		DS_EXPECT(stack.Get_Used_Size() < used_before_free);

		// Allocate again - should reuse the same space
		void* mem3b = stack.Allocate(256);
		DS_EXPECT(mem3b != nullptr);
		DS_EXPECT(mem3b == mem3);

		// Free all and verify
		DS_EXPECT(stack.Free_Latest()); // Free mem3b
		DS_EXPECT(stack.Free_Latest()); // Free mem2
		DS_EXPECT(stack.Free_Latest()); // Free mem1

		// Stack should be empty
		DS_EXPECT(stack.Get_Used_Size() == 0);

		// Trying to free from empty stack should fail
		DS_EXPECT(stack.Free_Latest() == false);

		return true;
	}

	// Test move operations
	static ds_bool Test_Stack_Move_Operations()
	{
		// Create source stack
		Stack_Allocator stack1(1024, "Source_Stack");

		// Allocate memory
		void* mem1 = stack1.Allocate(256);
		DS_EXPECT(mem1 != nullptr);

		// Verify state
		ds_u64 used_size = stack1.Get_Used_Size();

		// Move-construct a new stack
		Stack_Allocator stack2(std::move(stack1));

		// Verify stack2 has the resources
		DS_EXPECT(stack2.Get_Size() == 1024);
		DS_EXPECT(stack2.Get_Used_Size() == used_size);

		// stack1 should be empty
		DS_EXPECT(stack1.Get_Size() == 0);
		DS_EXPECT(stack1.Get_Used_Size() == 0);

		// Create a third stack 
		Stack_Allocator stack3(512, "Stack3");

		// Move-assign from stack2
		stack3 = std::move(stack2);

		// Verify stack3 now has the resources
		DS_EXPECT(stack3.Get_Size() == 1024);
		DS_EXPECT(stack3.Get_Used_Size() == used_size);

		// stack2 should be empty
		DS_EXPECT(stack2.Get_Size() == 0);
		DS_EXPECT(stack2.Get_Used_Size() == 0);

		stack3.Reset();
		return true;
	}

	// Add all tests to the test suite
	static ds_bool Add_All_Tests(Test_Suite& test_suite)
	{
		DS_TEST(test_suite, "Basic Stack Operations")
		{
			return Test_Stack_Basic();
		});

		DS_TEST(test_suite, "Stack Object Creation")
		{
			return Test_Stack_Object_Creation();
		});

		DS_TEST(test_suite, "Stack Scope")
		{
			return Test_Stack_Scope();
		});

		DS_TEST(test_suite, "Stack Alignment")
		{
			return Test_Stack_Alignment();
		});

		DS_TEST(test_suite, "Stack Out Of Memory")
		{
			return Test_Stack_Out_Of_Memory();
		});

		DS_TEST(test_suite, "Stack Free Latest")
		{
			return Test_Stack_Free_Latest();
		});

		DS_TEST(test_suite, "Stack Move Operations")
		{
			return Test_Stack_Move_Operations();
		});

		return true;
	}
}