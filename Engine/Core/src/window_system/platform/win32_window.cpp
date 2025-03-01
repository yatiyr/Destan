#ifdef DESTAN_PLATFORM_WINDOWS

#include <core/window_system/main_window.h>
#include <core/events/application_event.h>
#include <core/events/key_event.h>
#include <core/events/mouse_event.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace destan::core
{

struct Main_Window::Window_Data
{
    GLFWWindow* window = nullptr;
    void* context = nullptr; // for graphics api context
    String title;
    u32 width = 0;
    u32 height = 0;
    bool vsync = false;
};

static void GLFW_Error_Callback(int error, const char* description)
{
    DESTAN_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

static bool s_glfw_initialized = false;

Scope<Main_Window> Main_Window::Create(const Window_Props& props)
{
    return CreateScope<Main_Window>(props);
}

Main_Window::Main_Window(const Window_Props& props) : m_data(new Window_Data())
{
    m_data->title = props.title;
    m_data->width = props.width;
    m_data->height = props.height;

    if (!s_glfw_initialized)
    {
        int success = glfwInit();
        DESTAN_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFW_Error_Callback);
        s_glfw_initialized = true;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // for vulkan
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_data->window = glfwCreateWindow(props.width, props.height, props.title.c_str(), nullptr, nullptr);
    DESTAN_CORE_ASSERT(m_data->window, "Failed to create GLFW window!");

    // window user pointer for callbacks
    glfwSetWindowUserPointer(m_data->window, this);

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(m_data->window, [](GLFWwindow* window, int width, int height)
    {
        Main_Window& win = *(Main_Window*)glfwGetWindowUserPointer(window);
        win.m_data->width = width;
        win.m_data->height = height;

        Window_Resize_Event event(width, height);
        win.m_event_callback(event);
    });

    glfwSetWindowCloseCallback(m_data->window, [](GLFWwindow* window)
    {
        Main_Window& win = *(Main_Window*)glfwGetWindowUserPointer(window);
        Window_Close_Event event;
        win.m_event_callback(event);
    });

    glfwSetKeyCallback(m_data->window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        Main_Window& win = *(Main_Window*)glfwGetWindowUserPointer(window);

        switch (action) {
            case GLFW_PRESS: {
                Key_Pressed_Event event(key, 0);
                win.m_event_callback(event);
                break;
            }
            case GLFW_RELEASE: {
                Key_Released_Event event(key);
                win.m_event_callback(event);
                break;
            }
            case GLFW_REPEAT: {
                Key_Pressed_Event event(key, 1);
                win.m_event_callback(event);
                break;
            }
        }
    });

    glfwSetMouseButtonCallback(m_data->window, [](GLFWwindow* window, int button, int action, int mods)
    {
        Main_Window& win = *(Main_Window*)glfwGetWindowUserPointer(window);

        switch (action) {
            case GLFW_PRESS: {
                Mouse_Button_Pressed_Event event(button);
                win.m_event_callback(event);
                break;
            }
            case GLFW_RELEASE: {
                Mouse_Button_Released_Event event(button);
                win.m_event_callback(event);
                break;
            }
        }
    });

    glfwSetScrollCallback(m_data->window, [](GLFWwindow* window, double xOffset, double yOffset)
    {
        Main_Window& win = *(Main_Window*)glfwGetWindowUserPointer(window);
        
        Mouse_Scrolled_Event event((float)xOffset, (float)yOffset);
        win.m_event_callback(event);
    });

    glfwSetCursorPosCallback(m_data->window, [](GLFWwindow* window, double x, double y)
    {
        Main_Window& win = *(Main_Window*)glfwGetWindowUserPointer(window);
        
        Mouse_Moved_Event event((float)x, (float)y);
        win.m_event_callback(event);
    });

    Set_VSync(props.vsync);    
}

Main_Window::~Main_Window()
{
    if (m_data->window) {
        glfwDestroyWindow(m_data->window);
    }
    delete m_data;
}

void Main_Window::On_Update(float delta_time)
{
    glfwPollEvents();
    // Graphics context swap buffers will be here when we add renderer
}

void Main_Window::Process_Events()
{
    // Additional event processing if needed
}

u32 Main_Window::Get_Width() const
{ 
    return m_data->width; 
}

u32 Main_Window::Get_Height() const
{ 
    return m_data->height; 
}

void Main_Window::Set_Event_Callback(const Event_Callback_Fn& callback)
{
    m_event_callback = callback;
}

void Main_Window::Set_VSync(bool enabled)
{
    glfwSwapInterval(enabled ? 1 : 0);
    m_data->vsync = enabled;
}

bool Main_Window::Is_VSync() const
{
    return m_data->vsync;
}

void* Main_Window::Get_Native_Window() const
{
    return m_data->window;
}

void* Main_Window::Get_Native_Context() const
{
    return m_data->context;
}

}
#endif