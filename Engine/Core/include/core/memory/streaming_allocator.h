#pragma once
#include <core/memory/page_allocator.h>
#include <core/defines.h>

namespace ds::core::memory
{
    /**
     * Resource priority levels
     * These define how important a resource is, which affects when it gets loaded or unloaded
     */
    enum class Resource_Priority
    {
        CRITICAL,   // Never unloaded once loaded (e.g., player character, essential UI)
        HIGH,       // Unloaded only when absolutely necessary (e.g., current level geometry)
        MEDIUM,     // Standard priority (e.g., textures for nearby objects)
        LOW,        // Unloaded first when memory is needed (e.g., distant objects)
        BACKGROUND  // Loaded only when there's spare memory and CPU time (e.g., pre-caching)
    };

    /**
     * Resource state in the streaming system
     */
    enum class Resource_State
    {
        UNLOADED,       // Not loaded
        LOADING,        // Currently being loaded
        RESIDENT,       // Fully loaded and ready to use
        UNLOADING,      // Currently being unloaded
        FAILED          // Failed to load
    };

    /**
     * Resource loading callback function type
     * Called when a resource has finished loading
     *
     * @param resource_id The ID of the resource that finished loading
     * @param data Pointer to the loaded resource data
     * @param size Size of the loaded resource data
     * @param user_data User-provided data passed to the loading request
     */
    using Resource_Loaded_Callback =
        void (*)(ds_u64 resource_id, void* data, ds_u64 size, void* user_data);

    /**
     * Resource handle for referring to managed resources
     */
    struct Resource_Handle
    {
        ds_u64 id = 0;            // Unique identifier for the resource
        Resource_State state = Resource_State::UNLOADED;  // Current state

        bool IsValid() const { return id != 0; }
        bool IsReady() const { return state == Resource_State::RESIDENT; }
    };

    /**
     * Streaming Allocator
     *
     * A high-level memory system that manages dynamic loading and unloading of resources.
     * Ideal for game worlds where assets need to be streamed in and out as the player moves.
     *
     * Key characteristics:
     * - Asynchronous loading and unloading of resources
     * - Priority-based memory management
     * - Predictive loading based on usage patterns
     * - Configurable memory budgets for different resource types
     * - Distance-based resource management for open worlds
     */
    class Streaming_Allocator
    {
    public:
        /**
         * Resource access options
         */
        enum class Access_Mode
        {
            READ_ONLY,        // Resource can only be read
            READ_WRITE,       // Resource can be read and written
            PERSISTENT_WRITE  // Changes to resource are saved back to storage
        };

        /**
         * Resource category
         * Used to partition memory budgets for different resource types
         */
        enum class Resource_Category
        {
            GEOMETRY,   // 3D models, meshes
            TEXTURE,    // Textures, materials
            AUDIO,      // Sound effects, music
            ANIMATION,  // Animation data
            SCRIPT,     // Scripts, behavior data
            GENERIC     // Miscellaneous data
        };

        /**
         * Configuration structure for streaming allocator
         */
        struct Config
        {
            ds_u64 total_memory_budget = 256 * 1024 * 1024;  // Total memory budget (default: 256MB)
            ds_u64 page_size = 64 * 1024;                    // Page size (default: 64KB)
            ds_u32 max_concurrent_operations = 4;            // Maximum number of concurrent I/O operations
            ds_u32 cache_seconds = 60 * 60;                  // How long to keep unused resources (in seconds)
            ds_bool enable_predictive_loading = true;        // Whether to use predictive loading
            ds_bool log_detailed_stats = false;              // Whether to log detailed memory stats

            // Memory budget percentages for each category (must add up to 100)
            ds_u32 geometry_budget_percent = 30;
            ds_u32 texture_budget_percent = 50;
            ds_u32 audio_budget_percent = 10;
            ds_u32 animation_budget_percent = 5;
            ds_u32 script_budget_percent = 2;
            ds_u32 generic_budget_percent = 3;
        };

        /**
         * Resource request information
         */
        struct Resource_Request
        {
            ds_u64 resource_id;                          // Unique identifier for the resource
            const ds_char* path;                         // Path to the resource
            Resource_Category category;                      // Resource category
            Resource_Priority priority;                      // Resource priority
            Access_Mode access_mode;                         // How the resource will be accessed
            Resource_Loaded_Callback callback;               // Callback when resource is loaded
            void* user_data;                                 // User data passed to callback
            ds_bool auto_unload;                         // Whether to automatically unload
            ds_u64 estimated_size;                       // Estimated size (used for preallocation)
        };

