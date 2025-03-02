#pragma once
#include <core/defines.h>

// For different platforms, we need to include
// these headers for high precision time calculations
#ifdef DESTAN_PLATFORM_LINUX
    #include <time.h>
#elif defined(DESTAN_PLATFORM_MACOS)
    #include <mach/mach_time.h>
#endif

namespace destan::core
{

    class Platform_Time
    {
    public:
        static float Get_Time();
        static u64 Get_Time_Microseconds();
        static void Sleep(u32 milliseconds);

        // Platform specific implementations
        static void Initialize();
        static void Shutdown();
    private:
        inline static f64 s_frequency = 0.0;
        inline static u64 s_start_time = 0;

        #ifdef DESTAN_PLATFORM_MACOS
            inline static mach_timebase_info_data_s_timebase_info = {0, 0};
        #endif

        friend class Application;
    };


    // Platform specific implementations
    #ifdef DESTAN_PLATFORM_WINDOWS

        inline float Platform_Time::Get_Time()
        {
            LARGE_INTEGER current;
            QueryPerformanceCounter(&current);
            return static_cast<float>((current.QuadPart - s_start_time) * s_frequency);        
        }

        inline u64 Platform_Time::Get_Time_Microseconds()
        {
            LARGE_INTEGER current;
            QueryPerformanceCounter(&current);
            return static_cast<u64>((current.QuadPart - s_start_time) * s_frequency * 1000000.0);
        }

        inline void Platform_Time::Sleep(u32 milliseconds)
        {
            Sleep(milliseconds);
        }

        inline void Platform_Time::Initialize()
        {
            LARGE_INTEGER frequency;
            QueryPerformanceCounter(&frequency);
            s_frequency = 1.0 / static_cast<f64>(frequency.QuadPart);

            LARGE_INTEGER start;
            QueryPerformanceCounter(&start);
            s_start_time = start.QuadPart;
        }

        inline void Platform_Time::Shutdown()
        {
            // we don't need cleanup for windows
        }

    #elif defined(DESTAN_PLATFORM_LINUX)

        inline float Platform_Time::Get_Time()
        {
            struct timespec current;
            clock_gettime(CLOCK_MONOTONIC, &current);
            u64 time = current.tv_sec * 1000000000ull + current.tv_nsec;
            return static_cast<float>(time - s_start_time) * s_frequency;
        }

        inline u64 Platform_Time::Get_Time_Microseconds()
        {
            struct timespec current;
            clock_gettime(CLOCK_MONOTONIC, &current);
            u64 time = current.tv_sec * 1000000000ull + current.tv_nsec;
            return static_cast<u64>((time - s_start_time) * s_frequency * 1000.0);
        }

        inline void Platform_Time::Sleep(u32 milliseconds)
        {
            struct timespec ts;
            ts.tv_sec = milliseconds / 1000;
            ts.tv_nsec = (milliseconds % 1000) * 1000000;
            nanosleep(&ts, nullptr);
        }

        inline void Platform_Time::Initialize()
        {
            s_frequency = 1.0 / 1000000000.0;  // nanoseconds to seconds
        
            struct timespec start;
            clock_gettime(CLOCK_MONOTONIC, &start);
            s_start_time = start.tv_sec * 1000000000ull + start.tv_nsec;
        }

        inline void Platform_Time::Shutdown()
        {
            // we don't need cleanup for linux
        }

    #elif defined(DESTAN_PLATFORM_MACOS)

        inline float Platform_Time::Get_Time()
        {
            uint64_t current = mach_absolute_time();
            return static_cast<float>((current - s_start_time) * s_timebase_info.numer * s_frequency / s_timebase_info.denom);
        }

        inline u64 Platform_Time::Get_Time_Microseconds()
        {
            uint64_t current = mach_absolute_time();
            return static_cast<u64>((current - s_start_time) * s_timebase_info.numer / s_timebase_info.denom) / 1000;
        }

        inline void Platform_Time::Sleep(u32 milliseconds)
        {
            struct timespec ts;
            ts.tv_sec = milliseconds / 1000;
            ts.tv_nsec = (milliseconds % 1000) * 1000000;
            nanosleep(&ts, nullptr);
        }

        inline void Platform_Time::Initialize()
        {
            mach_timebase_info(&s_timebase_info);
            s_frequency = 1.0e-9;  // nanoseconds to seconds
            s_start_time = mach_absolute_time();
        }

        inline void Platform_Time::Shutdown()
        {
            // we don't need cleanup for MacOS
        }

    #endif
}