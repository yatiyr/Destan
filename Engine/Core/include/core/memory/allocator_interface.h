#pragma once
#include <core/defines.h>

namespace destan::core::memory
{
    // Allocator interface for container compatibility
    template<typename T>
    class Allocator_Interface
    {
    public:
        using value_type = T;

        virtual ~Allocator_Interface() = default;

        virtual T* allocate(destan_u64 n) = 0;
        virtual void deallocate(T* p, destan_u64 n) = 0;

        template<typename U, typename... Args>
        void construct(U* p, Args&&... args)
        {
            new(p) U(std::forward<Args>(args)...);
        }

        template<typename U>
        void destroy(U* p)
        {
            p->~U();
        }
    };
}