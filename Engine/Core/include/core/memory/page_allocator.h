#pragma once
#include <core/memory/memory.h>
#include <core/defines.h>

namespace ds::core::memory
{
    /**
     * Memory page protection modes
     * These determine how the allocated pages can be accessed
     */
    enum class Page_Protection
    {
        READ_ONLY,      // Pages can only be read from
        READ_WRITE,     // Pages can be read from and written to
        READ_WRITE_EXEC, // Pages can be read from, written to, and executed (for dynamic code)
        NO_ACCESS       // Pages cannot be accessed (useful for guard pages)
    };

    /**
     * Page allocation flags
     * These control the behavior of page allocation
     */
    enum class Page_Flags
    {
        NONE = 0,
        COMMIT = 1 << 0,         // Commit physical memory immediately (otherwise pages are just reserved)
        GUARD = 1 << 1,          // Add guard pages to detect overruns/underruns
        ZERO = 1 << 2,           // Zero the memory after allocation
        LARGE_PAGES = 1 << 3,    // Use large pages if supported by the OS (better TLB efficiency)
        MAP_FILE = 1 << 4,       // Map a file into memory
        PERSISTENT = 1 << 5,     // Memory should persist even when not actively used
        SHARED = 1 << 6          // Memory can be shared between processes
    };

    // Allow flags to be combined using bitwise operators
    inline Page_Flags operator|(Page_Flags a, Page_Flags b)
    {
        return static_cast<Page_Flags>(static_cast<ds_u32>(a) | static_cast<ds_u32>(b));
    }

    inline Page_Flags operator&(Page_Flags a, Page_Flags b)
    {
        return static_cast<Page_Flags>(static_cast<ds_u32>(a) & static_cast<ds_u32>(b));
    }

    inline bool Has_Flag(Page_Flags flags, Page_Flags flag)
    {
        return (static_cast<ds_u32>(flags) & static_cast<ds_u32>(flag)) != 0;
    }

    /**
     * Page Allocator
     *
     * A memory allocator that manages memory in fixed-size pages.
     * Pages are the fundamental unit of memory allocation in modern operating systems,
     * and this allocator provides an efficient interface for page-level management.
     *
     * Key characteristics:
     * - Allocates memory in page-aligned, fixed-size chunks
     * - Supports memory protection modes (read, write, execute)
     * - Can map files directly into memory
     * - Works well with virtual memory systems
     * - Provides efficient memory-mapped I/O
     * - Supports large pages for better performance with large memory regions
     */
    class Page_Allocator
    {
    public:
        /**
         * Page info structure used to track allocated pages
         */
        struct Page_Info
        {
            void* base_address;              // Base address of the page
            ds_u64 size;                 // Size of the allocation in bytes
            ds_u64 page_count;           // Number of pages in this allocation
            Page_Protection protection;      // Protection mode
            Page_Flags flags;                // Allocation flags
            const ds_char* file_path;    // File path if this is a memory-mapped file (nullptr otherwise)

#ifdef DS_DEBUG
            const ds_char* allocation_file; // Source file where allocation occurred
            ds_i32 allocation_line;         // Source line where allocation occurred
            ds_u64 allocation_id;           // Unique ID for this allocation
#endif
        };

        /**
         * Creates a page allocator with specified parameters
         *
         * @param page_size Size of each page in bytes (must be a multiple of system page size)
         * @param reserve_address_space_size Size of virtual address space to reserve (0 = no prereservation)
         * @param name Optional name for debugging purposes
         */
        Page_Allocator(ds_u64 page_size = 0, ds_u64 reserve_address_space_size = 0,
            const ds_char* name = "Page_Allocator");

        /**
         * Destructor - releases all allocated pages
         */
        ~Page_Allocator();

        // Disable copying to prevent double-freeing
        Page_Allocator(const Page_Allocator&) = delete;
        Page_Allocator& operator=(const Page_Allocator&) = delete;

        // Move operations
        Page_Allocator(Page_Allocator&& other) noexcept;
        Page_Allocator& operator=(Page_Allocator&& other) noexcept;

