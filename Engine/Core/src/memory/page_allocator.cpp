#include <core/ds_pch.h>
#include <core/memory/page_allocator.h>

namespace ds::core::memory
{
#ifdef DS_DEBUG
    std::atomic<ds_u64> Page_Allocator::s_next_allocation_id{ 1 };
#endif

    Page_Allocator::Page_Allocator(ds_u64 page_size, ds_u64 reserve_address_space_size, const ds_char* name)
        : m_reserved_address_space_size(reserve_address_space_size)
    {
        // Safely copy the name to our fixed buffer
        if (name)
        {
            ds_u64 name_length = 0;
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
            const char* default_name = "Page_Allocator";
            ds_u64 i = 0;
            while (default_name[i] && i < MAX_NAME_LENGTH - 1)
            {
                m_name[i] = default_name[i];
                i++;
            }
            m_name[i] = '\0';
        }

        // Initialize the allocator
        Initialize();

        // If page_size is 0, use the system page size
        if (page_size == 0)
        {
            m_page_size = m_system_page_size;
        }
        else
        {
            // Ensure page size is a multiple of system page size
            m_page_size = Memory::Align_Size(page_size, m_system_page_size);
        }

        // Pre-reserve address space if requested
        if (m_reserved_address_space_size > 0)
        {
            m_reserved_address_space = Reserve_Address_Space(m_reserved_address_space_size, m_page_size);
            if (!m_reserved_address_space)
            {
                DS_LOG_ERROR("Page Allocator '{0}': Failed to reserve {1} bytes of address space",
                    m_name, m_reserved_address_space_size);
                m_reserved_address_space_size = 0;
            }
            else
            {
                DS_LOG_INFO("Page Allocator '{0}': Reserved {1} MB of address space at {2}",
                    m_name, m_reserved_address_space_size / (1024 * 1024), m_reserved_address_space);
            }
        }

        DS_LOG_INFO("Page Allocator '{0}' created with page size {1} KB",
            m_name, m_page_size / 1024);
    }

    Page_Allocator::~Page_Allocator()
    {
        // First, release all allocated pages
        for (ds_u64 i = 0; i < m_page_info_count; i++)
        {
            Page_Info& info = m_page_infos[i];
            if (info.base_address)
            {
                // For memory-mapped files, we need to unmap them
                if (info.file_path)
                {
#ifdef DS_PLATFORM_WINDOWS
                    UnmapViewOfFile(info.base_address);
#else
                    munmap(info.base_address, info.size);
#endif
                }
                else
                {
                    // Regular memory pages
                    Release_Address_Space(info.base_address, info.size);
                }
                info.base_address = nullptr;
            }
        }

        // Then release the pre-reserved address space if any
        if (m_reserved_address_space)
        {
            Release_Address_Space(m_reserved_address_space, m_reserved_address_space_size);
            m_reserved_address_space = nullptr;
            m_reserved_address_space_size = 0;
            m_reserved_address_space_used = 0;
        }

        DS_LOG_INFO("Page Allocator '{0}' destroyed", m_name);
    }

    Page_Allocator::Page_Allocator(Page_Allocator&& other) noexcept
        : m_page_size(other.m_page_size)
        , m_system_page_size(other.m_system_page_size)
        , m_large_page_size(other.m_large_page_size)
        , m_allocated_page_count(other.m_allocated_page_count)
        , m_reserved_address_space(other.m_reserved_address_space)
        , m_reserved_address_space_size(other.m_reserved_address_space_size)
        , m_reserved_address_space_used(other.m_reserved_address_space_used)
        , m_page_info_count(other.m_page_info_count)
    {
        // Copy name
        Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

        // Copy page infos
        for (ds_u64 i = 0; i < m_page_info_count; i++)
        {
            m_page_infos[i] = other.m_page_infos[i];
        }

        // Clear other's resources
        other.m_reserved_address_space = nullptr;
        other.m_reserved_address_space_size = 0;
        other.m_reserved_address_space_used = 0;
        other.m_allocated_page_count = 0;
        other.m_page_info_count = 0;
    }

