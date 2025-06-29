#pragma once
#include <core/ds_pch.h>
#include <core/memory/free_list_allocator.h>
#include <test_framework.h>

using namespace ds::core::memory;
using namespace ds::test;

namespace ds::test::free_list_allocator
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
            : x(x_val), y(y_val), z(z_val), f(f_val), c(c_val) {}

        ~Test_Object()
        {
            // Increment destruction counter
            destruction_count++;
        }
    };

    // Initialize static counter
    ds_i32 Test_Object::destruction_count = 0;

    // Test basic free list allocator functionality
    static ds_bool Test_Free_List_Basic()
    {
        // Create a free list allocator with 1MB size
        const ds_u64 allocator_size = 1024 * 1024; // 1MB
        Free_List_Allocator allocator(allocator_size, Free_List_Allocator::Allocation_Strategy::FIND_FIRST, "TestFreeList");

        // Verify initial state
        DS_EXPECT(allocator.Get_Size() == allocator_size);
        DS_EXPECT(allocator.Get_Free_Size() == allocator_size);
        DS_EXPECT(allocator.Get_Used_Size() == 0);
        DS_EXPECT(allocator.Get_Free_Block_Count() == 1); // Initially one big free block
        DS_EXPECT(allocator.Get_Utilization() == 0.0f);

        // Allocate memory
        const ds_u64 alloc_size = 1024;
        void* mem1 = allocator.Allocate(alloc_size);

        // Verify allocation succeeded
        DS_EXPECT(mem1 != nullptr);
        DS_EXPECT(allocator.Get_Used_Size() > 0);
        DS_EXPECT(allocator.Get_Free_Size() < allocator_size);

        // Write to memory
        std::memset(mem1, 0xAB, alloc_size);

        // Allocate a second block
        void* mem2 = allocator.Allocate(alloc_size);
        DS_EXPECT(mem2 != nullptr);
        DS_EXPECT(mem2 != mem1); // Should be different addresses

        // Deallocate first block
        ds_u64 before_free_size = allocator.Get_Free_Size();
        ds_bool free_result = allocator.Deallocate(mem1);
        DS_EXPECT(free_result);

        // Verify memory was freed
        DS_EXPECT(allocator.Get_Free_Size() > before_free_size);

        // Allocate a new block that should fit in the freed space
        void* mem3 = allocator.Allocate(alloc_size);
        DS_EXPECT(mem3 != nullptr);

        // In a first-fit strategy, this might reuse the same address as mem1
        // Depending on implementation details and header sizes
        // So we won't check for address equality

        // Deallocate remaining blocks
        DS_EXPECT(allocator.Deallocate(mem2));
        DS_EXPECT(allocator.Deallocate(mem3));

        // Verify all memory is freed
        DS_EXPECT(allocator.Get_Used_Size() == 0);
        DS_EXPECT(allocator.Get_Free_Size() == allocator_size);

        return true;
    }

    // Test different allocation strategies
    static ds_bool Test_Free_List_Strategies()
    {
        const ds_u64 allocator_size = 1024 * 1024; // 1MB
        const ds_u64 alloc_size = 1024;

        // Test First Fit strategy
        {
            Free_List_Allocator allocator(allocator_size, Free_List_Allocator::Allocation_Strategy::FIND_FIRST, "FirstFitFreeList");
            DS_EXPECT(allocator.Get_Strategy() == Free_List_Allocator::Allocation_Strategy::FIND_FIRST);

            // Allocate several blocks
            void* blocks[10];
            for (ds_i32 i = 0; i < 10; i++)
            {
                blocks[i] = allocator.Allocate(alloc_size);
                DS_EXPECT(blocks[i] != nullptr);
            }

            // Free non-contiguous blocks to create fragmentation
            allocator.Deallocate(blocks[1]);
            allocator.Deallocate(blocks[4]);
            allocator.Deallocate(blocks[7]);

            // First fit should use the first available block (previously blocks[1])
            void* new_block = allocator.Allocate(alloc_size);
            DS_EXPECT(new_block != nullptr);

            // Clean up
            allocator.Deallocate(new_block);
            for (ds_i32 i = 0; i < 10; i++)
            {
                if (i != 1 && i != 4 && i != 7) // Skip already freed blocks
                {
                    allocator.Deallocate(blocks[i]);
                }
            }
        }

        // Test Best Fit strategy
        {
            Free_List_Allocator allocator(allocator_size, Free_List_Allocator::Allocation_Strategy::FIND_BEST, "BestFitFreeList");
            DS_EXPECT(allocator.Get_Strategy() == Free_List_Allocator::Allocation_Strategy::FIND_BEST);

            // Allocate blocks of varying sizes
            void* block1 = allocator.Allocate(1024);
            void* block2 = allocator.Allocate(2048);
            void* block3 = allocator.Allocate(4096);
            void* block4 = allocator.Allocate(8192);

            DS_EXPECT(block1 != nullptr);
            DS_EXPECT(block2 != nullptr);
            DS_EXPECT(block3 != nullptr);
            DS_EXPECT(block4 != nullptr);

            // Free blocks to create free space of different sizes
            allocator.Deallocate(block1); // 1024 bytes free
            allocator.Deallocate(block3); // 4096 bytes free

            // Allocate with best fit, which should use the smallest block that fits
            void* small_block = allocator.Allocate(1000); // Should use the 1024 block (previously block1)
            DS_EXPECT(small_block != nullptr);

            // Clean up
            allocator.Deallocate(small_block);
            allocator.Deallocate(block2);
            allocator.Deallocate(block4);
        }

        // Test Next Fit strategy
        {
            Free_List_Allocator allocator(allocator_size, Free_List_Allocator::Allocation_Strategy::FIND_NEXT, "NextFitFreeList");
            DS_EXPECT(allocator.Get_Strategy() == Free_List_Allocator::Allocation_Strategy::FIND_NEXT);

            // Allocate several blocks
            void* blocks[10];
            for (ds_i32 i = 0; i < 10; i++)
            {
                blocks[i] = allocator.Allocate(alloc_size);
                DS_EXPECT(blocks[i] != nullptr);
            }

            // Free non-contiguous blocks
            allocator.Deallocate(blocks[3]);
            allocator.Deallocate(blocks[6]);
            allocator.Deallocate(blocks[9]);

            // Next fit should start searching from the last allocated block
            // and wrap around if necessary

            // Allocate new blocks - the exact behavior depends on the implementation,
            // but we can verify they're allocated
            void* new_block1 = allocator.Allocate(alloc_size);
            void* new_block2 = allocator.Allocate(alloc_size);
            void* new_block3 = allocator.Allocate(alloc_size);

            DS_EXPECT(new_block1 != nullptr);
            DS_EXPECT(new_block2 != nullptr);
            DS_EXPECT(new_block3 != nullptr);

            // Clean up
            allocator.Deallocate(new_block1);
            allocator.Deallocate(new_block2);
            allocator.Deallocate(new_block3);

            for (ds_i32 i = 0; i < 10; i++)
            {
                if (i != 3 && i != 6 && i != 9) // Skip already freed blocks
                {
                    allocator.Deallocate(blocks[i]);
                }
            }
        }

        // Test changing strategy
        {
            Free_List_Allocator allocator(allocator_size, Free_List_Allocator::Allocation_Strategy::FIND_FIRST, "ChangingStrategyFreeList");
            DS_EXPECT(allocator.Get_Strategy() == Free_List_Allocator::Allocation_Strategy::FIND_FIRST);

            // Change to best fit
            allocator.Set_Strategy(Free_List_Allocator::Allocation_Strategy::FIND_BEST);
            DS_EXPECT(allocator.Get_Strategy() == Free_List_Allocator::Allocation_Strategy::FIND_BEST);

            // Change to next fit
            allocator.Set_Strategy(Free_List_Allocator::Allocation_Strategy::FIND_NEXT);
            DS_EXPECT(allocator.Get_Strategy() == Free_List_Allocator::Allocation_Strategy::FIND_NEXT);

            // Allocate and free to ensure it works after strategy changes
            void* block = allocator.Allocate(alloc_size);
            DS_EXPECT(block != nullptr);
            DS_EXPECT(allocator.Deallocate(block));
        }

        return true;
    }

    // Test allocation alignment
    static ds_bool Test_Free_List_Alignment()
    {
        Free_List_Allocator allocator(1024 * 1024, Free_List_Allocator::Allocation_Strategy::FIND_FIRST, "AlignmentFreeList");

        // Test different alignments
        const ds_u64 size = 128;
        const ds_u64 alignments[] = { 4, 8, 16, 32, 64, 128, 256 };

        for (auto alignment : alignments)
        {
            void* ptr = allocator.Allocate(size, alignment);
            DS_EXPECT(ptr != nullptr);

            // Verify alignment
            ds_uiptr address = reinterpret_cast<ds_uiptr>(ptr);
            DS_EXPECT((address % alignment) == 0);

            // Verify we can write to memory
            std::memset(ptr, 0xAB, size);

            // Free for next iteration
            allocator.Deallocate(ptr);
        }

        return true;
    }

    // Test object creation and destruction
    static ds_bool Test_Free_List_Object_Creation()
    {
        Free_List_Allocator allocator(1024 * 1024, Free_List_Allocator::Allocation_Strategy::FIND_BEST, "ObjectFreeList");

        // Reset destruction counter
        Test_Object::destruction_count = 0;

        // Create a single object
        Test_Object* obj = allocator.Create<Test_Object>(1, 2, 3, 4.0f, 'A');
        DS_EXPECT(obj != nullptr);
        DS_EXPECT(obj->x == 1);
        DS_EXPECT(obj->y == 2);
        DS_EXPECT(obj->z == 3);
        DS_EXPECT(obj->f == 4.0f);
        DS_EXPECT(obj->c == 'A');

        // Create a second object
        Test_Object* obj2 = allocator.Create<Test_Object>(5, 6, 7, 8.0f, 'B');
        DS_EXPECT(obj2 != nullptr);
        DS_EXPECT(obj2->x == 5);
        DS_EXPECT(obj2->y == 6);
        DS_EXPECT(obj2->z == 7);
        DS_EXPECT(obj2->f == 8.0f);
        DS_EXPECT(obj2->c == 'B');

        // Destroy objects
        DS_EXPECT(allocator.Destroy(obj));
        DS_EXPECT(allocator.Destroy(obj2));

        // Verify destructors were called
        DS_EXPECT(Test_Object::destruction_count == 2);

        return true;
    }

    // Test memory fragmentation and coalescing
    static ds_bool Test_Free_List_Fragmentation()
    {
        Free_List_Allocator allocator(1024 * 1024, Free_List_Allocator::Allocation_Strategy::FIND_BEST, "FragmentationFreeList");

        // Create allocation pattern that will lead to fragmentation
        void* small_blocks[100];
        void* medium_blocks[10];
        void* large_block;

        // Allocate small blocks
        for (ds_i32 i = 0; i < 100; i++)
        {
            small_blocks[i] = allocator.Allocate(128);
            DS_EXPECT(small_blocks[i] != nullptr);
        }

        // Allocate medium blocks
        for (ds_i32 i = 0; i < 10; i++)
        {
            medium_blocks[i] = allocator.Allocate(1024);
            DS_EXPECT(medium_blocks[i] != nullptr);
        }

        // Save initial state
        ds_u64 initial_free_blocks = allocator.Get_Free_Block_Count();
        ds_u64 initial_free_size = allocator.Get_Free_Size();

        // Free blocks to create fragmentation, with some adjacent
        for (ds_i32 i = 0; i < 100; i += 3)
        {
            DS_EXPECT(allocator.Deallocate(small_blocks[i]));
        }

        // Record state after initial freeing
        ds_u64 after_free_blocks = allocator.Get_Free_Block_Count();
        ds_u64 after_free_size = allocator.Get_Free_Size();

        // Verify that some coalescing may have occurred automatically
        DS_LOG_INFO("Free blocks after deallocation: {0}", after_free_blocks);
        DS_LOG_INFO("Free size after deallocation: {0}", after_free_size);

        // Additional freeing to potentially create more adjacent blocks
        for (ds_i32 i = 1; i < 100; i += 3)
        {
            DS_EXPECT(allocator.Deallocate(small_blocks[i]));
        }

        // Record state after more freeing
        ds_u64 after_more_free_blocks = allocator.Get_Free_Block_Count();
        ds_u64 after_more_free_size = allocator.Get_Free_Size();

        // Run defragmentation
        ds_u64 coalesced = allocator.Defragment();

        // Verify system state after defragmentation
        DS_LOG_INFO("Blocks coalesced by explicit defragmentation: {0}", coalesced);
        DS_LOG_INFO("Free blocks after defragmentation: {0}", allocator.Get_Free_Block_Count());

        // If coalescing is working correctly (either in Deallocate or Defragment),
        // we should have fewer free blocks than the maximum possible
        DS_EXPECT(allocator.Get_Free_Block_Count() < 100 / 3 + 100 / 3);

        // Allocate a large block that should fit in the combined free space
        large_block = allocator.Allocate(4096);
        DS_EXPECT(large_block != nullptr);

        // Free remaining allocations
        DS_EXPECT(allocator.Deallocate(large_block));

        for (ds_i32 i = 0; i < 100; i++)
        {
            if (i % 3 >= 2) // Free the blocks we haven't freed yet
            {
                DS_EXPECT(allocator.Deallocate(small_blocks[i]));
            }
        }

        for (ds_i32 i = 0; i < 10; i++)
        {
            DS_EXPECT(allocator.Deallocate(medium_blocks[i]));
        }

        // Verify we're back to a single free block (everything coalesced)
        DS_EXPECT(allocator.Get_Free_Block_Count() == 1);
        DS_EXPECT(allocator.Get_Free_Size() == allocator.Get_Size());

        return true;
    }

    // Test reset functionality
    static ds_bool Test_Free_List_Reset()
    {
        Free_List_Allocator allocator(1024 * 1024, Free_List_Allocator::Allocation_Strategy::FIND_FIRST, "ResetFreeList");

        // Allocate several blocks to fragment the memory
        void* blocks[10];
        for (ds_i32 i = 0; i < 10; i++)
        {
            blocks[i] = allocator.Allocate(1024);
            DS_EXPECT(blocks[i] != nullptr);
        }

        // Free some blocks non-contiguously
        allocator.Deallocate(blocks[2]);
        allocator.Deallocate(blocks[5]);
        allocator.Deallocate(blocks[8]);

        // Verify fragmentation
        DS_EXPECT(allocator.Get_Free_Block_Count() > 1);

        // Reset the allocator
        allocator.Reset();

        // Verify we're back to a clean state
        DS_EXPECT(allocator.Get_Free_Block_Count() == 1);
        DS_EXPECT(allocator.Get_Free_Size() == allocator.Get_Size());
        DS_EXPECT(allocator.Get_Used_Size() == 0);

        // Allocate after reset
        void* new_block = allocator.Allocate(1024);
        DS_EXPECT(new_block != nullptr);
        DS_EXPECT(allocator.Deallocate(new_block));

        return true;
    }

    // Test largest free block size
    static ds_bool Test_Free_List_Largest_Free_Block()
    {
        Free_List_Allocator allocator(1024 * 1024, Free_List_Allocator::Allocation_Strategy::FIND_BEST, "LargestBlockFreeList");

        // Initially the largest free block should be the entire allocator
        DS_EXPECT(allocator.Get_Largest_Free_Block_Size() == allocator.Get_Size() - sizeof(Free_List_Allocator::Block_Header));

        // Allocate a large block
        const ds_u64 large_size = 512 * 1024; // 512KB
        void* large_block = allocator.Allocate(large_size);
        DS_EXPECT(large_block != nullptr);

        // Now the largest free block should be smaller
        DS_EXPECT(allocator.Get_Largest_Free_Block_Size() < allocator.Get_Size() - sizeof(Free_List_Allocator::Block_Header));

        // Free the block
        DS_EXPECT(allocator.Deallocate(large_block));

        // Largest free block should be back to original size (minus some overhead)
        DS_EXPECT(allocator.Get_Largest_Free_Block_Size() >= allocator.Get_Size() - sizeof(Free_List_Allocator::Block_Header) - 64);

        return true;
    }

    // Test move operations
    static ds_bool Test_Free_List_Move_Operations()
    {
        // Create source allocator
        Free_List_Allocator allocator1(1024 * 1024, Free_List_Allocator::Allocation_Strategy::FIND_FIRST, "SourceFreeList");

        // Allocate some memory
        void* block = allocator1.Allocate(1024);
        DS_EXPECT(block != nullptr);

        // Move-construct a new allocator
        Free_List_Allocator allocator2(std::move(allocator1));

        // Verify the new allocator has the memory
        DS_EXPECT(allocator2.Get_Size() == 1024 * 1024);
        DS_EXPECT(allocator2.Get_Used_Size() > 0);

        // First allocator should be empty
        DS_EXPECT(allocator1.Get_Size() == 0);
        DS_EXPECT(allocator1.Get_Used_Size() == 0);

        // Should be able to deallocate through the new allocator
        DS_EXPECT(allocator2.Deallocate(block));

        // Create another allocator for move assignment
        Free_List_Allocator allocator3(512 * 1024, Free_List_Allocator::Allocation_Strategy::FIND_NEXT, "DestFreeList");
        void* block3 = allocator3.Allocate(1024);
        DS_EXPECT(block3 != nullptr);

        // Move-assign
        allocator3 = std::move(allocator2);

        // Verify new state
        DS_EXPECT(allocator3.Get_Size() == 1024 * 1024);
        DS_EXPECT(allocator3.Get_Used_Size() == 0);
        DS_EXPECT(allocator3.Get_Strategy() == Free_List_Allocator::Allocation_Strategy::FIND_FIRST);

        // Second allocator should be empty
        DS_EXPECT(allocator2.Get_Size() == 0);

        return true;
    }

    // Test stress case with many allocations
    static ds_bool Test_Free_List_Stress()
    {
        Free_List_Allocator allocator(10 * 1024 * 1024, // 10MB
            Free_List_Allocator::Allocation_Strategy::FIND_BEST, "StressFreeList");

        const ds_i32 allocation_count = 1000;
        void* allocations[allocation_count];

        // Perform many allocations of various sizes
        for (ds_i32 i = 0; i < allocation_count; i++)
        {
            // Random size between 32 and 2048 bytes
            ds_u64 size = 32 + (rand() % 2016);
            allocations[i] = allocator.Allocate(size);
            DS_EXPECT(allocations[i] != nullptr);

            // Write a pattern to verify memory is usable
            std::memset(allocations[i], (ds_u8)i % 255, size);
        }

        // Free in random order
        for (ds_i32 i = 0; i < allocation_count; i++)
        {
            ds_i32 index = rand() % allocation_count;
            if (allocations[index] != nullptr)
            {
                DS_EXPECT(allocator.Deallocate(allocations[index]));
                allocations[index] = nullptr;
            }
        }

        // Verify all memory is freed
        for (ds_i32 i = 0; i < allocation_count; i++)
        {
            if (allocations[i] != nullptr)
            {
                DS_EXPECT(allocator.Deallocate(allocations[i]));
                allocations[i] = nullptr;
            }
        }

        // Verify the allocator is in a clean state
        DS_EXPECT(allocator.Get_Used_Size() == 0);

        return true;
    }

    // Add all tests to the test suite
    static ds_bool Add_All_Tests(Test_Suite& test_suite)
    {
        DS_TEST(test_suite, "Basic Free List Operations")
        {
            return Test_Free_List_Basic();
        });

        DS_TEST(test_suite, "Free List Allocation Strategies")
        {
            return Test_Free_List_Strategies();
        });

        DS_TEST(test_suite, "Free List Alignment")
        {
            return Test_Free_List_Alignment();
        });

        DS_TEST(test_suite, "Free List Object Creation")
        {
            return Test_Free_List_Object_Creation();
        });

        DS_TEST(test_suite, "Free List Fragmentation and Coalescing")
        {
            return Test_Free_List_Fragmentation();
        });

        DS_TEST(test_suite, "Free List Reset")
        {
            return Test_Free_List_Reset();
        });

        DS_TEST(test_suite, "Free List Largest Free Block")
        {
            return Test_Free_List_Largest_Free_Block();
        });

        DS_TEST(test_suite, "Free List Move Operations")
        {
            return Test_Free_List_Move_Operations();
        });

        DS_TEST(test_suite, "Free List Stress Test")
        {
            return Test_Free_List_Stress();
        });

        return true;
    }
}