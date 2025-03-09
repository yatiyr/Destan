#include <core/destan_pch.h>
#include <core/memory/streaming_allocator.h>

// For getting current time
#ifdef DESTAN_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <sys/time.h>
#endif

namespace destan::core::memory
{
    // Helper function to get current time in milliseconds
    destan_u64 GetCurrentTimeMS()
    {
#ifdef DESTAN_PLATFORM_WINDOWS
        LARGE_INTEGER frequency, counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        return static_cast<destan_u64>(counter.QuadPart * 1000 / frequency.QuadPart);
#else
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return static_cast<destan_u64>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
#endif
    }

    Streaming_Allocator::Streaming_Allocator(const Config& config, const destan_char* name)
        : m_config(config)
        , m_page_allocator(config.page_size, 0, "Streaming_Page_Allocator")
    {
        // Safely copy the name to our fixed buffer
        if (name)
        {
            destan_u64 name_length = 0;
            while (name[name_length] && name_length < MAX_NAME_LENGTH - 1)
            {
                m_name[name_length] = name[name_length];
                name_length++;
            }
            m_name[name_length] = '\0';
        }
        else
        {
            // Default name if none provided
            const char* default_name = "Streaming_Allocator";
            destan_u64 i = 0;
            while (default_name[i] && i < MAX_NAME_LENGTH - 1)
            {
                m_name[i] = default_name[i];
                i++;
            }
            m_name[i] = '\0';
        }

        // Set up memory budgets for each category
        m_category_memory_budget[static_cast<int>(Resource_Category::GEOMETRY)] =
            (m_config.total_memory_budget * m_config.geometry_budget_percent) / 100;

        m_category_memory_budget[static_cast<int>(Resource_Category::TEXTURE)] =
            (m_config.total_memory_budget * m_config.texture_budget_percent) / 100;

        m_category_memory_budget[static_cast<int>(Resource_Category::AUDIO)] =
            (m_config.total_memory_budget * m_config.audio_budget_percent) / 100;

        m_category_memory_budget[static_cast<int>(Resource_Category::ANIMATION)] =
            (m_config.total_memory_budget * m_config.animation_budget_percent) / 100;

        m_category_memory_budget[static_cast<int>(Resource_Category::SCRIPT)] =
            (m_config.total_memory_budget * m_config.script_budget_percent) / 100;

        m_category_memory_budget[static_cast<int>(Resource_Category::GENERIC)] =
            (m_config.total_memory_budget * m_config.generic_budget_percent) / 100;

        // Initialize statistics
        Memory::Memset(&m_stats, 0, sizeof(Stats));
        m_stats.total_memory_budget = m_config.total_memory_budget;

        for (int i = 0; i < 6; i++)
        {
            m_stats.category_stats[i].memory_budget = m_category_memory_budget[i];
        }

        // Initialize resource entries
        Memory::Memset(m_resources, 0, sizeof(m_resources));

        // Record start time
        m_last_update_time = GetCurrentTimeMS();

        DESTAN_LOG_INFO("Streaming Allocator '{0}' initialized with {1} MB memory budget",
            m_name, m_config.total_memory_budget / (1024 * 1024));
    }

    Streaming_Allocator::~Streaming_Allocator()
    {
        // Unload all resources
        for (destan_u64 i = 0; i < m_resource_count; i++)
        {
            Resource_Entry& entry = m_resources[i];
            if (entry.data && entry.info.state == Resource_State::RESIDENT)
            {
                // Deallocate the memory
                m_page_allocator.Deallocate(entry.data);
                entry.data = nullptr;
            }
        }

        DESTAN_LOG_INFO("Streaming Allocator '{0}' destroyed. Stats: {1} resources, {2} MB loaded, {3} load operations",
            m_name, m_stats.resource_count, m_stats.total_memory_used / (1024 * 1024), m_stats.load_operations);
    }

