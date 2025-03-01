#pragma once
#include <core/destan_pch.h>

// Detect platforms
#if defined(_WIN32)
    #define DESTAN_PLATFORM_WINDOWS
    #ifdef _WIN64
        #define DESTAN_PLATFORM_WINDOWS_64
    #else
        #define DESTAN_PLATFORM_WINDOWS_32
    #endif
#elif defined(__linux__)
    #define DESTAN_PLATFORM_LINUX
#elif defined(__APPLE__)
    #define DESTAN_PLATFORM_MACOS
#else
    #error "Platform not supported!"
#endif

// Detect compiler
#if defined(_MSC_VER)
    #define DESTAN_COMPILER_MSVC
#elif defined(__clang__)
    #define DESTAN_COMPILER_CLANG
#elif defined(__GNUC__)
    #define DESTAN_COMPILER_GCC
#else
    #error "Compiler not supported!"
#endif

// Detect CPU Architecture
#if defined(_M_X64) || defined(__x86_64__)
    #define DESTAN_ARCH_X64
#elif defined(_M_IX86) || defined(__i386__)
    #define DESTAN_ARCH_X86
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define DESTAN_ARCH_ARM64
#elif defined(_M_ARM) || defined(__arm__)
    #define DESTAN_ARCH_ARM32
#else
    #error "Architecture not supported!"
#endif

// Detect SIMD Support
#if defined(DESTAN_ARCH_X64) || defined(DESTAN_ARCH_X86)
    #if defined(DESTAN_COMPILER_MSVC)
        #include <intrin.h>
    #else
        #include <x86intrin.h>
    #endif

    #if defined(__AVX512F__)
        #define DESTAN_SIMD_AVX512
    #endif

    #if defined(__AVX2__)
        #define DESTAN_SIMD_AVX2
    #endif

    #if defined(__AVX__)
        #define DESTAN_SIMD_AVX
    #endif

    #if defined(__SSE4_2__)
        #define DESTAN_SIMD_SSE4_2
    #endif

    #if defined(__SSE4_1__)
        #define DESTAN_SIMD_SSE4_1
    #endif

    #if defined(__SSSE3__)
        #define DESTAN_SIMD_SSSE3
    #endif

    #if defined(__SSE3__)
        #define DESTAN_SIMD_SSE3
    #endif

    #if defined(__SSE2__) || defined(DESTAN_ARCH_X64)
        #define DESTAN_SIMD_SSE2
    #endif

    #if defined(__SSE__)
        #define DESTAN_SIMD_SSE
    #endif

#elif defined(DESTAN_ARCH_ARM64) || defined(DESTAN_ARCH_ARM32)
    #if defined(__ARM_NEON)
        #define DESTAN_SIMD_NEON
    #endif

#endif

// Debug configuration
#ifdef DESTAN_DEBUG
    #if defined(DESTAN_COMPILER_MSVC)
        #define DESTAN_DEBUGBREAK() __debugbreak()
    #elif defined(DESTAN_COMPILER_CLANG) || defined(DESTAN_COMPILER_GCC)
        #include <signal.h>
        #define DESTAN_DEBUGBREAK() raise(SIGTRAP)
    #else
        #error "Debugbreak is not supported by this platform!"
    #endif
    #define DESTAN_ENABLE_ASSERTS
#else
    #define DESTAN_DEBUGBREAK()
#endif

// Macros for assertions
#ifdef DESTAN_ENABLE_ASSERTS
    #define DESTAN_ASSERT(x, ...) { if(!(x)) { DESTAN_ERROR("Assertion Failed: {0}", __VA_ARGS__); DESTAN_DEBUGBREAK(); } }
    #define DESTAN_CORE_ASSERT(x, ...) { if(!(x)) { DESTAN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); DESTAN_DEBUGBREAK(); } }
#else
    #define DESTAN_ASSERT(x, ...)
    #define DESTAN_CORE_ASSERT(x, ...)
#endif

// Macros for bit manipulation
#define BIT(x) (1 << (x))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define ALIGN_UP(x, align) ALIGN_DOWN((x) + (align) - 1, (align))
#define IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

// Basic type definitions
namespace destan {
    // Integer types
    using i8  = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;
    
    using u8  = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;
    
    // Floating point types
    using f32 = float;
    using f64 = double;
    
    // String types
    using String = std::string;
    using StringView = std::string_view;
    
    // Smart pointer aliases
    template<typename T>
    using Scope = std::unique_ptr<T>;
    
    template<typename T>
    using Ref = std::shared_ptr<T>;
    
    template<typename T>
    using WeakRef = std::weak_ptr<T>;

    // SIMD vector types (platform specific)
    #if defined(DESTAN_SIMD_AVX512)
        using SimdVec = __m512;
        using SimdVecInt = __m512i;
        using SimdVecDouble = __m512d;
    #elif defined(DESTAN_SIMD_AVX)
        using SimdVec = __m256;
        using SimdVecInt = __m256i;
        using SimdVecDouble = __m256d;
    #elif defined(DESTAN_SIMD_SSE2)
        using SimdVec = __m128;
        using SimdVecInt = __m128i;
        using SimdVecDouble = __m128d;
    #elif defined(DESTAN_SIMD_NEON)
        using SimdVec = float32x4_t;
        using SimdVecInt = int32x4_t;
        using SimdVecDouble = float64x2_t;
    #endif
}


// Utility macros
#define DESTAN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
#define DESTAN_DISABLE_COPY(TypeName) TypeName(const TypeName&) = delete; TypeName& operator=(const TypeName&) = delete;
#define DESTAN_DISABLE_MOVE(TypeName) TypeName(TypeName&&) = delete; TypeName& operator=(TypeName&&) = delete;
#define DESTAN_DEFAULT_COPY(TypeName) TypeName(const TypeName&) = default; TypeName& operator=(const TypeName&) = default;
#define DESTAN_DEFAULT_MOVE(TypeName) TypeName(TypeName&&) = default; TypeName& operator=(TypeName&&) = default;