        /**
         * Allocates a block of memory in page-aligned chunks
         *
         * @param size Size of the allocation in bytes (will be rounded up to page size)
         * @param protection Memory protection mode
         * @param flags Allocation flags
         * @param file_path Path to file for memory-mapped allocations (nullptr for regular allocations)
         * @param file_offset Offset within the file to start mapping from
         * @return Pointer to the allocated memory, or nullptr if allocation failed
         */
        void* Allocate(ds_u64 size, Page_Protection protection = Page_Protection::READ_WRITE,
            Page_Flags flags = Page_Flags::COMMIT | Page_Flags::ZERO,
            const ds_char* file_path = nullptr, ds_u64 file_offset = 0);

        /**
         * Allocates and constructs an object using page allocation
         *
         * @tparam T Type of object to construct
         * @tparam Args Constructor argument types
         * @param protection Memory protection mode
         * @param flags Allocation flags
         * @param args Arguments to forward to the constructor
         * @return Pointer to the constructed object, or nullptr if allocation failed
         */
        template<typename T, typename... Args>
        T* Create(Page_Protection protection, Page_Flags flags, Args&&... args)
        {
            // Allocate a single page for the object
            void* memory = Allocate(sizeof(T), protection, flags);
            if (!memory)
            {
                return nullptr;
            }

            // Construct the object
            return new(memory) T(std::forward<Args>(args)...);
        }

        /**
         * Deallocates a previously allocated page block
         *
         * @param ptr Pointer to the memory to deallocate
         * @return True if deallocation was successful, false otherwise
         */
        bool Deallocate(void* ptr);

        /**
         * Destroys an object and deallocates its memory
         *
         * @tparam T Type of object to destroy
         * @param ptr Pointer to the object
         * @return True if deallocation was successful, false otherwise
         */
        template<typename T>
        bool Destroy(T* ptr)
        {
            if (!ptr)
            {
                return false;
            }

            // Call the destructor
            ptr->~T();

            // Deallocate the memory
            return Deallocate(ptr);
        }

        /**
         * Changes the protection mode of an allocated page block
         *
         * @param ptr Pointer to the memory to protect
         * @param protection New protection mode
         * @return True if protection was changed successfully, false otherwise
         */
        bool Protect(void* ptr, Page_Protection protection);

        /**
         * Commits previously reserved pages, allocating physical memory
         *
         * @param ptr Pointer to the memory to commit
         * @param size Size of the region to commit (must be within the original allocation)
         * @return True if commit was successful, false otherwise
         */
        bool Commit(void* ptr, ds_u64 size);

        /**
         * Decommits pages, releasing physical memory but keeping virtual address space
         *
         * @param ptr Pointer to the memory to decommit
         * @param size Size of the region to decommit (must be within the original allocation)
         * @return True if decommit was successful, false otherwise
         */
        bool Decommit(void* ptr, ds_u64 size);

        /**
         * Flushes changes made to a memory-mapped file back to disk
         *
         * @param ptr Pointer to the memory to flush
         * @param size Size of the region to flush (must be within the original allocation)
         * @return True if flush was successful, false otherwise
         */
        bool Flush(void* ptr, ds_u64 size);

        /**
         * Returns information about an allocated page block
         *
         * @param ptr Pointer to query
         * @return Page_Info structure if ptr is valid, otherwise nullptr
         */
        const Page_Info* Get_Page_Info(void* ptr) const;

        /**
         * Checks if a pointer is within a page allocated by this allocator
         *
         * @param ptr Pointer to check
         * @return True if the pointer is within an allocated page, false otherwise
         */
        bool Is_Allocated(void* ptr) const;

        /**
         * Gets the system's page size
         *
         * @return System page size in bytes
         */
        static ds_u64 Get_System_Page_Size();

        /**
         * Gets the large page size if supported by the system
         *
         * @return Large page size in bytes, or 0 if not supported
         */
        static ds_u64 Get_Large_Page_Size();