    Streaming_Allocator::Streaming_Allocator(Streaming_Allocator&& other) noexcept
        : m_config(other.m_config)
        , m_resource_count(other.m_resource_count)
        , m_player_x(other.m_player_x)
        , m_player_y(other.m_player_y)
        , m_player_z(other.m_player_z)
        , m_last_update_time(other.m_last_update_time)
        , m_page_allocator(std::move(other.m_page_allocator))
        , m_stats(other.m_stats)
    {
        // Copy memory tracking
        Memory::Memcpy(m_category_memory_used, other.m_category_memory_used, sizeof(m_category_memory_used));
        Memory::Memcpy(m_category_memory_budget, other.m_category_memory_budget, sizeof(m_category_memory_budget));

        // Copy resource entries
        Memory::Memcpy(m_resources, other.m_resources, sizeof(Resource_Entry) * m_resource_count);

        // Move operation queues
        m_pending_operations = std::move(other.m_pending_operations);
        m_active_operations = std::move(other.m_active_operations);

        // Copy name
        Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

        // Set next resource ID
        m_next_resource_id.store(other.m_next_resource_id.load(std::memory_order_relaxed), std::memory_order_relaxed);

        // Clear the moved-from object
        other.m_resource_count = 0;
        Memory::Memset(other.m_category_memory_used, 0, sizeof(other.m_category_memory_used));
        Memory::Memset(other.m_resources, 0, sizeof(other.m_resources));
        other.m_last_update_time = 0;
        Memory::Memset(&other.m_stats, 0, sizeof(other.m_stats));
    }

    Streaming_Allocator& Streaming_Allocator::operator=(Streaming_Allocator&& other) noexcept
    {
        if (this != &other)
        {
            // Clear existing resources
            for (destan_u64 i = 0; i < m_resource_count; i++)
            {
                if (m_resources[i].data && m_resources[i].info.state == Resource_State::RESIDENT)
                {
                    m_page_allocator.Deallocate(m_resources[i].data);
                }
            }

            // Move page allocator
            m_page_allocator = std::move(other.m_page_allocator);

            // Copy configuration and state
            m_config = other.m_config;
            m_resource_count = other.m_resource_count;
            m_player_x = other.m_player_x;
            m_player_y = other.m_player_y;
            m_player_z = other.m_player_z;
            m_last_update_time = other.m_last_update_time;
            m_stats = other.m_stats;

            // Copy memory tracking
            Memory::Memcpy(m_category_memory_used, other.m_category_memory_used, sizeof(m_category_memory_used));
            Memory::Memcpy(m_category_memory_budget, other.m_category_memory_budget, sizeof(m_category_memory_budget));

            // Copy resource entries
            Memory::Memcpy(m_resources, other.m_resources, sizeof(Resource_Entry) * m_resource_count);

            // Move operation queues
            m_pending_operations = std::move(other.m_pending_operations);
            m_active_operations = std::move(other.m_active_operations);

            // Copy name
            Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

            // Set next resource ID
            m_next_resource_id.store(other.m_next_resource_id.load(std::memory_order_relaxed), std::memory_order_relaxed);

            // Clear the moved-from object
            other.m_resource_count = 0;
            Memory::Memset(other.m_category_memory_used, 0, sizeof(other.m_category_memory_used));
            Memory::Memset(other.m_resources, 0, sizeof(other.m_resources));
            other.m_last_update_time = 0;
            Memory::Memset(&other.m_stats, 0, sizeof(other.m_stats));
        }
        return *this;
    }

    Resource_Handle Streaming_Allocator::Request_Resource(const Resource_Request& request)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        Resource_Handle handle;
        handle.id = 0;
        handle.state = Resource_State::UNLOADED;

        // Check if the resource already exists
        Resource_Entry* existing = Find_Resource_Entry(request.resource_id);
        if (existing)
        {
            // If it exists and is being unloaded, cancel the unload
            if (existing->unloading_scheduled)
            {
                existing->unloading_scheduled = false;
                DESTAN_LOG_TRACE("Streaming Allocator '{0}': Canceling unload for resource {1}",
                    m_name, request.resource_id);
            }

            // Update priority if needed
            if (existing->info.priority < request.priority)
            {
                existing->info.priority = request.priority;

                // Re-prioritize in the loading queue if still loading
                if (existing->info.state == Resource_State::LOADING || existing->loading_scheduled)
                {
                    // Remove any pending operations for this resource
                    auto it = std::remove_if(m_pending_operations.begin(), m_pending_operations.end(),
                        [request](const IO_Operation& op) {
                            return op.type == IO_Operation::LOAD && op.resource_id == request.resource_id;
                        });

                    if (it != m_pending_operations.end())
                    {
                        m_pending_operations.erase(it, m_pending_operations.end());

                        // Add back with new priority
                        IO_Operation op;
                        op.type = IO_Operation::LOAD;
                        op.resource_id = request.resource_id;
                        op.path = existing->info.path;
                        op.priority = request.priority;
                        m_pending_operations.push_back(op);
                    }
                }
            }

            // Update the auto_unload flag
            existing->info.auto_unload = request.auto_unload;

            // Set callback if provided
            if (request.callback)
            {
                existing->callback = request.callback;
                existing->user_data = request.user_data;
            }

            // Create and return handle
            handle.id = existing->info.id;
            handle.state = existing->info.state;
            return handle;
        }

