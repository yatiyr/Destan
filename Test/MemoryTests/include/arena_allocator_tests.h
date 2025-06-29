#include <core/ds_pch.h>
#include <core/memory/arena_allocator.h>
#include <test_framework.h>

using namespace ds::core::memory;
using namespace ds::test;

namespace ds::test::arena_allocator
{
    // Test struct to verify object construction and alignment
    struct Test_Object
    {
        int x, y, z;
        float f;
        char c;

        Test_Object() : x(0), y(0), z(0), f(0.0f), c(0) {}

        Test_Object(int x_val, int y_val, int z_val, float f_val, char c_val)
            : x(x_val), y(y_val), z(z_val), f(f_val), c(c_val) {}
    };

    // Test basic allocation functionality
    static bool Test_Arena_Basic()
    {
        // Create an arena with 1KB size
        const ds_u64 arena_size = 1024;
        Arena_Allocator arena(arena_size, "TestArena");

        // Verify initial state
        DS_EXPECT(arena.Get_Size() == arena_size);
        DS_EXPECT(arena.Get_Used_Size() == 0);
        DS_EXPECT(arena.Get_Free_Size() == arena_size);
        DS_EXPECT(arena.Get_Allocation_Count() == 0);
        DS_EXPECT(arena.Get_Utilization() == 0.0f);

        // Allocate memory
        const ds_u64 alloc_size = 128;
        void* mem1 = arena.Allocate(alloc_size);

        // Verify allocation succeeded
        DS_EXPECT(mem1 != nullptr);

        // Verify arena state after allocation
        DS_EXPECT(arena.Get_Used_Size() > 0);
        DS_EXPECT(arena.Get_Used_Size() <= alloc_size + DEFAULT_ALIGNMENT); // Account for possible alignment padding
        DS_EXPECT(arena.Get_Free_Size() == arena_size - arena.Get_Used_Size());
        DS_EXPECT(arena.Get_Allocation_Count() == 1);

        // Write to memory to ensure it's usable
        std::memset(mem1, 0xAB, alloc_size);

        // Allocate a second block
        void* mem2 = arena.Allocate(alloc_size);

        // Verify second allocation
        DS_EXPECT(mem2 != nullptr);
        DS_EXPECT(mem2 != mem1); // Should be a different address
        DS_EXPECT(static_cast<ds_u8*>(mem2) > static_cast<ds_u8*>(mem1)); // Should be after first allocation

        // Verify arena state after second allocation
        DS_EXPECT(arena.Get_Allocation_Count() == 2);

        // Reset the arena
        arena.Reset();

        // Verify reset state
        DS_EXPECT(arena.Get_Used_Size() == 0);
        DS_EXPECT(arena.Get_Free_Size() == arena_size);
        DS_EXPECT(arena.Get_Allocation_Count() == 0);

        return true;
    }

    // Test alignment functionality
    static bool Test_Arena_Alignment()
    {
        // Create an arena
        Arena_Allocator arena(4096, "AlignmentArena");

        // Test different alignments
        const ds_u64 alignments[] = { 4, 8, 16, 32, 64, 128, 256 };
        const ds_u64 size = 32;

        for (auto alignment : alignments)
        {
            void* mem = arena.Allocate(size, alignment);
            DS_EXPECT(mem != nullptr);

            // Verify memory is properly aligned
            ds_uiptr address = reinterpret_cast<ds_uiptr>(mem);
            DS_EXPECT((address % alignment) == 0);
        }

        // Reset for next test
        arena.Reset();

        return true;
    }

    // Test allocation limits and out-of-memory conditions
    static bool Test_Arena_Limits()
    {
        // Create a small arena
        const ds_u64 arena_size = 256;
        Arena_Allocator arena(arena_size, "SmallArena");

        // Allocate almost all memory
        void* mem1 = arena.Allocate(arena_size - 32);
        DS_EXPECT(mem1 != nullptr);

        // Small allocation should succeed
        void* mem2 = arena.Allocate(16);
        DS_EXPECT(mem2 != nullptr);

        // This allocation should fail as we're out of memory
        // So we need to see an error in the logs here don't worry
        DS_LOG_WARN("{}", DS_STYLED(DS_CONSOLE_FG_BRIGHT_BLUE DS_CONSOLE_TEXT_BLINK, "There should be an error right below!"));
        void* mem3 = arena.Allocate(arena_size);
        DS_EXPECT(mem3 == nullptr);

        // Deallocate should have no effect in arena allocator
        arena.Deallocate(mem1);
        DS_EXPECT(arena.Get_Free_Size() == arena_size - arena.Get_Used_Size());

        // Only reset should free memory
        arena.Reset();
        DS_EXPECT(arena.Get_Free_Size() == arena_size);

        // Now allocation should succeed
        mem3 = arena.Allocate(arena_size);
        DS_EXPECT(mem3 != nullptr);

        // Reset the arena so that we don't have active allocations left before
        // destroying the arena object
        arena.Reset();

        return true;
    }