        /**
         * Resource information
         */
        struct Resource_Info
        {
            ds_u64 id;                                 // Unique identifier
            ds_char path[256];                         // Path to the resource
            Resource_Category category;                    // Resource category
            Resource_Priority priority;                    // Resource priority
            Resource_State state;                          // Current state
            ds_u64 size;                               // Size in bytes
            ds_u64 last_used_time;                     // Last time the resource was accessed
            ds_bool auto_unload;                       // Whether to automatically unload
            ds_u32 reference_count;                    // Number of active references
        };

        /**
         * Statistics about resource usage
         */
        struct Stats
        {
            ds_u64 total_memory_used;                   // Total memory used by all resources
            ds_u64 total_memory_budget;                 // Total memory budget

            // Per-category statistics
            struct Category_Stats
            {
                ds_u64 memory_used;                     // Memory used for this category
                ds_u64 memory_budget;                   // Memory budget for this category
                ds_u64 resource_count;                  // Number of resources in this category
            };

            Category_Stats category_stats[6];               // Stats for each category

            ds_u64 resource_count;                      // Total number of managed resources
            ds_u32 loading_count;                       // Number of resources currently loading
            ds_u32 failed_count;                        // Number of resources that failed to load
            ds_u64 bytes_loaded;                        // Total bytes loaded in this session
            ds_u64 bytes_unloaded;                      // Total bytes unloaded in this session
            ds_u32 load_operations;                     // Total load operations
            ds_u32 unload_operations;                   // Total unload operations
        };

        /**
         * Creates a streaming allocator with the specified configuration
         *
         * @param config Configuration for the streaming allocator
         * @param name Optional name for debugging purposes
         */
        Streaming_Allocator(const Config& config = Config(), const ds_char* name = "Streaming_Allocator");

        /**
         * Destructor - releases all resources
         */
        ~Streaming_Allocator();

        // Disable copying to prevent double-freeing
        Streaming_Allocator(const Streaming_Allocator&) = delete;
        Streaming_Allocator& operator=(const Streaming_Allocator&) = delete;

        // Move operations
        Streaming_Allocator(Streaming_Allocator&& other) noexcept;
        Streaming_Allocator& operator=(Streaming_Allocator&& other) noexcept;

        /**
         * Requests a resource to be loaded
         *
         * @param request Information about the resource to load
         * @return Handle to the requested resource
         */
        Resource_Handle Request_Resource(const Resource_Request& request);

        /**
         * Prefetches a resource (loads it with background priority)
         *
         * @param path Path to the resource
         * @param category Resource category
         * @return Handle to the prefetched resource
         */
        Resource_Handle Prefetch_Resource(const ds_char* path, Resource_Category category = Resource_Category::GENERIC);

        /**
         * Accesses a loaded resource
         *
         * @param handle Handle to the resource
         * @return Pointer to the resource data, or nullptr if not loaded
         */
        void* Access_Resource(Resource_Handle handle);

        /**
         * Marks a resource as being actively used
         * This prevents it from being automatically unloaded
         *
         * @param handle Handle to the resource
         * @return True if the resource was referenced successfully
         */
        bool Reference_Resource(Resource_Handle handle);

        /**
         * Releases a reference to a resource
         * When the reference count reaches zero, the resource becomes eligible for automatic unloading
         *
         * @param handle Handle to the resource
         * @return True if the resource was released successfully
         */
        bool Release_Resource(Resource_Handle handle);

        /**
         * Prefetches resources near the specified position
         * Useful for predictive loading based on player position
         *
         * @param position_x X coordinate of position
         * @param position_y Y coordinate of position
         * @param position_z Z coordinate of position (optional)
         * @param radius Radius around the position to prefetch
         * @param category Resource category to prefetch (optional, all categories if not specified)
         */
        void Prefetch_Resources_At_Position(float position_x, float position_y, float position_z = 0.0f,
            float radius = 100.0f,
            Resource_Category category = Resource_Category::GENERIC);

        /**
         * Sets the priority of a resource
         *
         * @param handle Handle to the resource
         * @param priority New priority for the resource
         * @return True if the priority was changed successfully
         */
        bool Set_Resource_Priority(Resource_Handle handle, Resource_Priority priority);