        // Check if we have room to track this resource
        if (m_resource_count >= MAX_RESOURCES)
        {
            DESTAN_LOG_ERROR("Streaming Allocator '{0}': Maximum number of resources ({1}) reached",
                m_name, MAX_RESOURCES);
            return handle;
        }

        // Create a new resource entry
        Resource_Entry& entry = m_resources[m_resource_count++];
        entry.info.id = request.resource_id > 0 ? request.resource_id : Generate_Resource_ID();

        // Copy path
        destan_u64 path_len = strlen(request.path);
        destan_u64 max_copy = std::min(path_len, static_cast<destan_u64>(255));
        Memory::Memcpy(entry.info.path, request.path, max_copy);
        entry.info.path[max_copy] = '\0';

        entry.info.category = request.category;
        entry.info.priority = request.priority;
        entry.info.state = Resource_State::UNLOADED;
        entry.info.size = request.estimated_size;
        entry.info.last_used_time = GetCurrentTimeMS();
        entry.info.auto_unload = request.auto_unload;
        entry.info.reference_count = 0;

        entry.data = nullptr;
        entry.callback = request.callback;
        entry.user_data = request.user_data;
        entry.access_mode = request.access_mode;
        entry.distance_from_player = 0.0f;
        entry.loading_scheduled = false;
        entry.unloading_scheduled = false;

        // Schedule loading
        Schedule_Resource_Load(&entry);

        // Create and return handle
        handle.id = entry.info.id;
        handle.state = entry.info.state;

        // Increment resource count in stats
        m_stats.resource_count++;
        m_stats.category_stats[static_cast<int>(request.category)].resource_count++;

