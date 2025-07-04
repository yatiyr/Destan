#pragma once

// Core headers
#include <core/defines.h>
#include <core/logger/logger.h>

// Application & Entry Point
// #include "core/application.h"
// #include "core/entry_point.h"

// Window System
// #include "core/window_system/window.h"
// #include "core/window_system/node.h"

// Event System
// #include "core/events/event.h"
// #include "core/events/key_event.h"
// #include "core/events/mouse_event.h"
// #include "core/events/window_event.h"

// Layer System
// #include "core/layer.h"
// #include "core/layer_stack.h"

// Utility
// #include "core/logger.h"


// Entry point macro 
#define DS_MAIN(AppClass) \
    ds::core::Application* ds::core::Create_Application() { \
        return new AppClass(); \
    }


// Namespace ds
namespace ds
{

// Version info
constexpr const char* ENGINE_VERSION = "0.1.0";

// Engine mode flags
enum class Engine_Mode
{
    DEBUG,
    RELEASE,
    SHIPPING
};

#ifdef DS_DEBUG
    constexpr Engine_Mode CURRENT_MODE = Engine_Mode::DEBUG;
#elif defined(DS_RELEASE)
    constexpr Engine_Mode CURRENT_MODE = Engine_Mode::RELEASE;
#elif defined(DS_SHIPPING)
    constexpr Engine_Mode CURRENT_MODE = Engine_Mode::SHIPPING;
#endif

} // namespace ds

// ds namespace for core
namespace ds::core
{
    // Will be added
} // namespace ds::core