    Page_Allocator& Page_Allocator::operator=(Page_Allocator&& other) noexcept
    {
        if (this != &other)
        {
            // Call destructor to clean up our resources
            this->~Page_Allocator();

            // Copy properties
            m_page_size = other.m_page_size;
            m_system_page_size = other.m_system_page_size;
            m_large_page_size = other.m_large_page_size;
            m_allocated_page_count = other.m_allocated_page_count;
            m_reserved_address_space = other.m_reserved_address_space;
            m_reserved_address_space_size = other.m_reserved_address_space_size;
            m_reserved_address_space_used = other.m_reserved_address_space_used;
            m_page_info_count = other.m_page_info_count;

            // Copy name
            Memory::Memcpy(m_name, other.m_name, MAX_NAME_LENGTH);

            // Copy page infos
            for (ds_u64 i = 0; i < m_page_info_count; i++)
            {
                m_page_infos[i] = other.m_page_infos[i];
            }

            // Clear other's resources
            other.m_reserved_address_space = nullptr;
            other.m_reserved_address_space_size = 0;
            other.m_reserved_address_space_used = 0;
            other.m_allocated_page_count = 0;
            other.m_page_info_count = 0;
        }
        return *this;
    }

    void Page_Allocator::Initialize()
    {
        // Get the system page size
#ifdef DS_PLATFORM_WINDOWS
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        m_system_page_size = sys_info.dwPageSize;

        // Large pages require special privileges on Windows
        HANDLE token;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        {
            TOKEN_PRIVILEGES tp;
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (LookupPrivilegeValue(nullptr, SE_LOCK_MEMORY_NAME, &tp.Privileges[0].Luid))
            {
                if (AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
                {
                    // If privileges were adjusted, try to get large page size
                    m_large_page_size = GetLargePageMinimum();
                }
            }
            CloseHandle(token);
        }
#else
        m_system_page_size = sysconf(_SC_PAGESIZE);

        // On Linux/macOS, large pages are typically 2MB or 1GB
        // Here we're assuming 2MB large pages, but this should be detected at runtime
        m_large_page_size = 2 * 1024 * 1024;
#endif

        // Initialize memory mapping subsystem
        Initialize_Memory_Mapping();

        // Clear page info array
        Memory::Memset(m_page_infos, 0, sizeof(m_page_infos));
    }

    bool Page_Allocator::Initialize_Memory_Mapping()
    {
        // Initialize platform-specific memory mapping
#ifdef DS_PLATFORM_WINDOWS
        // Nothing special needed for Windows
        return true;
#elif defined(DS_PLATFORM_LINUX) || defined(DS_PLATFORM_MACOS)
        // Nothing special needed for Unix-like systems
        return true;
#else
        DS_LOG_ERROR("Page Allocator: Platform not supported");
        return false;
#endif
    }

    void* Page_Allocator::Reserve_Address_Space(ds_u64 size, ds_u64 alignment)
    {
        // Align size to page size
        size = Memory::Align_Size(size, m_system_page_size);

#ifdef DS_PLATFORM_WINDOWS
        // On Windows, VirtualAlloc with MEM_RESERVE reserves address space without committing physical memory
        return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
#else
        // On Unix-like systems, use mmap with PROT_NONE to reserve address space
        void* ptr = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED)
        {
            return nullptr;
        }
        return ptr;
#endif
    }

    void Page_Allocator::Release_Address_Space(void* ptr, ds_u64 size)
    {
        if (!ptr) return;

#ifdef DS_PLATFORM_WINDOWS
        // On Windows, VirtualFree with MEM_RELEASE releases address space
        VirtualFree(ptr, 0, MEM_RELEASE);
#else
        // On Unix-like systems, use munmap to release address space
        munmap(ptr, size);
#endif
    }

