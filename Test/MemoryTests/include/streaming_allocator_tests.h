#pragma once
#include <core/ds_pch.h>
#include <core/memory/streaming_allocator.h>
#include <test_framework.h>

using namespace ds::core::memory;
using namespace ds::test;

namespace ds::test::streaming_allocator
{

    // Create test files for streaming allocator tests
    void Create_Test_Files()
    {
        // Create directories if needed
        std::filesystem::create_directories("test_data");
        std::filesystem::path x = std::filesystem::current_path();
        // Define test files with specific sizes
        struct TestFile {
            const char* path;
            size_t size;
        };

        const TestFile test_files[] = {
            {"test_data/test_resource.bin", 256 * 1024},      // 256KB
            {"test_data/low.bin", 128 * 1024},                // 128KB
            {"test_data/medium.bin", 256 * 1024},             // 256KB
            {"test_data/high.bin", 384 * 1024},               // 384KB
            {"test_data/critical.bin", 512 * 1024},           // 512KB
            {"test_data/callback_resource.bin", 256 * 1024},  // 256KB
            {"test_data/prefetch_resource.bin", 128 * 1024},  // 128KB
            {"test_data/auto_unload_resource.bin", 256 * 1024}, // 256KB
            {"test_data/move_resource.bin", 192 * 1024},      // 192KB
            {"test_data/move_resource2.bin", 192 * 1024},     // 192KB
            {"test_data/move_resource3.bin", 192 * 1024},     // 192KB
            {"test_data/stats_resource.bin", 64 * 1024}       // 64KB
            // mapped_file.bin is created in the memory-mapped test
        };

        // Create each file with appropriate content
        for (const auto& file : test_files)
        {
            std::ofstream ofs(file.path, std::ios::binary);
            if (ofs)
            {
                // Create a buffer of the desired size
                std::vector<char> buffer(file.size);

                // Fill the buffer with a deterministic pattern based on the file path
                // This helps verify that we're loading the correct data
                for (size_t i = 0; i < file.size; i++)
                {
                    // Use a simple hash of the file path to create a unique pattern
                    size_t hash_value = 0;
                    const char* p = file.path;
                    while (*p) {
                        hash_value = (hash_value * 31) + *p++;
                    }

                    buffer[i] = static_cast<char>((i + hash_value) % 256);
                }

                // Write the buffer to the file
                ofs.write(buffer.data(), file.size);

                // Ensure the file is properly closed
                ofs.close();

                DS_LOG_INFO("Created test file: {0} ({1} KB)", file.path, file.size / 1024);
            }
            else
            {
                DS_LOG_ERROR("Failed to create test file: {0}", file.path);
            }
        }
    }

    // Clean up test files after tests complete
    static void Delete_Test_Files()
    {
        // Delete the test_data directory and all its contents
        std::error_code ec;
        if (std::filesystem::exists("test_data"))
        {
            size_t removed = std::filesystem::remove_all("test_data", ec);

            if (ec)
            {
                DS_LOG_ERROR("Error removing test files: {0}", ec.message());
            }
            else
            {
                DS_LOG_INFO("Removed {0} test files/directories", removed);
            }
        }
    }

    // Callback function for resource loading
    void Resource_Loaded_Callback_Func(ds_u64 resource_id, void* data, ds_u64 size, void* user_data)
    {
        // Cast user_data to bool* and set it to true to indicate callback was called
        if (user_data)
        {
            bool* callback_called = static_cast<bool*>(user_data);
            *callback_called = true;
        }
    }

