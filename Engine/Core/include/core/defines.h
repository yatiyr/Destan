#pragma once

// Detect platforms
#if defined(_WIN32)
    #ifdef _WIN64
        #define DS_PLATFORM_WINDOWS_64
    #else
        #define DS_PLATFORM_WINDOWS_32
    #endif
#elif defined(__linux__)
    #define DS_PLATFORM_LINUX
#elif defined(__APPLE__)
    #define DS_PLATFORM_MACOS
#else
    #error "Platform not supported!"
#endif

// Detect compiler
#if defined(_MSC_VER)
    #define DS_COMPILER_MSVC
#elif defined(__clang__)
    #define DS_COMPILER_CLANG
#elif defined(__GNUC__)
    #define DS_COMPILER_GCC
#else
    #error "Compiler not supported!"
#endif

// Detect CPU Architecture
#if defined(_M_X64) || defined(__x86_64__)
    #define DS_ARCH_X64
#elif defined(_M_IX86) || defined(__i386__)
    #define DS_ARCH_X86
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define DS_ARCH_ARM64
#elif defined(_M_ARM) || defined(__arm__)
    #define DS_ARCH_ARM32
#else
    #error "Architecture not supported!"
#endif

// Detect SIMD Support
#if defined(DS_ARCH_X64) || defined(DS_ARCH_X86)
    #if defined(DS_COMPILER_MSVC)
        #include <intrin.h>
    #else
        #include <x86intrin.h>
    #endif

    #if defined(__AVX512F__)
        #define DS_SIMD_AVX512
    #endif

    #if defined(__AVX2__)
        #define DS_SIMD_AVX2
    #endif

    #if defined(__AVX__)
        #define DS_SIMD_AVX
    #endif

    #if defined(__SSE4_2__)
        #define DS_SIMD_SSE4_2
    #endif

    #if defined(__SSE4_1__)
        #define DS_SIMD_SSE4_1
    #endif

    #if defined(__SSSE3__)
        #define DS_SIMD_SSSE3
    #endif

    #if defined(__SSE3__)
        #define DS_SIMD_SSE3
    #endif

    #if defined(__SSE2__) || defined(DS_ARCH_X64)
        #define DS_SIMD_SSE2
    #endif

    #if defined(__SSE__)
        #define DS_SIMD_SSE
    #endif

#elif defined(DS_ARCH_ARM64) || defined(DS_ARCH_ARM32)
    #if defined(__ARM_NEON)
        #define DS_SIMD_NEON
    #endif

#endif

// Debug configuration
#ifdef DS_DEBUG
    #if defined(DS_COMPILER_MSVC)
        #define DS_DEBUGBREAK() __debugbreak()
    #elif defined(DS_COMPILER_CLANG) || defined(DS_COMPILER_GCC)
        #include <signal.h>
        #define DS_DEBUGBREAK() raise(SIGTRAP)
    #else
        #error "Debugbreak is not supported by this platform!"
    #endif
    #define DS_ENABLE_ASSERTS
#else
    #define DS_DEBUGBREAK()
#endif

// Macros for bit manipulation
#define BIT(x) (1 << (x))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define ALIGN_UP(x, align) ALIGN_DOWN((x) + (align) - 1, (align))
#define IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

// Basic type definitions
namespace ds {

    // All types used in the engine are defined here
	// their names are similar to the standard library
	// but with ds_ prefix

    // Unsigned Integer types
	using ds_u8 = uint8_t;
	using ds_u16 = uint16_t;
	using ds_u32 = uint32_t;
	using ds_u64 = uint64_t;

    // Integer Types
	using ds_i8 = int8_t;
	using ds_i16 = int16_t;
	using ds_i32 = int32_t;
	using ds_i64 = int64_t;

	// Floating point types
	using ds_f32 = float;
	using ds_f64 = double;

    // Character tyoes
	using ds_char = char;
	using ds_wchar = wchar_t;
	using ds_char8 = char8_t;
	using ds_char16 = char16_t;
	using ds_char32 = char32_t;

	// Const character types
	using ds_const_char = const char;
	using ds_const_wchar = const wchar_t;
	using ds_const_char8 = const char8_t;
	using ds_const_char16 = const char16_t;
	using ds_const_char32 = const char32_t;

    // Boolean type
	using ds_bool = bool;

	// Pointer types
	using ds_vptr = void*;
	using ds_cvptr = const void*;
	using ds_uiptr = uintptr_t;
	using ds_ciptr = const uintptr_t;          

    // SIMD vector types (platform specific)
    #if defined(DS_SIMD_AVX512)
        using SimdVec = __m512;
        using SimdVecInt = __m512i;
        using SimdVecDouble = __m512d;
    #elif defined(DS_SIMD_AVX)
        using SimdVec = __m256;
        using SimdVecInt = __m256i;
        using SimdVecDouble = __m256d;
    #elif defined(DS_SIMD_SSE2)
        using SimdVec = __m128;
        using SimdVecInt = __m128i;
        using SimdVecDouble = __m128d;
    #elif defined(DS_SIMD_NEON)
        using SimdVec = float32x4_t;
        using SimdVecInt = int32x4_t;
        using SimdVecDouble = float64x2_t;
    #endif
}


// Utility macros
#define DS_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
#define DS_DISABLE_COPY(TypeName) TypeName(const TypeName&) = delete; TypeName& operator=(const TypeName&) = delete;
#define DS_DISABLE_MOVE(TypeName) TypeName(TypeName&&) = delete; TypeName& operator=(TypeName&&) = delete;
#define DS_DEFAULT_COPY(TypeName) TypeName(const TypeName&) = default; TypeName& operator=(const TypeName&) = default;
#define DS_DEFAULT_MOVE(TypeName) TypeName(TypeName&&) = default; TypeName& operator=(TypeName&&) = default;