    void* Page_Allocator::Map_File_To_Memory(const ds_char* file_path, ds_u64 file_offset,
        ds_u64& size, Page_Protection protection)  // Note: size is now a reference
    {
        if (!file_path || size == 0) return nullptr;

#ifdef DS_PLATFORM_WINDOWS
        // Open the file
        HANDLE file_handle = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_handle == INVALID_HANDLE_VALUE)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to open file {1}", m_name, file_path);
            return nullptr;
        }

        // TODO_EREN: In the future, I need to implement headers for resources that are going to be
        // used by my engine so that I will know their flags, sizes and other info
        // I plan something like this:
        //    [4 bytes] - Magic identifier "DEST"
        //    [8 bytes] - Total file size(including header)
        //    [8 bytes] - Payload size
        //    [8 bytes] - Version
        //    [8 bytes] - Resource type
        //    [8 bytes] - Flags
        //    [...] - Resource data
        // I will return here back after finishing my resource system
        // The system will first try to get resource size from the header but If
        // it cannot get the header, it will check its size and then try mapping it

        // Get the actual file size
        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(file_handle, &file_size))
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to get file size for {1}", m_name, file_path);
            CloseHandle(file_handle);
            return nullptr;
        }

        // Adjust size if needed
        ds_u64 actual_file_size = static_cast<ds_u64>(file_size.QuadPart);
        if (size > actual_file_size)
        {
            DS_LOG_WARN("Page Allocator '{0}': Requested size {1} KB exceeds file size {2} KB for {3}",
                m_name, size / 1024, actual_file_size / 1024, file_path);
            size = actual_file_size;  // Update the reference parameter
        }

        // Create a file mapping object with the adjusted size
        HANDLE mapping_handle = CreateFileMappingA(file_handle, nullptr,
            Convert_Protection_Flags(protection),
            (DWORD)((size + file_offset) >> 32),
            (DWORD)((size + file_offset) & 0xFFFFFFFF),
            nullptr);
        if (!mapping_handle)
        {
            CloseHandle(file_handle);
            DS_LOG_ERROR("Page Allocator '{0}': Failed to create file mapping for {1}", m_name, file_path);
            return nullptr;
        }

        // Map the file into memory
        DWORD access_flags = 0;
        switch (protection)
        {
        case Page_Protection::READ_ONLY:
            access_flags = FILE_MAP_READ;
            break;
        case Page_Protection::READ_WRITE:
            access_flags = FILE_MAP_READ | FILE_MAP_WRITE;
            break;
        case Page_Protection::READ_WRITE_EXEC:
            access_flags = FILE_MAP_READ | FILE_MAP_WRITE | FILE_MAP_EXECUTE;
            break;
        default:
            access_flags = FILE_MAP_READ;
            break;
        }

        void* mapped_address = MapViewOfFile(mapping_handle, access_flags,
            (DWORD)(file_offset >> 32),
            (DWORD)(file_offset & 0xFFFFFFFF),
            size);

        // Close the handles (the mapping remains valid)
        CloseHandle(mapping_handle);
        CloseHandle(file_handle);

        if (!mapped_address)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to map file {1} into memory", m_name, file_path);
            return nullptr;
        }

        return mapped_address;
#else
        // Open the file
        int fd = open(file_path, O_RDWR);
        if (fd == -1)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to open file {1}", m_name, file_path);
            return nullptr;
        }

        // Get the actual file size
        struct stat file_stat;
        if (fstat(fd, &file_stat) == -1)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to get file size for {1}", m_name, file_path);
            close(fd);
            return nullptr;
        }

        // Adjust size if needed
        ds_u64 actual_file_size = static_cast<ds_u64>(file_stat.st_size);
        if (size > actual_file_size)
        {
            DS_LOG_WARN("Page Allocator '{0}': Requested size {1} KB exceeds file size {2} KB for {3}",
                m_name, size / 1024, actual_file_size / 1024, file_path);
            size = actual_file_size;  // Update the reference parameter
        }

        // Determine protection flags
        int prot_flags = 0;
        switch (protection)
        {
        case Page_Protection::READ_ONLY:
            prot_flags = PROT_READ;
            break;
        case Page_Protection::READ_WRITE:
            prot_flags = PROT_READ | PROT_WRITE;
            break;
        case Page_Protection::READ_WRITE_EXEC:
            prot_flags = PROT_READ | PROT_WRITE | PROT_EXEC;
            break;
        case Page_Protection::NO_ACCESS:
            prot_flags = PROT_NONE;
            break;
        }

        // Map the file into memory with adjusted size
        void* mapped_address = mmap(nullptr, size, prot_flags, MAP_SHARED, fd, file_offset);

        // Close the file descriptor (the mapping remains valid)
        close(fd);

        if (mapped_address == MAP_FAILED)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to map file {1} into memory", m_name, file_path);
            return nullptr;
        }

        return mapped_address;