    // Test basic streaming allocator functionality
    static ds_bool Test_Streaming_Basic()
    {
        // Create a configuration with a small budget for testing
        Streaming_Allocator::Config config;
        config.total_memory_budget = 16 * 1024 * 1024; // 16MB
        config.page_size = 64 * 1024; // 64KB pages
        config.max_concurrent_operations = 4;
        config.cache_seconds = 5; // Short cache time for testing

        // Create the allocator on the heap instead of the stack
        std::unique_ptr<Streaming_Allocator> allocator =
            std::make_unique<Streaming_Allocator>(config, "TestStreamingAllocator");

        // Verify initial state
        auto stats = allocator->Get_Stats();
        DS_EXPECT(stats.total_memory_budget == config.total_memory_budget);
        DS_EXPECT(stats.total_memory_used == 0);
        DS_EXPECT(stats.resource_count == 0);

        // Create a resource request
        Streaming_Allocator::Resource_Request request;
        request.resource_id = 0; // Generate new ID
        request.path = "test_data/test_resource.bin";
        request.category = Streaming_Allocator::Resource_Category::GENERIC;
        request.priority = Resource_Priority::MEDIUM;
        request.access_mode = Streaming_Allocator::Access_Mode::READ_ONLY;
        request.callback = nullptr;
        request.user_data = nullptr;
        request.auto_unload = true;
        request.estimated_size = 256 * 1024; // 256KB

        // Request the resource
        Resource_Handle handle = allocator->Request_Resource(request);

        // Verify handle is valid
        DS_EXPECT(handle.IsValid());
        DS_EXPECT(handle.id != 0);

        // Initially, the resource may be in LOADING state
        DS_EXPECT(handle.state == Resource_State::LOADING);

        // Update the streaming system to process pending operations
        allocator->Update(0.016f); // 16ms frame time

        // The resource should now be in RESIDENT state
        const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);

        // Resource might still be LOADING if the async operation hasn't completed,
        // or it might be RESIDENT if it completed immediately
        DS_EXPECT(info->state == Resource_State::RESIDENT || info->state == Resource_State::LOADING);

        // If it's RESIDENT, we should be able to access it
        if (info->state == Resource_State::RESIDENT)
        {
            void* data = allocator->Access_Resource(handle);
            DS_EXPECT(data != nullptr);
        }

        // Reference the resource
        DS_EXPECT(allocator->Reference_Resource(handle));