    // Test object creation and array creation
    static bool Test_Arena_Object_Creation()
    {
        // Create arena
        Arena_Allocator arena(4096, "ObjectArena");

        // Create single object
        Test_Object* obj = arena.Create<Test_Object>(1, 2, 3, 4.0f, 'A');
        DS_EXPECT(obj != nullptr);
        DS_EXPECT(obj->x == 1);
        DS_EXPECT(obj->y == 2);
        DS_EXPECT(obj->z == 3);
        DS_EXPECT(obj->f == 4.0f);
        DS_EXPECT(obj->c == 'A');

        // Create array of objects
        const ds_u64 array_count = 10;
        Test_Object* objs = arena.Create_Array<Test_Object>(array_count);
        DS_EXPECT(objs != nullptr);

        // Verify array elements are default initialized
        for (ds_u64 i = 0; i < array_count; i++)
        {
            DS_EXPECT(objs[i].x == 0);
            DS_EXPECT(objs[i].y == 0);
            DS_EXPECT(objs[i].z == 0);
            DS_EXPECT(objs[i].f == 0.0f);
            DS_EXPECT(objs[i].c == 0);

            // Modify the objects to verify they're writable
            objs[i].x = static_cast<ds_i32>(i);
            objs[i].y = static_cast<ds_i32>(i * 2);
            objs[i].z = static_cast<ds_i32>(i * 3);
        }

        // Verify modifications
        for (ds_u64 i = 0; i < array_count; i++)
        {
            DS_EXPECT(objs[i].x == static_cast<ds_i32>(i));
            DS_EXPECT(objs[i].y == static_cast<ds_i32>(i * 2));
            DS_EXPECT(objs[i].z == static_cast<ds_i32>(i * 3));
        }

        // Reset the arena before destroying it so we don't have active allocations
        // left
        arena.Reset();

        return true;
    }

    // Test move operations
    static bool Test_Arena_Move_Operations()
    {
        // Create source arena
        Arena_Allocator arena1(1024, "SourceArena");

        // Allocate some memory
        void* mem1 = arena1.Allocate(128);
        DS_EXPECT(mem1 != nullptr);
        DS_EXPECT(arena1.Get_Allocation_Count() == 1);

        // Move-construct a new arena
        Arena_Allocator arena2(std::move(arena1));

        // Verify source arena is empty
        DS_EXPECT(arena1.Get_Size() == 0);
        DS_EXPECT(arena1.Get_Allocation_Count() == 0);

        // Verify destination arena has the resources
        DS_EXPECT(arena2.Get_Size() == 1024);
        DS_EXPECT(arena2.Get_Allocation_Count() == 1);

        // Create new source arena
        Arena_Allocator arena3(1024, "SourceArena2");
        void* mem3 = arena3.Allocate(256);
        DS_EXPECT(mem3 != nullptr);

        // Move-assign to destination arena
        arena2 = std::move(arena3);

        // Verify final state
        DS_EXPECT(arena3.Get_Size() == 0);
        DS_EXPECT(arena3.Get_Allocation_Count() == 0);
        DS_EXPECT(arena2.Get_Size() == 1024);
        DS_EXPECT(arena2.Get_Allocation_Count() == 1);

        // Reset arena2 before destruction, we don't need to reset arena1
        // because we passed the ownership of arena1 to arena2
        arena2.Reset();

        return true;
    }

    // Test multiple allocations performance and behavior
    static bool Test_Arena_Multiple_Allocations()
    {
        // Create a large arena
        const ds_u64 arena_size = 1024 * 1024; // 1MB
        Arena_Allocator arena(arena_size, "LargeArena");

        // Perform many small allocations
        const int allocation_count = 1000;
        std::vector<void*> allocations;

        for (int i = 0; i < allocation_count; i++)
        {
            // Random size between 16 and 1024 bytes
            ds_u64 size = 16 + (rand() % 1008);
            void* mem = arena.Allocate(size);
            DS_EXPECT(mem != nullptr);
            allocations.push_back(mem);

            // Write a pattern to the memory
            std::memset(mem, i % 256, size);
        }

        // Verify allocations don't overlap by checking the patterns
        for (int i = 0; i < allocation_count; i++)
        {
            // Read the first byte, which should match our pattern
            ds_u8 expected = static_cast<ds_u8>(i % 256);
            ds_u8 actual = *static_cast<ds_u8*>(allocations[i]);
            DS_EXPECT(actual == expected);
        }

        // Ensure we've allocated the expected number
        DS_EXPECT(arena.Get_Allocation_Count() == allocation_count);

        // Reset the arena before destroying it so we don't have active allocations
        // left
        arena.Reset();

        return true;
    }

    static bool Add_All_Tests(Test_Suite& test_suite)
    {
        DS_TEST(test_suite, "Basic Arena Operations")
        {
            return Test_Arena_Basic();
        });

        DS_TEST(test_suite, "Arena Alignment")
        {
            return Test_Arena_Alignment();
        });

        DS_TEST(test_suite, "Arena Limits")
        {
            return Test_Arena_Limits();
        });

        DS_TEST(test_suite, "Arena Object Creation")
        {
            return Test_Arena_Object_Creation();
        });

        DS_TEST(test_suite, "Arena Move Operations")
        {
            return Test_Arena_Move_Operations();
        });

        DS_TEST(test_suite, "Arena Multiple Allocations")
        {
            return Test_Arena_Multiple_Allocations();
        });

        return true;
    }
}