#endif
    }

    ds_u32 Page_Allocator::Convert_Protection_Flags(Page_Protection protection)
    {
#ifdef DS_PLATFORM_WINDOWS
        switch (protection)
        {
        case Page_Protection::READ_ONLY:
            return PAGE_READONLY;
        case Page_Protection::READ_WRITE:
            return PAGE_READWRITE;
        case Page_Protection::READ_WRITE_EXEC:
            return PAGE_EXECUTE_READWRITE;
        case Page_Protection::NO_ACCESS:
            return PAGE_NOACCESS;
        default:
            return PAGE_READONLY;
        }
#else
        switch (protection)
        {
        case Page_Protection::READ_ONLY:
            return PROT_READ;
        case Page_Protection::READ_WRITE:
            return PROT_READ | PROT_WRITE;
        case Page_Protection::READ_WRITE_EXEC:
            return PROT_READ | PROT_WRITE | PROT_EXEC;
        case Page_Protection::NO_ACCESS:
            return PROT_NONE;
        default:
            return PROT_READ;
        }
#endif
    }

    void* Page_Allocator::Allocate(ds_u64 size, Page_Protection protection, Page_Flags flags,
        const ds_char* file_path, ds_u64 file_offset)
    {
        Lock();

        // Validate allocation size
        if (size == 0)
        {
            DS_LOG_WARN("Page Allocator '{0}': Attempted to allocate 0 bytes", m_name);
            Unlock();
            return nullptr;
        }

        // Round size up to page size
        ds_u64 aligned_size = Memory::Align_Size(size, m_page_size);
        ds_u64 page_count = aligned_size / m_page_size;

        // Check if we have room to track this allocation
        if (m_page_info_count >= MAX_PAGE_ALLOCATIONS)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Maximum number of page allocations ({1}) reached",
                m_name, MAX_PAGE_ALLOCATIONS);
            Unlock();
            return nullptr;
        }

        void* allocation = nullptr;

        // For READ_ONLY pages that need zeroing, we need to allocate as READ_WRITE first
        Page_Protection initial_protection = protection;
        if (protection == Page_Protection::READ_ONLY && Has_Flag(flags, Page_Flags::ZERO) && file_path == nullptr)
        {
            // Temporarily use READ_WRITE for initialization
            initial_protection = Page_Protection::READ_WRITE;
        }

        // Check if this is a file mapping
        if (file_path != nullptr)
        {
            // Memory map the file
            allocation = Map_File_To_Memory(file_path, file_offset, aligned_size, protection);

            // For memory-mapped files, we don't need to zero since they're initialized with file content
        }
        else
        {
            // Determine if we should use the pre-reserved address space
            if (m_reserved_address_space &&
                m_reserved_address_space_used + aligned_size <= m_reserved_address_space_size)
            {
                // Use the next portion of our pre-reserved address space
                ds_u8* base = static_cast<ds_u8*>(m_reserved_address_space) + m_reserved_address_space_used;

                // Commit the memory if requested
                if (Has_Flag(flags, Page_Flags::COMMIT))
                {
#ifdef DS_PLATFORM_WINDOWS
                    // Use initial_protection which might be READ_WRITE for zeroing
                    allocation = VirtualAlloc(base, aligned_size, MEM_COMMIT, Convert_Protection_Flags(initial_protection));
#else
                    // On Unix, we need to change the protection to actually commit the pages
                    if (mprotect(base, aligned_size, Convert_Protection_Flags(initial_protection)) == 0)
                    {
                        allocation = base;
                    }
#endif
                }
                else
                {
                    // Just return the address without committing
                    allocation = base;
                }

                if (allocation)
                {
                    m_reserved_address_space_used += aligned_size;
                }
            }
            else
            {
                // Allocate new pages
#ifdef DS_PLATFORM_WINDOWS
                DWORD alloc_type = MEM_RESERVE;
                if (Has_Flag(flags, Page_Flags::COMMIT))
                {
                    alloc_type |= MEM_COMMIT;
                }
                if (Has_Flag(flags, Page_Flags::LARGE_PAGES) && m_large_page_size > 0)
                {
                    alloc_type |= MEM_LARGE_PAGES;
                }

                // Use initial_protection which might be READ_WRITE for zeroing
                allocation = VirtualAlloc(nullptr, aligned_size, alloc_type, Convert_Protection_Flags(initial_protection));
#else
                int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;

                // Some platforms support large pages via special flags
#if defined(DS_PLATFORM_LINUX) && defined(MAP_HUGETLB)
                if (Has_Flag(flags, Page_Flags::LARGE_PAGES) && m_large_page_size > 0)
                {
                    mmap_flags |= MAP_HUGETLB;
                }
#endif

                allocation = mmap(nullptr, aligned_size, Convert_Protection_Flags(initial_protection), mmap_flags, -1, 0);
                if (allocation == MAP_FAILED)
                {
                    allocation = nullptr;
                }
#endif
            }
        }

        if (!allocation)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to allocate {1} bytes ({2} pages)",
                m_name, aligned_size, page_count);
            Unlock();
            return nullptr;
        }

        // Zero memory if requested (now safe since we're using READ_WRITE if needed)
        if (Has_Flag(flags, Page_Flags::ZERO) && file_path == nullptr)
        {
            Memory::Memset(allocation, 0, aligned_size);
        }

        // Change protection to READ_ONLY if we temporarily used READ_WRITE
        if (initial_protection != protection)
        {
#ifdef DS_PLATFORM_WINDOWS
            DWORD old_protect;
            if (!VirtualProtect(allocation, aligned_size, Convert_Protection_Flags(protection), &old_protect))
            {
                DS_LOG_ERROR("Page Allocator '{0}': Failed to change protection after initialization", m_name);
            }
#else
            if (mprotect(allocation, aligned_size, Convert_Protection_Flags(protection)) != 0)
            {
                DS_LOG_ERROR("Page Allocator '{0}': Failed to change protection after initialization", m_name);
            }
#endif
        }

        // Add guard pages if requested
        if (Has_Flag(flags, Page_Flags::GUARD))
        {
            // Allocate additional guard pages before and after the allocation
#ifdef DS_PLATFORM_WINDOWS
            VirtualAlloc(static_cast<ds_u8*>(allocation) - m_page_size, m_page_size, MEM_COMMIT, PAGE_NOACCESS);
            VirtualAlloc(static_cast<ds_u8*>(allocation) + aligned_size, m_page_size, MEM_COMMIT, PAGE_NOACCESS);
#else
            void* guard_before = static_cast<ds_u8*>(allocation) - m_page_size;
            void* guard_after = static_cast<ds_u8*>(allocation) + aligned_size;

            mprotect(guard_before, m_page_size, PROT_NONE);
            mprotect(guard_after, m_page_size, PROT_NONE);
#endif
        }

        // Record allocation information
        Page_Info& info = m_page_infos[m_page_info_count++];
        info.base_address = allocation;
        info.size = aligned_size;
        info.page_count = page_count;
        info.protection = protection;  // Store the final protection, not the initial one
        info.flags = flags;
        info.file_path = file_path;

