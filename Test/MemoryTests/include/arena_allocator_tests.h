#include <core/destan_pch.h>
#include <core/memory/arena_allocator.h>
#include <test_framework.h>

using namespace destan::core::memory;
using namespace destan::test;

namespace destan::test::arena_allocator
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
        const destan_u64 arena_size = 1024;
        Arena_Allocator arena(arena_size, "TestArena");

        // Verify initial state
        DESTAN_EXPECT(arena.Get_Size() == arena_size);
        DESTAN_EXPECT(arena.Get_Used_Size() == 0);
        DESTAN_EXPECT(arena.Get_Free_Size() == arena_size);
        DESTAN_EXPECT(arena.Get_Allocation_Count() == 0);
        DESTAN_EXPECT(arena.Get_Utilization() == 0.0f);

        // Allocate memory
        const destan_u64 alloc_size = 128;
        void* mem1 = arena.Allocate(alloc_size);

        // Verify allocation succeeded
        DESTAN_EXPECT(mem1 != nullptr);

        // Verify arena state after allocation
        DESTAN_EXPECT(arena.Get_Used_Size() > 0);
        DESTAN_EXPECT(arena.Get_Used_Size() <= alloc_size + DEFAULT_ALIGNMENT); // Account for possible alignment padding
        DESTAN_EXPECT(arena.Get_Free_Size() == arena_size - arena.Get_Used_Size());
        DESTAN_EXPECT(arena.Get_Allocation_Count() == 1);

        // Write to memory to ensure it's usable
        std::memset(mem1, 0xAB, alloc_size);

        // Allocate a second block
        void* mem2 = arena.Allocate(alloc_size);

        // Verify second allocation
        DESTAN_EXPECT(mem2 != nullptr);
        DESTAN_EXPECT(mem2 != mem1); // Should be a different address
        DESTAN_EXPECT(static_cast<destan_u8*>(mem2) > static_cast<destan_u8*>(mem1)); // Should be after first allocation

        // Verify arena state after second allocation
        DESTAN_EXPECT(arena.Get_Allocation_Count() == 2);

        // Reset the arena
        arena.Reset();

        // Verify reset state
        DESTAN_EXPECT(arena.Get_Used_Size() == 0);
        DESTAN_EXPECT(arena.Get_Free_Size() == arena_size);
        DESTAN_EXPECT(arena.Get_Allocation_Count() == 0);

        return true;
    }

    // Test alignment functionality
    static bool Test_Arena_Alignment()
    {
        // Create an arena
        Arena_Allocator arena(4096, "AlignmentArena");

        // Test different alignments
        const destan_u64 alignments[] = { 4, 8, 16, 32, 64, 128, 256 };
        const destan_u64 size = 32;

        for (auto alignment : alignments)
        {
            void* mem = arena.Allocate(size, alignment);
            DESTAN_EXPECT(mem != nullptr);

            // Verify memory is properly aligned
            destan_uiptr address = reinterpret_cast<destan_uiptr>(mem);
            DESTAN_EXPECT((address % alignment) == 0);
        }

        // Reset for next test
        arena.Reset();

        return true;
    }

    // Test allocation limits and out-of-memory conditions
    static bool Test_Arena_Limits()
    {
        // Create a small arena
        const destan_u64 arena_size = 256;
        Arena_Allocator arena(arena_size, "SmallArena");

        // Allocate almost all memory
        void* mem1 = arena.Allocate(arena_size - 32);
        DESTAN_EXPECT(mem1 != nullptr);

        // Small allocation should succeed
        void* mem2 = arena.Allocate(16);
        DESTAN_EXPECT(mem2 != nullptr);

        // This allocation should fail as we're out of memory
        // So we need to see an error in the logs here don't worry
        DESTAN_LOG_WARN("{}", DESTAN_STYLED(DESTAN_CONSOLE_FG_BRIGHT_BLUE DESTAN_CONSOLE_TEXT_BLINK, "There should be an error right below!"));
        void* mem3 = arena.Allocate(arena_size);
        DESTAN_EXPECT(mem3 == nullptr);

        // Deallocate should have no effect in arena allocator
        arena.Deallocate(mem1);
        DESTAN_EXPECT(arena.Get_Free_Size() == arena_size - arena.Get_Used_Size());

        // Only reset should free memory
        arena.Reset();
        DESTAN_EXPECT(arena.Get_Free_Size() == arena_size);

        // Now allocation should succeed
        mem3 = arena.Allocate(arena_size);
        DESTAN_EXPECT(mem3 != nullptr);

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
        DESTAN_EXPECT(obj != nullptr);
        DESTAN_EXPECT(obj->x == 1);
        DESTAN_EXPECT(obj->y == 2);
        DESTAN_EXPECT(obj->z == 3);
        DESTAN_EXPECT(obj->f == 4.0f);
        DESTAN_EXPECT(obj->c == 'A');

        // Create array of objects
        const destan_u64 array_count = 10;
        Test_Object* objs = arena.Create_Array<Test_Object>(array_count);
        DESTAN_EXPECT(objs != nullptr);

        // Verify array elements are default initialized
        for (destan_u64 i = 0; i < array_count; i++)
        {
            DESTAN_EXPECT(objs[i].x == 0);
            DESTAN_EXPECT(objs[i].y == 0);
            DESTAN_EXPECT(objs[i].z == 0);
            DESTAN_EXPECT(objs[i].f == 0.0f);
            DESTAN_EXPECT(objs[i].c == 0);

            // Modify the objects to verify they're writable
            objs[i].x = static_cast<destan_i32>(i);
            objs[i].y = static_cast<destan_i32>(i * 2);
            objs[i].z = static_cast<destan_i32>(i * 3);
        }

        // Verify modifications
        for (destan_u64 i = 0; i < array_count; i++)
        {
            DESTAN_EXPECT(objs[i].x == static_cast<destan_i32>(i));
            DESTAN_EXPECT(objs[i].y == static_cast<destan_i32>(i * 2));
            DESTAN_EXPECT(objs[i].z == static_cast<destan_i32>(i * 3));
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
        DESTAN_EXPECT(mem1 != nullptr);
        DESTAN_EXPECT(arena1.Get_Allocation_Count() == 1);

        // Move-construct a new arena
        Arena_Allocator arena2(std::move(arena1));

        // Verify source arena is empty
        DESTAN_EXPECT(arena1.Get_Size() == 0);
        DESTAN_EXPECT(arena1.Get_Allocation_Count() == 0);

        // Verify destination arena has the resources
        DESTAN_EXPECT(arena2.Get_Size() == 1024);
        DESTAN_EXPECT(arena2.Get_Allocation_Count() == 1);

        // Create new source arena
        Arena_Allocator arena3(1024, "SourceArena2");
        void* mem3 = arena3.Allocate(256);
        DESTAN_EXPECT(mem3 != nullptr);

        // Move-assign to destination arena
        arena2 = std::move(arena3);

        // Verify final state
        DESTAN_EXPECT(arena3.Get_Size() == 0);
        DESTAN_EXPECT(arena3.Get_Allocation_Count() == 0);
        DESTAN_EXPECT(arena2.Get_Size() == 1024);
        DESTAN_EXPECT(arena2.Get_Allocation_Count() == 1);

        // Reset arena2 before destruction, we don't need to reset arena1
        // because we passed the ownership of arena1 to arena2
        arena2.Reset();

        return true;
    }

    // Test multiple allocations performance and behavior
    static bool Test_Arena_Multiple_Allocations()
    {
        // Create a large arena
        const destan_u64 arena_size = 1024 * 1024; // 1MB
        Arena_Allocator arena(arena_size, "LargeArena");

        // Perform many small allocations
        const int allocation_count = 1000;
        std::vector<void*> allocations;

        for (int i = 0; i < allocation_count; i++)
        {
            // Random size between 16 and 1024 bytes
            destan_u64 size = 16 + (rand() % 1008);
            void* mem = arena.Allocate(size);
            DESTAN_EXPECT(mem != nullptr);
            allocations.push_back(mem);

            // Write a pattern to the memory
            std::memset(mem, i % 256, size);
        }

        // Verify allocations don't overlap by checking the patterns
        for (int i = 0; i < allocation_count; i++)
        {
            // Read the first byte, which should match our pattern
            destan_u8 expected = static_cast<destan_u8>(i % 256);
            destan_u8 actual = *static_cast<destan_u8*>(allocations[i]);
            DESTAN_EXPECT(actual == expected);
        }

        // Ensure we've allocated the expected number
        DESTAN_EXPECT(arena.Get_Allocation_Count() == allocation_count);

        // Reset the arena before destroying it so we don't have active allocations
        // left
        arena.Reset();

        return true;
    }

    static bool Add_All_Tests(Test_Suite& test_suite)
    {
        DESTAN_TEST(test_suite, "Basic Arena Operations")
        {
            return Test_Arena_Basic();
        });

        DESTAN_TEST(test_suite, "Arena Alignment")
        {
            return Test_Arena_Alignment();
        });

        DESTAN_TEST(test_suite, "Arena Limits")
        {
            return Test_Arena_Limits();
        });

        DESTAN_TEST(test_suite, "Arena Object Creation")
        {
            return Test_Arena_Object_Creation();
        });

        DESTAN_TEST(test_suite, "Arena Move Operations")
        {
            return Test_Arena_Move_Operations();
        });

        DESTAN_TEST(test_suite, "Arena Multiple Allocations")
        {
            return Test_Arena_Multiple_Allocations();
        });

        return true;
    }
}