        return handle;
    }

    Resource_Handle Streaming_Allocator::Prefetch_Resource(const destan_char* path, Resource_Category category)
    {
        Resource_Request request;
        request.resource_id = 0; // Generate a new ID
        request.path = path;
        request.category = category;
        request.priority = Resource_Priority::BACKGROUND; // Low priority for prefetching
        request.access_mode = Access_Mode::READ_ONLY;
        request.callback = nullptr;
        request.user_data = nullptr;
        request.auto_unload = true;
        request.estimated_size = 0; // Unknown size

        return Request_Resource(request);
    }

    void* Streaming_Allocator::Access_Resource(Resource_Handle handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Find the resource entry
        Resource_Entry* entry = Find_Resource_Entry(handle);
        if (!entry || !Is_Resource_Ready(entry))
        {
            return nullptr;
        }

        // Update the last used time
        entry->info.last_used_time = GetCurrentTimeMS();

        // Return the data pointer
        return entry->data;
    }

    bool Streaming_Allocator::Reference_Resource(Resource_Handle handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Find the resource entry
        Resource_Entry* entry = Find_Resource_Entry(handle);
        if (!entry)
        {
            return false;
        }

        // Increment the reference count
        entry->info.reference_count++;

        // Update the last used time
        entry->info.last_used_time = GetCurrentTimeMS();

        // If it was scheduled for unloading, cancel it
        if (entry->unloading_scheduled)
        {
            entry->unloading_scheduled = false;
            DESTAN_LOG_TRACE("Streaming Allocator '{0}': Canceling unload for resource {1}",
                m_name, entry->info.id);
        }

        return true;
    }

    bool Streaming_Allocator::Release_Resource(Resource_Handle handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Find the resource entry
        Resource_Entry* entry = Find_Resource_Entry(handle);
        if (!entry)
        {
            return false;
        }

        // Decrement the reference count (if > 0)
        if (entry->info.reference_count > 0)
        {
            entry->info.reference_count--;
        }

        return true;
    }

    void Streaming_Allocator::Prefetch_Resources_At_Position(float position_x, float position_y, float position_z,
        float radius, Resource_Category category)
    {
        // Update player position (used for distance calculations)
        m_player_x = position_x;
        m_player_y = position_y;
        m_player_z = position_z;

        // This method would typically search for resources near the given position
        // and prefetch them. In a real implementation, this would likely query a
        // spatial database or resource registry to find nearby resources.

        // For now, just log that we intend to prefetch
        DESTAN_LOG_TRACE("Streaming Allocator '{0}': Prefetching resources at position ({1}, {2}, {3}) with radius {4}",
            m_name, position_x, position_y, position_z, radius);

        // The implementation would depend on how resources are organized spatially
        // In a real engine, we might have a spatial index like:
        //   std::vector<Resource_Info> nearby_resources = m_resource_registry.Query_Region(position_x, position_y, position_z, radius);
        //   for (const auto& resource : nearby_resources) {
        //       if (category == Resource_Category::GENERIC || resource.category == category) {
        //           Prefetch_Resource(resource.path, resource.category);
        //       }
        //   }
    }

    bool Streaming_Allocator::Set_Resource_Priority(Resource_Handle handle, Resource_Priority priority)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Find the resource entry
        Resource_Entry* entry = Find_Resource_Entry(handle);
        if (!entry)
        {
            return false;
        }

        // Update priority
        entry->info.priority = priority;

        // If it's in the loading queue, update its priority
        if (entry->info.state == Resource_State::LOADING || entry->loading_scheduled)
        {
            // Remove any pending operations for this resource
            auto it = std::remove_if(m_pending_operations.begin(), m_pending_operations.end(),
                [entry](const IO_Operation& op) {
                    return op.type == IO_Operation::LOAD && op.resource_id == entry->info.id;
                });

            if (it != m_pending_operations.end())
            {
                m_pending_operations.erase(it, m_pending_operations.end());

                // Add back with new priority
                IO_Operation op;
                op.type = IO_Operation::LOAD;
                op.resource_id = entry->info.id;
                op.path = entry->info.path;
                op.priority = priority;
                m_pending_operations.push_back(op);
            }
        }

        return true;
    }

    bool Streaming_Allocator::Unload_Resource(Resource_Handle handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Find the resource entry
        Resource_Entry* entry = Find_Resource_Entry(handle);
        if (!entry)
        {
            return false;
        }

        // If it has references, we can't unload it
        if (entry->info.reference_count > 0)
        {
            DESTAN_LOG_WARN("Streaming Allocator '{0}': Cannot unload resource {1} with {2} references",
                m_name, entry->info.id, entry->info.reference_count);
            return false;
        }

        // If it's already unloading or not loaded, do nothing
        if (entry->info.state == Resource_State::UNLOADING ||
            entry->info.state == Resource_State::UNLOADED)
        {
            return true;
        }

        // Schedule unloading
        Schedule_Resource_Unload(entry);
        return true;
    }

    void Streaming_Allocator::Flush_Resources()
    {
        // For memory-mapped files that have been modified, we need to flush changes to disk
        std::lock_guard<std::mutex> lock(m_mutex);

        for (destan_u64 i = 0; i < m_resource_count; i++)
        {
            Resource_Entry& entry = m_resources[i];

            // Only flush resources that are loaded and have write access
            if (entry.info.state == Resource_State::RESIDENT &&
                (entry.access_mode == Access_Mode::READ_WRITE ||
                    entry.access_mode == Access_Mode::PERSISTENT_WRITE) &&
                entry.data)
            {
                // Flush memory-mapped file
                m_page_allocator.Flush(entry.data, entry.info.size);

                DESTAN_LOG_TRACE("Streaming Allocator '{0}': Flushed resource {1} to disk",
                    m_name, entry.info.id);
            }
        }
    }

    void Streaming_Allocator::Update(float delta_time)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Get current time
        destan_u64 current_time = GetCurrentTimeMS();
        destan_u64 time_since_last_update = current_time - m_last_update_time;
        m_last_update_time = current_time;

        // Update resource distances based on player position
        Update_Resource_Distances(m_player_x, m_player_y, m_player_z);

        // Check for resources that can be unloaded due to timeout
        Check_Resource_Lifetimes();

        // Process pending IO operations
        Process_IO_Operations();

        // Update loading queue based on priorities
        Update_Loading_Queue();

        // Log detailed stats if enabled
        if (m_config.log_detailed_stats)
        {
            DESTAN_LOG_INFO("Streaming Allocator '{0}': {1} resources, {2}/{3} MB used, {4} loading, {5} operations pending",
                m_name, m_stats.resource_count,
                m_stats.total_memory_used / (1024 * 1024),
                m_stats.total_memory_budget / (1024 * 1024),
                m_stats.loading_count,
                m_pending_operations.size());
        }
    }

    const Streaming_Allocator::Resource_Info* Streaming_Allocator::Get_Resource_Info(Resource_Handle handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Find the resource entry
        const Resource_Entry* entry = Find_Resource_Entry(handle);
        if (!entry)
        {
            return nullptr;
        }

        return &entry->info;
    }

    Streaming_Allocator::Stats Streaming_Allocator::Get_Stats()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stats;
    }

    // Clear all non-critical resources
    void Streaming_Allocator::Clear_Non_Critical_Resources()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Unload all non-critical resources
        for (destan_u64 i = 0; i < m_resource_count; i++)
        {
            Resource_Entry& entry = m_resources[i];

            // Skip critical resources or those with references
            if (entry.info.priority == Resource_Priority::CRITICAL ||
                entry.info.reference_count > 0)
            {
                continue;
            }

            // Schedule unloading
            if (entry.info.state == Resource_State::RESIDENT && !entry.unloading_scheduled)
            {
                Schedule_Resource_Unload(&entry);

                DESTAN_LOG_TRACE("Streaming Allocator '{0}': Cleared non-critical resource {1}",
                    m_name, entry.info.id);
            }
        }
    }

    // Find resource entry by ID
    Streaming_Allocator::Resource_Entry* Streaming_Allocator::Find_Resource_Entry(destan_u64 resource_id)
    {
        // Linear search through resources
        for (destan_u64 i = 0; i < m_resource_count; i++)
        {
            if (m_resources[i].info.id == resource_id)
            {
                return &m_resources[i];
            }
        }

        return nullptr;
    }

    // Const version of find resource entry by ID
    const Streaming_Allocator::Resource_Entry* Streaming_Allocator::Find_Resource_Entry(destan_u64 resource_id) const
    {
        // Linear search through resources
        for (destan_u64 i = 0; i < m_resource_count; i++)
        {
            if (m_resources[i].info.id == resource_id)
            {
                return &m_resources[i];
            }
        }

        return nullptr;
    }

    // Find resource entry by handle
    Streaming_Allocator::Resource_Entry* Streaming_Allocator::Find_Resource_Entry(Resource_Handle handle)
    {
        // A handle is valid if its ID is not zero
        if (!handle.IsValid())
        {
            return nullptr;
        }

        // Find by ID
        return Find_Resource_Entry(handle.id);
    }

    // Const version of find resource entry by handle
    const Streaming_Allocator::Resource_Entry* Streaming_Allocator::Find_Resource_Entry(Resource_Handle handle) const
    {
        // A handle is valid if its ID is not zero
        if (!handle.IsValid())
        {
            return nullptr;
        }

        // Find by ID
        return Find_Resource_Entry(handle.id);
    }

    // Process pending IO operations
    void Streaming_Allocator::Process_IO_Operations()
    {
        // Process a limited number of operations per frame for smooth performance
        const destan_u32 max_operations_per_update = m_config.max_concurrent_operations;
        destan_u32 operations_processed = 0;

        // First, check if any active operations have completed
        // This would be done in a background thread in a real implementation
        // For this example, we'll simulate immediate completion
        for (const auto& operation : m_active_operations)
        {
            if (operation.type == IO_Operation::LOAD)
            {
                Execute_Resource_Load(operation);
            }
            else
            {
                Execute_Resource_Unload(operation);
            }

            operations_processed++;
        }

        // Clear active operations
        m_active_operations.clear();

        // Add new operations from the pending queue
        while (!m_pending_operations.empty() && operations_processed < max_operations_per_update)
        {
            // Sort operations by priority
            std::sort(m_pending_operations.begin(), m_pending_operations.end(),
                [](const IO_Operation& a, const IO_Operation& b) {
                    // Higher priority comes first
                    return static_cast<int>(a.priority) < static_cast<int>(b.priority);
                });

            // Take the highest priority operation
            IO_Operation operation = m_pending_operations.front();
            m_pending_operations.erase(m_pending_operations.begin());

            // Add to active operations
            m_active_operations.push_back(operation);

            // In a real implementation, we would dispatch the operation to a background thread
            // But for this example, we'll execute it immediately
            if (operation.type == IO_Operation::LOAD)
            {
                Execute_Resource_Load(operation);
            }
            else
            {
                Execute_Resource_Unload(operation);
            }

            operations_processed++;
        }
    }

    // Schedule a resource for loading
    void Streaming_Allocator::Schedule_Resource_Load(Resource_Entry* entry)
    {
        if (!entry || entry->loading_scheduled || entry->info.state == Resource_State::RESIDENT)
        {
            return;
        }

        // Mark as loading
        entry->info.state = Resource_State::LOADING;
        entry->loading_scheduled = true;

        // Create an IO operation
        IO_Operation operation;
        operation.type = IO_Operation::LOAD;
        operation.resource_id = entry->info.id;
        operation.path = entry->info.path;
        operation.priority = entry->info.priority;

        // Add to pending operations
        m_pending_operations.push_back(operation);

        // Update stats
        m_stats.loading_count++;

        DESTAN_LOG_TRACE("Streaming Allocator '{0}': Scheduled load for resource {1} with priority {2}",
            m_name, entry->info.id, static_cast<int>(entry->info.priority));
    }

    // Schedule a resource for unloading
    void Streaming_Allocator::Schedule_Resource_Unload(Resource_Entry* entry)
    {
        if (!entry || entry->unloading_scheduled || entry->info.state != Resource_State::RESIDENT)
        {
            return;
        }

        // Cannot unload if it has references
        if (entry->info.reference_count > 0)
        {
            DESTAN_LOG_WARN("Streaming Allocator '{0}': Cannot unload resource {1} with {2} references",
                m_name, entry->info.id, entry->info.reference_count);
            return;
        }

        // Mark as unloading
        entry->info.state = Resource_State::UNLOADING;
        entry->unloading_scheduled = true;

        // Create an IO operation
        IO_Operation operation;
        operation.type = IO_Operation::UNLOAD;
        operation.resource_id = entry->info.id;
        operation.path = nullptr;  // Not needed for unload
        operation.priority = Resource_Priority::BACKGROUND;  // Unloads are low priority

        // Add to pending operations
        m_pending_operations.push_back(operation);

        DESTAN_LOG_TRACE("Streaming Allocator '{0}': Scheduled unload for resource {1}",
            m_name, entry->info.id);
    }

    // Execute a load operation
    void Streaming_Allocator::Execute_Resource_Load(const IO_Operation& operation)
    {
        // Find the resource entry
        Resource_Entry* entry = Find_Resource_Entry(operation.resource_id);
        if (!entry)
        {
            DESTAN_LOG_ERROR("Streaming Allocator '{0}': Cannot find resource {1} for loading",
                m_name, operation.resource_id);
            return;
        }

        // Reset loading scheduled flag
        entry->loading_scheduled = false;

        // Check if we have enough memory for this resource
        if (!Has_Available_Memory(entry->info.category, entry->info.size))
        {
            // Try to free up memory by unloading low-priority resources
            DESTAN_LOG_WARN("Streaming Allocator '{0}': Not enough memory for resource {1}, attempting to free memory",
                m_name, entry->info.id);

            // We'll need to implement a memory freeing strategy
            // For now, just mark the resource as failed
            entry->info.state = Resource_State::FAILED;
            m_stats.loading_count--;
            m_stats.failed_count++;
            return;
        }

        // Determine protection mode based on access mode
        Page_Protection protection;
        Page_Flags flags = Page_Flags::COMMIT | Page_Flags::ZERO;

        switch (entry->access_mode)
        {
        case Access_Mode::READ_ONLY:
            protection = Page_Protection::READ_ONLY;
            break;

        case Access_Mode::READ_WRITE:
        case Access_Mode::PERSISTENT_WRITE:
            protection = Page_Protection::READ_WRITE;
            break;

        default:
            protection = Page_Protection::READ_ONLY;
            break;
        }

        // For memory-mapped files, use MAP_FILE flag
        if (entry->access_mode == Access_Mode::PERSISTENT_WRITE)
        {
            flags = flags | Page_Flags::MAP_FILE;
        }

        // Allocate memory for the resource
        void* data = m_page_allocator.Allocate(
            entry->info.size,
            protection,
            flags,
            entry->access_mode == Access_Mode::PERSISTENT_WRITE ? entry->info.path : nullptr
        );

        if (!data)
        {
            DESTAN_LOG_ERROR("Streaming Allocator '{0}': Failed to allocate memory for resource {1}",
                m_name, entry->info.id);

            entry->info.state = Resource_State::FAILED;
            m_stats.loading_count--;
            m_stats.failed_count++;
            return;
        }

        // In a real implementation, we would load the resource data from disk
        // For this example, we'll just fill with a pattern to simulate loading
        if (entry->access_mode != Access_Mode::PERSISTENT_WRITE)
        {
            // Simulate loading by filling with a pattern
            for (destan_u64 i = 0; i < entry->info.size; i++)
            {
                static_cast<destan_u8*>(data)[i] = static_cast<destan_u8>(i % 256);
            }
        }

        // Set the data pointer
        entry->data = data;

        // Mark as resident
        entry->info.state = Resource_State::RESIDENT;

        // Update memory usage
        Update_Memory_Usage(entry->info.category, static_cast<destan_i64>(entry->info.size));

        // Update stats
        m_stats.loading_count--;
        m_stats.bytes_loaded += entry->info.size;
        m_stats.load_operations++;

        // Call the callback if provided
        if (entry->callback)
        {
            entry->callback(entry->info.id, entry->data, entry->info.size, entry->user_data);
        }

        DESTAN_LOG_TRACE("Streaming Allocator '{0}': Loaded resource {1} ({2} KB)",
            m_name, entry->info.id, entry->info.size / 1024);
    }

    // Execute an unload operation
    void Streaming_Allocator::Execute_Resource_Unload(const IO_Operation& operation)
    {
        // Find the resource entry
        Resource_Entry* entry = Find_Resource_Entry(operation.resource_id);
        if (!entry)
        {
            DESTAN_LOG_ERROR("Streaming Allocator '{0}': Cannot find resource {1} for unloading",
                m_name, operation.resource_id);
            return;
        }

        // Reset unloading scheduled flag
        entry->unloading_scheduled = false;

        // Skip if resource is not resident or has references
        if (entry->info.state != Resource_State::UNLOADING || entry->info.reference_count > 0)
        {
            DESTAN_LOG_WARN("Streaming Allocator '{0}': Cannot unload resource {1}, state={2}, refs={3}",
                m_name, entry->info.id, static_cast<int>(entry->info.state), entry->info.reference_count);
            return;
        }

        // If resource has persistent write access, flush changes to disk
        if (entry->access_mode == Access_Mode::PERSISTENT_WRITE && entry->data)
        {
            m_page_allocator.Flush(entry->data, entry->info.size);
        }

        // Deallocate memory
        if (entry->data)
        {
            m_page_allocator.Deallocate(entry->data);
            entry->data = nullptr;
        }

        // Update memory usage
        Update_Memory_Usage(entry->info.category, -static_cast<destan_i64>(entry->info.size));

        // Mark as unloaded
        entry->info.state = Resource_State::UNLOADED;

        // Update stats
        m_stats.bytes_unloaded += entry->info.size;
        m_stats.unload_operations++;

        DESTAN_LOG_TRACE("Streaming Allocator '{0}': Unloaded resource {1}",
            m_name, entry->info.id);
    }

    // Check if there is available memory for a resource
    bool Streaming_Allocator::Has_Available_Memory(Resource_Category category, destan_u64 size) const
    {
        int category_index = static_cast<int>(category);

        // Check if adding this size would exceed the category budget
        destan_u64 category_used = m_category_memory_used[category_index];
        destan_u64 category_budget = m_category_memory_budget[category_index];

        return (category_used + size <= category_budget);
    }

    // Update memory usage statistics
    void Streaming_Allocator::Update_Memory_Usage(Resource_Category category, destan_i64 size_delta)
    {
        int category_index = static_cast<int>(category);

        // Update category memory usage
        if (size_delta > 0)
        {
            m_category_memory_used[category_index] += size_delta;
        }
        else
        {
            // Ensure we don't underflow
            destan_u64 abs_delta = static_cast<destan_u64>(-size_delta);
            if (abs_delta > m_category_memory_used[category_index])
            {
                m_category_memory_used[category_index] = 0;
            }
            else
            {
                m_category_memory_used[category_index] -= abs_delta;
            }
        }

        // Update total memory usage
        if (size_delta > 0)
        {
            m_stats.total_memory_used += size_delta;
        }
        else
        {
            // Ensure we don't underflow
            destan_u64 abs_delta = static_cast<destan_u64>(-size_delta);
            if (abs_delta > m_stats.total_memory_used)
            {
                m_stats.total_memory_used = 0;
            }
            else
            {
                m_stats.total_memory_used -= abs_delta;
            }
        }

        // Update category stats
        m_stats.category_stats[category_index].memory_used = m_category_memory_used[category_index];
    }

    // Get memory budget for a category
    destan_u64 Streaming_Allocator::Get_Memory_Budget(Resource_Category category) const
    {
        return m_category_memory_budget[static_cast<int>(category)];
    }

    // Generate a unique resource ID
    destan_u64 Streaming_Allocator::Generate_Resource_ID()
    {
        // Use atomic increment to ensure thread safety
        return m_next_resource_id.fetch_add(1, std::memory_order_relaxed);
    }

    // Check if a resource is ready to use
    bool Streaming_Allocator::Is_Resource_Ready(const Resource_Entry* entry) const
    {
        if (!entry)
        {
            return false;
        }

        return (entry->info.state == Resource_State::RESIDENT && entry->data != nullptr);
    }

    // Update distances for all resources based on player position
    void Streaming_Allocator::Update_Resource_Distances(float player_x, float player_y, float player_z)
    {
        // In a real implementation, we would use the resource's world position
        // For this example, we'll use a simple formula to simulate distance
        // based on resource ID (just for demonstration purposes)

        for (destan_u64 i = 0; i < m_resource_count; i++)
        {
            Resource_Entry& entry = m_resources[i];

            // In a real implementation, you would get the resource's world position
            // For this example, we'll derive a fake position from the resource ID
            float resource_x = static_cast<float>(entry.info.id % 1000);
            float resource_y = static_cast<float>((entry.info.id / 1000) % 1000);
            float resource_z = static_cast<float>((entry.info.id / 1000000) % 1000);

            // Calculate 3D distance
            float dx = resource_x - player_x;
            float dy = resource_y - player_y;
            float dz = resource_z - player_z;

            entry.distance_from_player = std::sqrt(dx * dx + dy * dy + dz * dz);
        }
    }

    // Check and manage resource lifetimes based on usage
    void Streaming_Allocator::Check_Resource_Lifetimes()
    {
        destan_u64 current_time = GetCurrentTimeMS();

        for (destan_u64 i = 0; i < m_resource_count; i++)
        {
            Resource_Entry& entry = m_resources[i];

            // Skip resources that are not resident or are already being unloaded
            if (entry.info.state != Resource_State::RESIDENT || entry.unloading_scheduled)
            {
                continue;
            }

            // Skip resources with references
            if (entry.info.reference_count > 0)
            {
                continue;
            }

            // Skip resources that should not be auto-unloaded
            if (!entry.info.auto_unload)
            {
                continue;
            }

            // Skip critical resources
            if (entry.info.priority == Resource_Priority::CRITICAL)
            {
                continue;
            }

            // Check if the resource has timed out
            destan_u64 time_since_last_use = current_time - entry.info.last_used_time;
            destan_u64 timeout = m_config.cache_seconds * 1000;  // Convert to milliseconds

            if (time_since_last_use > timeout)
            {
                // Resource has timed out, schedule for unloading
                Schedule_Resource_Unload(&entry);

                DESTAN_LOG_TRACE("Streaming Allocator '{0}': Resource {1} timed out after {2} seconds of inactivity",
                    m_name, entry.info.id, time_since_last_use / 1000);
            }
        }
    }

    // Update the loading queue based on priorities and distances
    void Streaming_Allocator::Update_Loading_Queue()
    {
        // If there are too many pending operations, don't add more
        if (m_pending_operations.size() >= m_config.max_concurrent_operations * 4)
        {
            return;
        }

        // Look for resources that need loading
        for (destan_u64 i = 0; i < m_resource_count; i++)
        {
            Resource_Entry& entry = m_resources[i];

            // Skip resources that are already loading or resident
            if (entry.info.state != Resource_State::UNLOADED ||
                entry.loading_scheduled)
            {
                continue;
            }

            // Prioritize loading based on distance from player and resource priority
            // If the resource is close to the player or high priority, load it
            bool should_load = false;

            switch (entry.info.priority)
            {
            case Resource_Priority::CRITICAL:
                // Always load critical resources
                should_load = true;
                break;

            case Resource_Priority::HIGH:
                // Load high priority resources if close
                should_load = (entry.distance_from_player < 200.0f);
                break;

            case Resource_Priority::MEDIUM:
                // Load medium priority resources if very close
                should_load = (entry.distance_from_player < 100.0f);
                break;

            case Resource_Priority::LOW:
                // Load low priority resources if extremely close
                should_load = (entry.distance_from_player < 50.0f);
                break;

            case Resource_Priority::BACKGROUND:
                // Load background resources only if very few operations pending
                should_load = (entry.distance_from_player < 20.0f &&
                    m_pending_operations.size() < m_config.max_concurrent_operations);
                break;
            }

            if (should_load && Has_Available_Memory(entry.info.category, entry.info.size))
            {
                Schedule_Resource_Load(&entry);
            }
        }

        // Sort pending operations by priority
        std::sort(m_pending_operations.begin(), m_pending_operations.end(),
            [](const IO_Operation& a, const IO_Operation& b) {
                // Higher priority comes first
                return static_cast<int>(a.priority) < static_cast<int>(b.priority);
            });

        // Limit the number of pending operations
        if (m_pending_operations.size() > m_config.max_concurrent_operations * 4)
        {
            // Keep only the highest priority operations
            destan_u64 operations_to_keep = m_config.max_concurrent_operations * 4;
            m_pending_operations.resize(operations_to_keep);

            DESTAN_LOG_TRACE("Streaming Allocator '{0}': Trimmed pending operations to {1}",
                m_name, operations_to_keep);
        }
    }

}