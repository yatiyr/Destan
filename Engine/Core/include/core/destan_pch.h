#pragma once

// STL containers
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <queue>
#include <deque>

// STL utility
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <optional>
#include <variant>
#include <any>

// C++23 features
#include <expected>
#include <spanstream>

// #include <generator> TODO_EREN: will be added

// STL concurrency
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <latch>
#include <barrier>
#include <semaphore>

// STL I/O and filesystem
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

// Platform specific
#ifdef _WIN32
    #include <Windows.h>
    #ifdef CreateWindow
        #undef CreateWindow
    #endif
#elif defined(__linux__)
    #include <unistd.h>
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
#endif