        // After referencing, it should have a reference count of 1
        info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->reference_count == 1);

        // Release the resource
        DS_EXPECT(allocator->Release_Resource(handle));

        // After releasing, reference count should be 0
        info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->reference_count == 0);

        // Try to unload the resource
        DS_EXPECT(allocator->Unload_Resource(handle));

        // Update the streaming system to process the unload
        allocator->Update(0.016f);

        // Verify resource is unloaded or unloading
        info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->state == Resource_State::UNLOADING || info->state == Resource_State::UNLOADED);

        return true;
    }

    // Test resource priorities
    static ds_bool Test_Streaming_Priorities()
    {
        Streaming_Allocator::Config config;
        config.total_memory_budget = 512 * 1024 * 1024; // 512MB
        std::unique_ptr<Streaming_Allocator> allocator = std::make_unique<Streaming_Allocator>(config, "PriorityStreamingAllocator");

        // Create resources with different priorities
        const char* paths[] = { "test_data/low.bin", "test_data/medium.bin", "test_data/high.bin", "test_data/critical.bin" };
        Resource_Priority priorities[] = {
            Resource_Priority::LOW,
            Resource_Priority::MEDIUM,
            Resource_Priority::HIGH,
            Resource_Priority::CRITICAL
        };

        Resource_Handle handles[4];

        // Request resources with different priorities
        for (ds_i32 i = 0; i < 4; i++)
        {
            Streaming_Allocator::Resource_Request request;
            request.resource_id = 0; // Generate new ID
            request.path = paths[i];
            request.category = Streaming_Allocator::Resource_Category::GENERIC;
            request.priority = priorities[i];
            request.access_mode = Streaming_Allocator::Access_Mode::READ_ONLY;
            request.callback = nullptr;
            request.user_data = nullptr;
            request.auto_unload = true;
            request.estimated_size = 256 * 1024; // 256KB

            handles[i] = allocator->Request_Resource(request);
            DS_EXPECT(handles[i].IsValid());
        }

        // Update to process loading
        DS_LOG_INFO("{}", DS_STYLED(DS_CONSOLE_FG_BRIGHT_BLUE DS_CONSOLE_TEXT_BLINK, "There should be a warning right below so don't worry!" DS_CONSOLE_TEXT_RESET));
        for (int i = 0; i < 5; i++) // Several updates to ensure loading completes
        {
            allocator->Update(0.016f);
        }

        // Verify all resources are loaded
        for (ds_i32 i = 0; i < 4; i++)
        {
            const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handles[i]);
            DS_EXPECT(info != nullptr);
            DS_EXPECT(info->state == Resource_State::RESIDENT);
            DS_EXPECT(info->priority == priorities[i]);
        }

        // Test changing priorities
        DS_EXPECT(allocator->Set_Resource_Priority(handles[0], Resource_Priority::HIGH));

        // Verify priority was changed
        const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handles[0]);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->priority == Resource_Priority::HIGH);

        // Try to unload critical resource - should fail because it's critical
        bool unload_result = allocator->Unload_Resource(handles[3]);

        DS_LOG_INFO("{}", DS_STYLED(DS_CONSOLE_FG_BRIGHT_BLUE DS_CONSOLE_TEXT_BLINK, "There should be a warning right below so don't worry!" DS_CONSOLE_TEXT_RESET));
        // Update to process unloading
        allocator->Update(0.016f);

        // Verify the critical resource is still resident
        info = allocator->Get_Resource_Info(handles[3]);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->state == Resource_State::RESIDENT);

        return true;
    }

    // Test resource categories and budgets
    static ds_bool Test_Streaming_Categories()
    {
        Streaming_Allocator::Config config;
        config.total_memory_budget = 10 * 1024 * 1024; // 10MB total

        // Adjust budgets for each category
        config.geometry_budget_percent = 40;  // 4MB
        config.texture_budget_percent = 30;   // 3MB
        config.audio_budget_percent = 10;     // 1MB
        config.animation_budget_percent = 10; // 1MB
        config.script_budget_percent = 5;     // 0.5MB
        config.generic_budget_percent = 5;    // 0.5MB

        std::unique_ptr<Streaming_Allocator> allocator =  std::make_unique<Streaming_Allocator>(config, "CategoryStreamingAllocator");

        // Request resources in different categories
        Streaming_Allocator::Resource_Category categories[] = {
            Streaming_Allocator::Resource_Category::GEOMETRY,
            Streaming_Allocator::Resource_Category::TEXTURE,
            Streaming_Allocator::Resource_Category::AUDIO,
            Streaming_Allocator::Resource_Category::ANIMATION,
            Streaming_Allocator::Resource_Category::SCRIPT,
            Streaming_Allocator::Resource_Category::GENERIC
        };

        Resource_Handle handles[6];

        // Request one resource in each category
        for (ds_i32 i = 1; i < 7; i++)
        {
            Streaming_Allocator::Resource_Request request;
            request.resource_id = i; // Generate new ID
            request.path = "test_data/test_resource.bin";
            request.category = categories[i-1];
            request.priority = Resource_Priority::MEDIUM;
            request.access_mode = Streaming_Allocator::Access_Mode::READ_ONLY;
            request.callback = nullptr;
            request.user_data = nullptr;
            request.auto_unload = true;
            request.estimated_size = 256 * 1024; // 256KB

            handles[i-1] = allocator->Request_Resource(request);
            DS_EXPECT(handles[i-1].IsValid());
        }

        // Update to process loading
        for (int i = 0; i < 5; i++) // Several updates to ensure loading completes
        {
            allocator->Update(0.016f);
        }

        // Verify all resources are loaded and in correct categories
        for (ds_i32 i = 0; i < 6; i++)
        {
            const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handles[i]);
            DS_EXPECT(info != nullptr);
            DS_EXPECT(info->state == Resource_State::RESIDENT);
            DS_EXPECT(info->category == categories[i]);
        }

        // Get stats and verify category usage
        auto stats = allocator->Get_Stats();

        // Each category should have at least one resource
        for (ds_i32 i = 0; i < 6; i++)
        {
            DS_EXPECT(stats.category_stats[i].resource_count > 0);
            DS_EXPECT(stats.category_stats[i].memory_used > 0);
        }

        return true;
    }

    // Test resource callbacks
    static ds_bool Test_Streaming_Callbacks()
    {
        Streaming_Allocator::Config config;
        std::unique_ptr<Streaming_Allocator> allocator = std::make_unique<Streaming_Allocator>(config, "CallbackStreamingAllocator");

        // Create a flag to track callback invocation
        bool callback_called = false;

        // Create a resource request with a callback
        Streaming_Allocator::Resource_Request request;
        request.resource_id = 0; // Generate new ID
        request.path = "test_data/callback_resource.bin";
        request.category = Streaming_Allocator::Resource_Category::GENERIC;
        request.priority = Resource_Priority::MEDIUM;
        request.access_mode = Streaming_Allocator::Access_Mode::READ_ONLY;
        request.callback = Resource_Loaded_Callback_Func;
        request.user_data = &callback_called;
        request.auto_unload = true;
        request.estimated_size = 256 * 1024; // 256KB

        // Request the resource
        Resource_Handle handle = allocator->Request_Resource(request);
        DS_EXPECT(handle.IsValid());

        // Initial state should be loading
        DS_EXPECT(handle.state == Resource_State::LOADING);

        // Update to process loading
        for (int i = 0; i < 5; i++) // Several updates to ensure loading completes
        {
            allocator->Update(0.016f);

            // Check if the callback has been called
            if (callback_called)
            {
                break;
            }
        }

        // Verify the callback was called
        DS_EXPECT(callback_called);

        // Verify resource is resident
        const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->state == Resource_State::RESIDENT);

        return true;
    }

    // Test prefetching
    static ds_bool Test_Streaming_Prefetch()
    {
        Streaming_Allocator::Config config;
        std::unique_ptr<Streaming_Allocator> allocator = std::make_unique<Streaming_Allocator>(config, "PrefetchStreamingAllocator");

        // Prefetch a resource
        Resource_Handle handle = allocator->Prefetch_Resource("test_data/prefetch_resource.bin");
        DS_EXPECT(handle.IsValid());

        // Update to process prefetching
        for (int i = 0; i < 5; i++) // Several updates to ensure loading completes
        {
            allocator->Update(0.016f);
        }

        // Get resource info
        const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);

        // Verify prefetched resource has correct properties
        DS_EXPECT(info->priority == Resource_Priority::BACKGROUND); // Prefetched resources should be background priority
        DS_EXPECT(info->auto_unload == true); // Should be auto-unloadable

        // Test position-based prefetching
        allocator->Prefetch_Resources_At_Position(100.0f, 100.0f, 100.0f, 50.0f);

        // Update to process any potential prefetching
        allocator->Update(0.016f);

        return true;
    }

    // Test automatic unloading
    static ds_bool Test_Streaming_Auto_Unload()
    {
        Streaming_Allocator::Config config;
        config.cache_seconds = 1; // Very short cache time for testing
        std::unique_ptr<Streaming_Allocator> allocator = std::make_unique<Streaming_Allocator>(config, "AutoUnloadStreamingAllocator");

        // Request a resource with auto_unload=true
        Streaming_Allocator::Resource_Request request;
        request.resource_id = 0; // Generate new ID
        request.path = "test_data/auto_unload_resource.bin";
        request.category = Streaming_Allocator::Resource_Category::GENERIC;
        request.priority = Resource_Priority::LOW; // Low priority to ensure it can be unloaded
        request.access_mode = Streaming_Allocator::Access_Mode::READ_ONLY;
        request.callback = nullptr;
        request.user_data = nullptr;
        request.auto_unload = true;
        request.estimated_size = 256 * 1024; // 256KB

        Resource_Handle handle = allocator->Request_Resource(request);
        DS_EXPECT(handle.IsValid());

        // Update to process loading
        for (int i = 0; i < 5; i++) // Several updates to ensure loading completes
        {
            allocator->Update(0.016f);
        }

        // Verify resource is resident
        const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->state == Resource_State::RESIDENT);

        // Wait for the resource to time out (sleep for more than cache_seconds)
        std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // 1.5 seconds

        // Update to process automatic unloading
        allocator->Update(0.016f);

        // Check if the resource was automatically unloaded
        info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);

        // The resource should be either unloading or unloaded
        DS_EXPECT(info->state == Resource_State::UNLOADING || info->state == Resource_State::UNLOADED);

        return true;
    }

    // Test memory pressure and non-critical resource clearing
    static ds_bool Test_Streaming_Memory_Pressure()
    {
        Streaming_Allocator::Config config;
        config.total_memory_budget = 512 * 1024 * 1024; // Budget (512MB)
        std::unique_ptr<Streaming_Allocator> allocator = std::make_unique<Streaming_Allocator>(config, "MemoryPressureStreamingAllocator");

        // Request several resources with different priorities
        Resource_Priority priorities[] = {
            Resource_Priority::LOW,
            Resource_Priority::MEDIUM,
            Resource_Priority::HIGH,
            Resource_Priority::CRITICAL
        };

        Resource_Handle handles[4];

        // Request resources with different priorities
        for (ds_i32 i = 0; i < 4; i++)
        {
            Streaming_Allocator::Resource_Request request;
            request.resource_id = 0; // Generate new ID
            request.path = "test_data/test_resource.bin";
            request.category = Streaming_Allocator::Resource_Category::GENERIC;
            request.priority = priorities[i];
            request.access_mode = Streaming_Allocator::Access_Mode::READ_ONLY;
            request.callback = nullptr;
            request.user_data = nullptr;
            request.auto_unload = true;
            request.estimated_size = 512 * 1024; // 512KB each (total: 2MB)

            handles[i] = allocator->Request_Resource(request);
            DS_EXPECT(handles[i].IsValid());
        }

        // Update to process loading
        for (int i = 0; i < 5; i++) // Several updates to ensure loading completes
        {
            allocator->Update(0.016f);
        }

        // Clear non-critical resources
        allocator->Clear_Non_Critical_Resources();

        // Update to process unloading
        allocator->Update(0.016f);

        // Check the state of each resource
        for (ds_i32 i = 0; i < 4; i++)
        {
            const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handles[i]);
            DS_EXPECT(info != nullptr);

            if (info->priority == Resource_Priority::CRITICAL)
            {
                // Critical resources should still be resident
                DS_EXPECT(info->state == Resource_State::RESIDENT);
            }
            else
            {
                // Non-critical resources should be unloading or unloaded
                DS_EXPECT(info->state == Resource_State::UNLOADING || info->state == Resource_State::UNLOADED);
            }
        }

        return true;
    }

    // Test streaming allocator move operations
    static ds_bool Test_Streaming_Move_Operations()
    {
        Streaming_Allocator::Config config;
        std::unique_ptr<Streaming_Allocator> allocator1 = std::make_unique<Streaming_Allocator>(config, "SourceStreamingAllocator");

        // Request a resource
        Resource_Handle handle = allocator1->Prefetch_Resource("test_data/move_resource.bin");
        DS_EXPECT(handle.IsValid());

        // Update to process loading
        allocator1->Update(0.016f);

        // Move construct to a new allocator
        std::unique_ptr<Streaming_Allocator> allocator2(std::move(allocator1));

        // Request another resource from the moved-to allocator
        Resource_Handle handle2 = allocator2->Prefetch_Resource("test_data/move_resource2.bin");
        DS_EXPECT(handle2.IsValid());

        // Update the new allocator
        allocator2->Update(0.016f);

        // Create a third allocator and move-assign
        Streaming_Allocator::Config config3;
        config3.total_memory_budget = 8 * 1024 * 1024; // Different budget
        std::unique_ptr<Streaming_Allocator> allocator3 = std::make_unique<Streaming_Allocator>(config3, "TargetStreamingAllocator");

        // Move allocator2 to allocator3
        *allocator3 = std::move(*allocator2);

        // Try to use allocator3
        Resource_Handle handle3 = allocator3->Prefetch_Resource("test_data/move_resource3.bin");
        DS_EXPECT(handle3.IsValid());

        // Update allocator3
        allocator3->Update(0.016f);

        return true;
    }

    // Test resource stats
    static ds_bool Test_Streaming_Stats()
    {
        Streaming_Allocator::Config config;
        config.max_concurrent_operations = 5;
        std::unique_ptr<Streaming_Allocator> allocator = std::make_unique<Streaming_Allocator>(config, "StatsStreamingAllocator");

        // Get initial stats
        auto initial_stats = allocator->Get_Stats();

        // Verify initial stats
        DS_EXPECT(initial_stats.resource_count == 0);
        DS_EXPECT(initial_stats.total_memory_used == 0);

        // Request several resources
        std::vector<Resource_Handle> handles;
        const ds_i32 resource_count = 5;

        for (ds_i32 i = 0; i < resource_count; i++)
        {
            Resource_Handle handle = allocator->Prefetch_Resource("test_data/stats_resource.bin");
            DS_EXPECT(handle.IsValid());
            handles.push_back(handle);
        }

        // Update to process loading
        for (int i = 0; i < 5; i++) // Several updates to ensure loading completes
        {
            allocator->Update(0.016f);
        }

        // Get updated stats
        auto updated_stats = allocator->Get_Stats();

        // Verify updated stats
        DS_EXPECT(updated_stats.resource_count == resource_count);
        DS_EXPECT(updated_stats.total_memory_used > 0);
        DS_EXPECT(updated_stats.load_operations >= resource_count);

        // Unload resources
        for (auto handle : handles)
        {
            allocator->Unload_Resource(handle);
        }

        // Update to process unloading
        allocator->Update(0.016f);

        // Get final stats
        auto final_stats = allocator->Get_Stats();

        // Verify resources were unloaded
        DS_EXPECT(final_stats.unload_operations >= resource_count);

        return true;
    }

    // Test memory-mapped file loading and modification
    static ds_bool Test_Streaming_Memory_Mapped_Files()
    {
        // Create a streaming allocator with default configuration
        Streaming_Allocator::Config config;
        std::unique_ptr<Streaming_Allocator> allocator = std::make_unique<Streaming_Allocator>(config, "MappedFileStreamingAllocator");

        // Path to our test file
        const char* test_file = "test_data/mapped_file.bin";

        // Create a test file with known content
        {
            std::ofstream file(test_file, std::ios::binary);
            if (!file)
            {
                DS_LOG_ERROR("Failed to create test file: {0}", test_file);
                return false;
            }

            // Create a 1MB file filled with a repeating pattern
            std::vector<char> data(1024 * 1024);
            for (size_t i = 0; i < data.size(); i++)
            {
                data[i] = static_cast<char>(i % 256);
            }
            file.write(data.data(), data.size());
        }

        // Request the file with persistent write access
        Streaming_Allocator::Resource_Request request;
        request.resource_id = 0; // Generate new ID
        request.path = test_file;
        request.category = Streaming_Allocator::Resource_Category::GENERIC;
        request.priority = Resource_Priority::MEDIUM;
        request.access_mode = Streaming_Allocator::Access_Mode::PERSISTENT_WRITE; // Important for memory mapping with write
        request.callback = nullptr;
        request.user_data = nullptr;
        request.auto_unload = false; // Don't auto-unload
        request.estimated_size = 1024 * 1024; // 1MB

        // Request the resource (which should be memory-mapped)
        Resource_Handle handle = allocator->Request_Resource(request);
        DS_EXPECT(handle.IsValid());

        // Update to process loading
        for (int i = 0; i < 10; i++)
        {
            allocator->Update(0.016f);

            // Check if the resource is loaded
            const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handle);
            if (info && info->state == Resource_State::RESIDENT)
            {
                break; // Exit the loop once loaded
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // Verify resource is loaded
        const Streaming_Allocator::Resource_Info* info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->state == Resource_State::RESIDENT);

        // Access the memory-mapped file data
        void* data = allocator->Access_Resource(handle);
        DS_EXPECT(data != nullptr);

        // Verify the file content matches what we wrote
        ds_u8* bytes = static_cast<ds_u8*>(data);
        for (ds_u64 i = 0; i < 1024; i++) // Check first 1KB
        {
            ds_u8 expected = static_cast<ds_u8>(i % 256);
            DS_EXPECT(bytes[i] == expected);
        }

        // Modify the memory-mapped data
        for (ds_u64 i = 0; i < 1024; i++)
        {
            // Invert the pattern
            bytes[i] = static_cast<ds_u8>(255 - (i % 256));
        }

        // Flush changes to disk
        allocator->Flush_Resources();

        // Unload the resource
        DS_EXPECT(allocator->Unload_Resource(handle));
        allocator->Update(0.016f); // Process unloading

        // Verify the resource is unloaded
        info = allocator->Get_Resource_Info(handle);
        DS_EXPECT(info != nullptr);
        DS_EXPECT(info->state == Resource_State::UNLOADED);

        // Now read the file back and verify our changes were persisted
        {
            std::ifstream file(test_file, std::ios::binary);
            DS_EXPECT(file.is_open());

            if (file.is_open())
            {
                std::vector<char> read_buffer(1024);
                file.read(read_buffer.data(), read_buffer.size());

                // Verify the modified pattern
                for (ds_u64 i = 0; i < 1024; i++)
                {
                    ds_u8 expected = static_cast<ds_u8>(255 - (i % 256));
                    ds_u8 actual = static_cast<ds_u8>(read_buffer[i]);
                    DS_EXPECT(actual == expected);
                }
            }
        }

        // Clean up - request with read access to verify changes persisted through reload
        Streaming_Allocator::Resource_Request read_request;
        read_request.resource_id = 0; // Generate new ID
        read_request.path = test_file;
        read_request.category = Streaming_Allocator::Resource_Category::GENERIC;
        read_request.priority = Resource_Priority::MEDIUM;
        read_request.access_mode = Streaming_Allocator::Access_Mode::READ_ONLY; // Read-only this time
        read_request.callback = nullptr;
        read_request.user_data = nullptr;
        read_request.auto_unload = true;
        read_request.estimated_size = 1024 * 1024;

        Resource_Handle read_handle = allocator->Request_Resource(read_request);
        DS_EXPECT(read_handle.IsValid());

        // Update to process loading
        for (int i = 0; i < 10; i++)
        {
            allocator->Update(0.016f);

            // Check if the resource is loaded
            const Streaming_Allocator::Resource_Info* read_info = allocator->Get_Resource_Info(read_handle);
            if (read_info && read_info->state == Resource_State::RESIDENT)
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // Access the memory-mapped file data again
        data = allocator->Access_Resource(read_handle);
        DS_EXPECT(data != nullptr);

        // Verify the file content now contains our modified data
        bytes = static_cast<ds_u8*>(data);
        for (ds_u64 i = 0; i < 1024; i++)
        {
            ds_u8 expected = static_cast<ds_u8>(255 - (i % 256));
            DS_EXPECT(bytes[i] == expected);
        }

        return true;
    }

    // Add all tests to the test suite
    static ds_bool Add_All_Tests(Test_Suite& test_suite)
    {
        // Create test files at the beginning
        Create_Test_Files();

        DS_TEST(test_suite, "Basic Streaming Operations")
        {
            return Test_Streaming_Basic();
        });

        DS_TEST(test_suite, "Streaming Resource Priorities")
        {
            return Test_Streaming_Priorities();
        });

        DS_TEST(test_suite, "Streaming Resource Categories")
        {
            return Test_Streaming_Categories();
        });

        DS_TEST(test_suite, "Streaming Resource Callbacks")
        {
            return Test_Streaming_Callbacks();
        });

        DS_TEST(test_suite, "Streaming Resource Prefetching")
        {
            return Test_Streaming_Prefetch();
        });

        DS_TEST(test_suite, "Streaming Auto Unloading")
        {
            return Test_Streaming_Auto_Unload();
        });

        DS_TEST(test_suite, "Streaming Memory Pressure")
        {
            return Test_Streaming_Memory_Pressure();
        });

        DS_TEST(test_suite, "Streaming Move Operations")
        {
            return Test_Streaming_Move_Operations();
        });

        DS_TEST(test_suite, "Streaming Resource Stats")
        {
            return Test_Streaming_Stats();
        });

        DS_TEST(test_suite, "Streaming Memory-Mapped Files")
        {
            return Test_Streaming_Memory_Mapped_Files();
        });

        return true;
    }
}