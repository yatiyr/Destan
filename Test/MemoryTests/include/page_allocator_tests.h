#pragma once
#include <core/destan_pch.h>
#include <core/memory/page_allocator.h>
#include <test_framework.h>

using namespace destan::core::memory;
using namespace destan::test;

namespace destan::test::page_allocator
{
    // Test struct for object creation and destruction
    struct Test_Object
    {
        destan_i32 x, y, z;
        destan_f32 f;
        destan_char c;
        static destan_i32 destruction_count;

        Test_Object() : x(0), y(0), z(0), f(0.0f), c(0) {}

        Test_Object(destan_i32 x_val, destan_i32 y_val, destan_i32 z_val, destan_f32 f_val, destan_char c_val)
            : x(x_val), y(y_val), z(z_val), f(f_val), c(c_val) {
        }

        ~Test_Object()
        {
            // Increment destruction counter
            destruction_count++;
        }
    };

    // Initialize static counter
    destan_i32 Test_Object::destruction_count = 0;

    // Test basic page allocator functionality
    static destan_bool Test_Page_Basic()
    {
        // Create a page allocator with default page size
        Page_Allocator allocator(0, 0, "TestPageAllocator");

        // Get system page size for verification
        destan_u64 system_page_size = Page_Allocator::Get_System_Page_Size();
        DESTAN_EXPECT(system_page_size > 0);
        DESTAN_EXPECT(allocator.Get_Page_Size() == system_page_size);

        // Allocate a single page with COMMIT flag to ensure we can write to it
        void* page = allocator.Allocate(system_page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        DESTAN_EXPECT(page != nullptr);

        // Verify we can write to the page
        Memory::Memset(page, 0xAB, system_page_size);

        // Get page info
        const Page_Allocator::Page_Info* info = allocator.Get_Page_Info(page);
        DESTAN_EXPECT(info != nullptr);
        DESTAN_EXPECT(info->base_address == page);
        DESTAN_EXPECT(info->size >= system_page_size);
        DESTAN_EXPECT(info->page_count > 0);

        // Allocate another page
        void* page2 = allocator.Allocate(system_page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        DESTAN_EXPECT(page2 != nullptr);
        DESTAN_EXPECT(page2 != page);

        // Deallocate pages
        DESTAN_EXPECT(allocator.Deallocate(page));
        DESTAN_EXPECT(allocator.Deallocate(page2));

        // Verify allocation count is back to zero
        DESTAN_EXPECT(allocator.Get_Allocated_Page_Count() == 0);

        return true;
    }

    // Test different protection modes
    static destan_bool Test_Page_Protection_Modes()
    {
        Page_Allocator allocator(0, 0, "ProtectionPageAllocator");
        destan_u64 page_size = allocator.Get_Page_Size();

        // Test read-only pages - Need to include ZERO flag to avoid access violation during initialization
        void* read_only_page = allocator.Allocate(page_size, Page_Protection::READ_ONLY, Page_Flags::COMMIT | Page_Flags::ZERO);
        DESTAN_EXPECT(read_only_page != nullptr);

        // We should be able to read from read-only pages
        destan_u8 value = *static_cast<destan_u8*>(read_only_page);
        (void)value; // Avoid unused variable warning

        // Test read-write pages
        void* read_write_page = allocator.Allocate(page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        DESTAN_EXPECT(read_write_page != nullptr);

        // We should be able to write to read-write pages
        Memory::Memset(read_write_page, 0xCD, page_size);

        // Change protection of read-only page to read-write
        DESTAN_EXPECT(allocator.Protect(read_only_page, Page_Protection::READ_WRITE));

        // Now we should be able to write to the former read-only page
        Memory::Memset(read_only_page, 0xEF, page_size);

        // Change back to read-only
        DESTAN_EXPECT(allocator.Protect(read_only_page, Page_Protection::READ_ONLY));

        // Clean up
        DESTAN_EXPECT(allocator.Deallocate(read_only_page));
        DESTAN_EXPECT(allocator.Deallocate(read_write_page));

        // Test executable pages (on platforms that support it)
        void* exec_page = allocator.Allocate(page_size, Page_Protection::READ_WRITE_EXEC, Page_Flags::COMMIT);
        if (exec_page)
        {
            // We should be able to write to executable pages
            Memory::Memset(exec_page, 0x90, page_size); // 0x90 is x86 NOP instruction
            DESTAN_EXPECT(allocator.Deallocate(exec_page));
        }
        else
        {
            DESTAN_LOG_WARN("Executable pages not supported or restricted on this platform");
        }

        return true;
    }

    // Test page allocation flags
    static destan_bool Test_Page_Allocation_Flags()
    {
        Page_Allocator allocator(0, 0, "FlagsPageAllocator");
        destan_u64 page_size = allocator.Get_Page_Size();

        // Test committed pages
        void* committed_page = allocator.Allocate(page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        DESTAN_EXPECT(committed_page != nullptr);

        // Write to committed page to ensure it's usable
        Memory::Memset(committed_page, 0xAB, page_size);

        // Test zeroed pages
        void* zeroed_page = allocator.Allocate(page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT | Page_Flags::ZERO);
        DESTAN_EXPECT(zeroed_page != nullptr);

        // Verify page is zeroed
        destan_u8* bytes = static_cast<destan_u8*>(zeroed_page);
        for (destan_u64 i = 0; i < 100; i++) // Check first 100 bytes
        {
            DESTAN_EXPECT(bytes[i] == 0);
        }

        // Test decommit/commit operations
        DESTAN_EXPECT(allocator.Decommit(committed_page, page_size));
        DESTAN_EXPECT(allocator.Commit(committed_page, page_size));

        // Ensure page is still usable after recommit
        Memory::Memset(committed_page, 0xCD, page_size);

        // Clean up
        DESTAN_EXPECT(allocator.Deallocate(committed_page));
        DESTAN_EXPECT(allocator.Deallocate(zeroed_page));

        // Test other flags (some may not be supported on all platforms)
        Page_Flags flags = Page_Flags::COMMIT | Page_Flags::ZERO;

        // Large pages might not be supported, so handle potential failure
        void* large_page = allocator.Allocate(page_size * 10, Page_Protection::READ_WRITE,
            flags | Page_Flags::LARGE_PAGES);
        if (large_page)
        {
            DESTAN_LOG_INFO("Large pages supported on this platform");
            DESTAN_EXPECT(allocator.Deallocate(large_page));
        }
        else
        {
            DESTAN_LOG_WARN("Large pages not supported on this platform");
        }

        return true;
    }

    // Test object creation and destruction
    static destan_bool Test_Page_Object_Creation()
    {
        Page_Allocator allocator(0, 0, "ObjectPageAllocator");

        // Reset destruction counter
        Test_Object::destruction_count = 0;

        // Create an object
        Test_Object* obj = allocator.Create<Test_Object>(Page_Protection::READ_WRITE, Page_Flags::COMMIT | Page_Flags::ZERO,
            1, 2, 3, 4.0f, 'A');
        DESTAN_EXPECT(obj != nullptr);

        // Verify object state
        DESTAN_EXPECT(obj->x == 1);
        DESTAN_EXPECT(obj->y == 2);
        DESTAN_EXPECT(obj->z == 3);
        DESTAN_EXPECT(obj->f == 4.0f);
        DESTAN_EXPECT(obj->c == 'A');

        // Destroy the object
        DESTAN_EXPECT(allocator.Destroy(obj));

        // Verify destructor was called
        DESTAN_EXPECT(Test_Object::destruction_count == 1);

        return true;
    }

    // Test multi-page allocations
    static destan_bool Test_Page_Multi_Page_Allocation()
    {
        Page_Allocator allocator(0, 0, "MultiPageAllocator");
        destan_u64 page_size = allocator.Get_Page_Size();

        // Allocate multiple pages with COMMIT flag to ensure we can write to them
        destan_u64 multi_page_size = page_size * 5;
        void* multi_page = allocator.Allocate(multi_page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        DESTAN_EXPECT(multi_page != nullptr);

        // Verify the allocation info
        const Page_Allocator::Page_Info* info = allocator.Get_Page_Info(multi_page);
        DESTAN_EXPECT(info != nullptr);
        DESTAN_EXPECT(info->size >= multi_page_size);
        DESTAN_EXPECT(info->page_count >= 5);

        // Write to the entire allocation to ensure it's usable
        Memory::Memset(multi_page, 0xAB, multi_page_size);

        // Deallocate
        DESTAN_EXPECT(allocator.Deallocate(multi_page));

        return true;
    }

    // Test reserved address space
    static destan_bool Test_Page_Reserved_Address_Space()
    {
        // Create allocator with reserved address space
        destan_u64 reserved_size = 10 * 1024 * 1024; // 10 MB
        Page_Allocator allocator(0, reserved_size, "ReservedPageAllocator");
        destan_u64 page_size = allocator.Get_Page_Size();

        // Perform multiple allocations from the reserved space
        std::vector<void*> pages;
        for (destan_i32 i = 0; i < 10; i++)
        {
            // Use COMMIT flag to ensure we can write to the pages
            void* page = allocator.Allocate(page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
            DESTAN_EXPECT(page != nullptr);
            pages.push_back(page);

            // Write to the page
            Memory::Memset(page, static_cast<destan_i32>(i), page_size);
        }

        // Deallocate in reverse order
        for (auto it = pages.rbegin(); it != pages.rend(); ++it)
        {
            DESTAN_EXPECT(allocator.Deallocate(*it));
        }

        return true;
    }

    // Test validation of pointers
    static destan_bool Test_Page_Pointer_Validation()
    {
        Page_Allocator allocator(0, 0, "ValidationPageAllocator");
        destan_u64 page_size = allocator.Get_Page_Size();

        // Allocate a page with COMMIT flag
        void* page = allocator.Allocate(page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        DESTAN_EXPECT(page != nullptr);

        // Test Is_Allocated with valid pointer
        DESTAN_EXPECT(allocator.Is_Allocated(page));

        // Test with pointer inside the allocated page
        destan_u8* middle_ptr = static_cast<destan_u8*>(page) + (page_size / 2);
        DESTAN_EXPECT(allocator.Is_Allocated(middle_ptr));

        // Test with invalid pointer
        destan_i32 stack_var = 0;
        DESTAN_EXPECT(!allocator.Is_Allocated(&stack_var));

        // Test with null pointer
        DESTAN_EXPECT(!allocator.Is_Allocated(nullptr));

        // Test with out-of-bounds pointer
        destan_u8* out_of_bounds = static_cast<destan_u8*>(page) + page_size + 100;
        DESTAN_EXPECT(!allocator.Is_Allocated(out_of_bounds));

        // Clean up
        DESTAN_EXPECT(allocator.Deallocate(page));

        // Test with deallocated pointer
        DESTAN_EXPECT(!allocator.Is_Allocated(page));

        return true;
    }

    // Test move operations
    static destan_bool Test_Page_Move_Operations()
    {
        // Create source allocator
        Page_Allocator allocator1(0, 0, "SourcePageAllocator");
        destan_u64 page_size = allocator1.Get_Page_Size();

        // Allocate a page with COMMIT flag
        void* page = allocator1.Allocate(page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        DESTAN_EXPECT(page != nullptr);

        // Move construct to a new allocator
        Page_Allocator allocator2(std::move(allocator1));

        // Verify allocator2 took ownership
        DESTAN_EXPECT(allocator2.Get_Allocated_Page_Count() == 1);
        DESTAN_EXPECT(allocator2.Is_Allocated(page));

        // allocator1 should be empty after move
        DESTAN_EXPECT(allocator1.Get_Allocated_Page_Count() == 0);

        // Create another allocator
        Page_Allocator allocator3(0, 0, "TargetPageAllocator");
        void* page3 = allocator3.Allocate(page_size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        DESTAN_EXPECT(page3 != nullptr);

        // Move assign
        allocator3 = std::move(allocator2);

        // Verify allocator3 now has allocator2's resources
        DESTAN_EXPECT(allocator3.Get_Allocated_Page_Count() == 1);
        DESTAN_EXPECT(allocator3.Is_Allocated(page));

        // allocator2 should be empty
        DESTAN_EXPECT(allocator2.Get_Allocated_Page_Count() == 0);

        // Clean up
        DESTAN_EXPECT(allocator3.Deallocate(page));

        return true;
    }

    // Test multiple allocators operating simultaneously
    static destan_bool Test_Page_Multiple_Allocators()
    {
        // Create allocators with different page sizes
        Page_Allocator allocator1(4096, 0, "PageAllocator1");
        Page_Allocator allocator2(8192, 0, "PageAllocator2");

        // Verify different page sizes
        DESTAN_EXPECT(allocator1.Get_Page_Size() == 4096);
        DESTAN_EXPECT(allocator2.Get_Page_Size() == 8192);

        // Allocate from both with COMMIT flag
        void* page1 = allocator1.Allocate(4096, Page_Protection::READ_WRITE, Page_Flags::COMMIT);
        void* page2 = allocator2.Allocate(8192, Page_Protection::READ_WRITE, Page_Flags::COMMIT);

        DESTAN_EXPECT(page1 != nullptr);
        DESTAN_EXPECT(page2 != nullptr);

        // Verify each allocator only knows about its own pages
        DESTAN_EXPECT(allocator1.Is_Allocated(page1));
        DESTAN_EXPECT(!allocator1.Is_Allocated(page2));

        DESTAN_EXPECT(allocator2.Is_Allocated(page2));
        DESTAN_EXPECT(!allocator2.Is_Allocated(page1));

        // Clean up
        DESTAN_EXPECT(allocator1.Deallocate(page1));
        DESTAN_EXPECT(allocator2.Deallocate(page2));

        return true;
    }

    // Test stress with many allocations
    static destan_bool Test_Page_Stress()
    {
        Page_Allocator allocator(4096, 10 * 1024 * 1024, "StressPageAllocator"); // 10MB reserved
        destan_u64 page_size = allocator.Get_Page_Size();

        // Perform many small allocations
        const destan_i32 allocation_count = 100;
        std::vector<void*> pages;

        for (destan_i32 i = 0; i < allocation_count; i++)
        {
            // Randomize between single and multi-page allocations
            destan_u64 size = page_size * (1 + (rand() % 3));

            // Always use COMMIT flag to ensure we can write to the pages
            void* page = allocator.Allocate(size, Page_Protection::READ_WRITE, Page_Flags::COMMIT);

            if (!page)
            {
                DESTAN_LOG_WARN("Failed to allocate page at iteration {0}", i);
                break;
            }

            pages.push_back(page);

            // Write to the page with a pattern
            Memory::Memset(page, static_cast<destan_i32>(i % 256), size);
        }

        // Verify we have allocations
        DESTAN_EXPECT(!pages.empty());

        // Free in random order
        while (!pages.empty())
        {
            destan_i32 index = rand() % pages.size();
            DESTAN_EXPECT(allocator.Deallocate(pages[index]));

            // Remove from vector
            pages[index] = pages.back();
            pages.pop_back();
        }

        // Verify all pages were freed
        DESTAN_EXPECT(allocator.Get_Allocated_Page_Count() == 0);

        return true;
    }

    // Add all tests to the test suite
    static destan_bool Add_All_Tests(Test_Suite& test_suite)
    {
        DESTAN_TEST(test_suite, "Basic Page Operations")
        {
            return Test_Page_Basic();
        });

        DESTAN_TEST(test_suite, "Page Protection Modes")
        {
            return Test_Page_Protection_Modes();
        });

        DESTAN_TEST(test_suite, "Page Allocation Flags")
        {
            return Test_Page_Allocation_Flags();
        });

        DESTAN_TEST(test_suite, "Page Object Creation")
        {
            return Test_Page_Object_Creation();
        });

        DESTAN_TEST(test_suite, "Multi-Page Allocation")
        {
            return Test_Page_Multi_Page_Allocation();
        });

        DESTAN_TEST(test_suite, "Reserved Address Space")
        {
            return Test_Page_Reserved_Address_Space();
        });

        DESTAN_TEST(test_suite, "Page Pointer Validation")
        {
            return Test_Page_Pointer_Validation();
        });

        DESTAN_TEST(test_suite, "Page Move Operations")
        {
            return Test_Page_Move_Operations();
        });

        DESTAN_TEST(test_suite, "Multiple Page Allocators")
        {
            return Test_Page_Multiple_Allocators();
        });

        DESTAN_TEST(test_suite, "Page Allocator Stress Test")
        {
            return Test_Page_Stress();
        });

        return true;
    }
}