        /**
         * Gets the allocator's page size
         *
         * @return Page size in bytes
         */
        ds_u64 Get_Page_Size() const { return m_page_size; }

        /**
         * Gets the total number of pages currently allocated
         *
         * @return Number of allocated pages
         */
        ds_u64 Get_Allocated_Page_Count() const { return m_allocated_page_count; }

        /**
         * Gets the total memory currently allocated by this allocator
         *
         * @return Total allocated memory in bytes
         */
        ds_u64 Get_Total_Allocated() const { return m_allocated_page_count * m_page_size; }

        /**
         * Gets the name of this allocator
         *
         * @return Allocator name
         */
        const ds_char* Get_Name() const { return m_name; }

#ifdef DS_DEBUG
        /**
         * Debug version of Allocate that tracks the allocation
         */
        void* Allocate_Debug(ds_u64 size, Page_Protection protection, Page_Flags flags,
            const ds_char* file_path, ds_u64 file_offset,
            const ds_char* allocation_file, ds_i32 allocation_line);

        /**
         * Dumps statistics and information about all allocated pages
         */
        void Dump_Stats();
#endif

    private:
        // Helper methods
        void Initialize();
        bool Initialize_Memory_Mapping();
        void* Reserve_Address_Space(ds_u64 size, ds_u64 alignment);
        void Release_Address_Space(void* ptr, ds_u64 size);
        void* Map_File_To_Memory(const ds_char* file_path, ds_u64 file_offset,
            ds_u64& size, Page_Protection protection);

        // Private helper to convert our protection enum to system-specific flags
        static ds_u32 Convert_Protection_Flags(Page_Protection protection);

        // Memory tracking
        static constexpr ds_u64 MAX_PAGE_ALLOCATIONS = 1024; // Maximum number of tracked allocations
        Page_Info m_page_infos[MAX_PAGE_ALLOCATIONS];            // Array of page allocation info
        ds_u64 m_page_info_count = 0;                        // Number of tracked allocations

        // Allocator properties
        ds_u64 m_page_size = 0;                              // Size of each page in bytes
        ds_u64 m_system_page_size = 0;                       // Native OS page size
        ds_u64 m_large_page_size = 0;                        // Large page size (if supported)
        ds_u64 m_allocated_page_count = 0;                   // Number of pages allocated

        // Pre-reserved address space (optional)
        void* m_reserved_address_space = nullptr;                // Pre-reserved address space
        ds_u64 m_reserved_address_space_size = 0;            // Size of pre-reserved address space
        ds_u64 m_reserved_address_space_used = 0;            // Amount of pre-reserved space used

        // Fixed-size buffer for name
        static constexpr ds_u64 MAX_NAME_LENGTH = 64;
        ds_char m_name[MAX_NAME_LENGTH];

#ifdef DS_DEBUG
        // Debug tracking
        static std::atomic<ds_u64> s_next_allocation_id;
        std::mutex m_mutex;  // For thread safety in debug builds
#endif

        // Thread safety helper methods
        void Lock()
        {
#ifdef DS_DEBUG
            m_mutex.lock();
#endif
        }

        void Unlock()
        {
#ifdef DS_DEBUG
            m_mutex.unlock();
#endif
        }
    };

    // Helper macros for page allocator
#ifdef DS_DEBUG
#define DS_PAGE_ALLOC(allocator, size, protection, flags, file_path, file_offset) \
        (allocator)->Allocate_Debug((size), (protection), (flags), (file_path), (file_offset), __FILE__, __LINE__)
#else
#define DS_PAGE_ALLOC(allocator, size, protection, flags, file_path, file_offset) \
        (allocator)->Allocate((size), (protection), (flags), (file_path), (file_offset))
#endif

#define DS_PAGE_CREATE(allocator, type, protection, flags, ...) \
        (allocator)->Create<type>((protection), (flags), __VA_ARGS__)

#define DS_PAGE_DESTROY(allocator, ptr) \
        (allocator)->Destroy(ptr)

} // namespace ds::core::memory