#ifdef DS_DEBUG
        info.allocation_id = s_next_allocation_id.fetch_add(1, std::memory_order_relaxed);
        info.allocation_file = nullptr;
        info.allocation_line = 0;
#endif

        // Update stats
        m_allocated_page_count += page_count;

        DS_LOG_TRACE("Page Allocator '{0}': Allocated {1} bytes ({2} pages) at {3}",
            m_name, aligned_size, page_count, allocation);

        Unlock();
        return allocation;
    }

    bool Page_Allocator::Deallocate(void* ptr)
    {
        if (!ptr)
        {
            return false;
        }

        Lock();

        // Find the page info for this allocation
        for (ds_u64 i = 0; i < m_page_info_count; i++)
        {
            if (m_page_infos[i].base_address == ptr)
            {
                Page_Info& info = m_page_infos[i];

                // Handle memory-mapped files
                if (info.file_path)
                {
#ifdef DS_PLATFORM_WINDOWS
                    UnmapViewOfFile(ptr);
#else
                    munmap(ptr, info.size);
#endif
                }
                else
                {
                    // Regular memory pages
#ifdef DS_PLATFORM_WINDOWS
                    VirtualFree(ptr, 0, MEM_RELEASE);
#else
                    munmap(ptr, info.size);
#endif
                }

                // Update stats
                m_allocated_page_count -= info.page_count;

                DS_LOG_TRACE("Page Allocator '{0}': Deallocated {1} bytes ({2} pages) at {3}",
                    m_name, info.size, info.page_count, ptr);

                // Remove this entry by copying the last entry to this position
                if (i < m_page_info_count - 1)
                {
                    m_page_infos[i] = m_page_infos[m_page_info_count - 1];
                }

                // Clear the last entry
                Memory::Memset(&m_page_infos[m_page_info_count - 1], 0, sizeof(Page_Info));
                m_page_info_count--;

                Unlock();
                return true;
            }
        }

        DS_LOG_ERROR("Page Allocator '{0}': Attempted to deallocate unknown address {1}",
            m_name, ptr);

        Unlock();
        return false;
    }

    bool Page_Allocator::Protect(void* ptr, Page_Protection protection)
    {
        if (!ptr)
        {
            return false;
        }

        Lock();

        // Find the page info for this allocation
        const Page_Info* info = Get_Page_Info(ptr);
        if (!info)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Attempted to protect unknown address {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        // Change protection
#ifdef DS_PLATFORM_WINDOWS
        DWORD old_protect;
        bool result = VirtualProtect(ptr, info->size, Convert_Protection_Flags(protection), &old_protect) != 0;
#else
        bool result = mprotect(ptr, info->size, Convert_Protection_Flags(protection)) == 0;
#endif

        if (!result)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to change protection for address {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        // Update the stored protection
        // Find the page info again since we need a non-const reference
        for (ds_u64 i = 0; i < m_page_info_count; i++)
        {
            if (m_page_infos[i].base_address == ptr)
            {
                m_page_infos[i].protection = protection;
                break;
            }
        }

        Unlock();
        return true;
    }

    bool Page_Allocator::Commit(void* ptr, ds_u64 size)
    {
        if (!ptr || size == 0)
        {
            return false;
        }

        Lock();

        // Find the page info for this allocation
        const Page_Info* info = Get_Page_Info(ptr);
        if (!info)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Attempted to commit unknown address {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        // Ensure size is within bounds
        if (size > info->size)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Commit size {1} exceeds allocation size {2}",
                m_name, size, info->size);
            Unlock();
            return false;
        }

        // Commit the memory
#ifdef DS_PLATFORM_WINDOWS
        bool result = VirtualAlloc(ptr, size, MEM_COMMIT, Convert_Protection_Flags(info->protection)) != nullptr;
#else
        // On Unix, we might need to use madvise to ensure pages are resident
        bool result = madvise(ptr, size, MADV_WILLNEED) == 0;
#endif

        if (!result)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to commit memory at address {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        Unlock();
        return true;
    }

    bool Page_Allocator::Decommit(void* ptr, ds_u64 size)
    {
        if (!ptr || size == 0)
        {
            return false;
        }

        Lock();

        // Find the page info for this allocation
        const Page_Info* info = Get_Page_Info(ptr);
        if (!info)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Attempted to decommit unknown address {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        // Ensure size is within bounds
        if (size > info->size)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Decommit size {1} exceeds allocation size {2}",
                m_name, size, info->size);
            Unlock();
            return false;
        }

        // Decommit the memory
#ifdef DS_PLATFORM_WINDOWS
        bool result = VirtualFree(ptr, size, MEM_DECOMMIT) != 0;
#else
        // On Unix, we use MADV_DONTNEED to release physical pages
        bool result = madvise(ptr, size, MADV_DONTNEED) == 0;
#endif

        if (!result)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to decommit memory at address {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        Unlock();
        return true;
    }

    bool Page_Allocator::Flush(void* ptr, ds_u64 size)
    {
        if (!ptr || size == 0)
        {
            return false;
        }

        Lock();

        // Find the page info for this allocation
        const Page_Info* info = Get_Page_Info(ptr);
        if (!info)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Attempted to flush unknown address {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        // Ensure this is a memory-mapped file
        if (!info->file_path)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Attempted to flush non-file-mapped memory at {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        // Ensure size is within bounds
        if (size > info->size)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Flush size {1} exceeds allocation size {2}",
                m_name, size, info->size);
            Unlock();
            return false;
        }

        // Flush the memory
#ifdef DS_PLATFORM_WINDOWS
        bool result = FlushViewOfFile(ptr, size) != 0;
#else
        bool result = msync(ptr, size, MS_SYNC) == 0;
#endif

        if (!result)
        {
            DS_LOG_ERROR("Page Allocator '{0}': Failed to flush memory at address {1}",
                m_name, ptr);
            Unlock();
            return false;
        }

        Unlock();
        return true;
    }

    const Page_Allocator::Page_Info* Page_Allocator::Get_Page_Info(void* ptr) const
    {
        if (!ptr) return nullptr;

        // Check if this pointer is within any of our allocations
        for (ds_u64 i = 0; i < m_page_info_count; i++)
        {
            const Page_Info& info = m_page_infos[i];

            ds_u8* base = static_cast<ds_u8*>(info.base_address);
            ds_u8* end = base + info.size;
            ds_u8* check_ptr = static_cast<ds_u8*>(ptr);

            if (check_ptr >= base && check_ptr < end)
            {
                return &info;
            }
        }

        return nullptr;
    }

    bool Page_Allocator::Is_Allocated(void* ptr) const
    {
        return Get_Page_Info(ptr) != nullptr;
    }

    ds_u64 Page_Allocator::Get_System_Page_Size()
    {
#ifdef DS_PLATFORM_WINDOWS
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        return sys_info.dwPageSize;
#else
        return sysconf(_SC_PAGESIZE);
#endif
    }

    ds_u64 Page_Allocator::Get_Large_Page_Size()
    {
#ifdef DS_PLATFORM_WINDOWS
        return GetLargePageMinimum();
#else
        // On Linux, large pages are typically 2MB or 1GB
        // Here we're assuming 2MB large pages, but this should be detected at runtime
        return 2 * 1024 * 1024;
#endif
    }

#ifdef DS_DEBUG
    void* Page_Allocator::Allocate_Debug(ds_u64 size, Page_Protection protection, Page_Flags flags,
        const ds_char* file_path, ds_u64 file_offset,
        const ds_char* allocation_file, ds_i32 allocation_line)
    {
        void* result = Allocate(size, protection, flags, file_path, file_offset);

        if (result)
        {
            // Update debug information
            Lock();

            for (ds_u64 i = 0; i < m_page_info_count; i++)
            {
                if (m_page_infos[i].base_address == result)
                {
                    m_page_infos[i].allocation_file = allocation_file;
                    m_page_infos[i].allocation_line = allocation_line;
                    break;
                }
            }

            Unlock();
        }

        return result;
    }

    void Page_Allocator::Dump_Stats()
    {
        Lock();

        std::stringstream ss;
        ss << "===== Page Allocator '" << m_name << "' Stats =====" << std::endl;
        ss << "Page Size: " << m_page_size << " bytes (" << (m_page_size / 1024.0f) << " KB)" << std::endl;
        ss << "System Page Size: " << m_system_page_size << " bytes" << std::endl;
        ss << "Large Page Size: " << (m_large_page_size > 0 ? std::to_string(m_large_page_size) + " bytes" : "Not Available") << std::endl;
        ss << "Allocated Pages: " << m_allocated_page_count << " (" << (m_allocated_page_count * m_page_size / (1024.0f * 1024.0f)) << " MB)" << std::endl;
        ss << "Allocation Count: " << m_page_info_count << std::endl;

        if (m_reserved_address_space)
        {
            ss << "Reserved Address Space: " << m_reserved_address_space_size << " bytes at " << m_reserved_address_space << std::endl;
            ss << "Reserved Space Used: " << m_reserved_address_space_used << " bytes ("
                << (m_reserved_address_space_used * 100.0f / m_reserved_address_space_size) << "%)" << std::endl;
        }

        if (m_page_info_count > 0)
        {
            ss << "\nAllocations:" << std::endl;
            ss << "----------------------------------------------------------" << std::endl;
            ss << "    Size    | Pages |   Address   | Protection | Source" << std::endl;
            ss << "----------------------------------------------------------" << std::endl;

            for (ds_u64 i = 0; i < m_page_info_count; i++)
            {
                const Page_Info& info = m_page_infos[i];

                // Format protection
                const char* protection_str = "Unknown";
                switch (info.protection)
                {
                case Page_Protection::READ_ONLY:
                    protection_str = "Read-Only";
                    break;
                case Page_Protection::READ_WRITE:
                    protection_str = "Read-Write";
                    break;
                case Page_Protection::READ_WRITE_EXEC:
                    protection_str = "RWX";
                    break;
                case Page_Protection::NO_ACCESS:
                    protection_str = "No Access";
                    break;
                }

                ss << std::setw(10) << info.size << " | "
                    << std::setw(5) << info.page_count << " | "
                    << std::hex << std::setw(10) << info.base_address << std::dec << " | "
                    << std::setw(10) << protection_str << " | ";

                if (info.allocation_file)
                {
                    ss << info.allocation_file << ":" << info.allocation_line;
                }
                else
                {
                    ss << "Unknown Location";
                }

                if (info.file_path)
                {
                    ss << " (Mapped from " << info.file_path << ")";
                }

                ss << std::endl;
            }
        }

        ss << "==================================================";
        DS_LOG_INFO("{}", ss.str());

        Unlock();
    }
#endif

} // namespace ds::core::memory