        /**
         * Immediately unloads a resource
         * This is useful for explicit resource management when needed
         *
         * @param handle Handle to the resource
         * @return True if the resource was unloaded successfully
         */
        bool Unload_Resource(Resource_Handle handle);

        /**
         * Flushes all changes to memory-mapped resources to disk
         */
        void Flush_Resources();

        /**
         * Updates the streaming system
         * This should be called regularly (e.g., once per frame)
         *
         * @param delta_time Time since last update (in seconds)
         */
        void Update(float delta_time);

        /**
         * Gets information about a resource
         *
         * @param handle Handle to the resource
         * @return Resource information, or nullptr if not found
         */
        const Resource_Info* Get_Resource_Info(Resource_Handle handle);

        /**
         * Gets statistics about resource usage
         *
         * @return Statistics structure
         */
        Stats Get_Stats();

        /**
         * Clears all non-critical resources
         * Useful when transitioning between game areas or levels
         */
        void Clear_Non_Critical_Resources();

        /**
         * Gets the name of this allocator
         *
         * @return Allocator name
         */
        const ds_char* Get_Name() const { return m_name; }

    private:
        // Resource entry with additional internal information
        struct Resource_Entry
        {
            Resource_Info info;                      // Public resource information
            void* data;                              // Pointer to the resource data
            Resource_Loaded_Callback callback;       // Callback for when resource is loaded
            void* user_data;                         // User data for callback
            Access_Mode access_mode;                 // How the resource is accessed
            float distance_from_player;              // Current distance from player
            bool loading_scheduled;                  // Whether loading has been scheduled
            bool unloading_scheduled;                // Whether unloading has been scheduled
        };

        // IO operation information
        struct IO_Operation
        {
            enum Type { LOAD, UNLOAD };
            Type type;                               // Operation type
            ds_u64 resource_id;                  // Resource ID
            const ds_char* path;                 // File path (for loads)
            Resource_Priority priority;              // Priority of this operation
        };

        // Helper methods
        Resource_Entry* Find_Resource_Entry(ds_u64 resource_id);
        const Resource_Entry* Find_Resource_Entry(ds_u64 resource_id) const;
        Resource_Entry* Find_Resource_Entry(Resource_Handle handle);
        const Resource_Entry* Find_Resource_Entry(Resource_Handle handle) const;

        void Process_IO_Operations();
        void Schedule_Resource_Load(Resource_Entry* entry);
        void Schedule_Resource_Unload(Resource_Entry* entry);
        void Execute_Resource_Load(const IO_Operation& operation);
        void Execute_Resource_Unload(const IO_Operation& operation);

        bool Has_Available_Memory(Resource_Category category, ds_u64 size) const;
        void Update_Memory_Usage(Resource_Category category, ds_i64 size_delta);
        ds_u64 Get_Memory_Budget(Resource_Category category) const;

        ds_u64 Generate_Resource_ID();
        bool Is_Resource_Ready(const Resource_Entry* entry) const;

        void Update_Resource_Distances(float player_x, float player_y, float player_z);
        void Check_Resource_Lifetimes();
        void Update_Loading_Queue();

        // Config values
        Config m_config;

        // Memory tracking
        ds_u64 m_category_memory_used[6] = { 0 };   // Memory used per category
        ds_u64 m_category_memory_budget[6] = { 0 }; // Memory budget per category

        // Resource tracking
        static constexpr ds_u64 MAX_RESOURCES = 8192;
        Resource_Entry m_resources[MAX_RESOURCES];
        ds_u64 m_resource_count = 0;

        // IO operation queues
        std::vector<IO_Operation> m_pending_operations;
        std::vector<IO_Operation> m_active_operations;

        // Current player position for distance-based streaming
        float m_player_x = 0.0f;
        float m_player_y = 0.0f;
        float m_player_z = 0.0f;

        // Page allocator for memory management
        Page_Allocator m_page_allocator;

        // Statistics
        Stats m_stats;

        // Timing
        ds_u64 m_last_update_time = 0;

        // Resource ID counter
        std::atomic<ds_u64> m_next_resource_id{ 1 };

        // Thread synchronization
        std::mutex m_mutex;

        // Fixed-size buffer for name
        static constexpr ds_u64 MAX_NAME_LENGTH = 64;
        ds_char m_name[MAX_NAME_LENGTH];
    };

} // namespace ds::core::memory