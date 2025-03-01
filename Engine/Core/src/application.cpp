#include <core/application.h>

namespace destan::core
{

Application* Application::s_instance = nullptr;

Application::Application()
{
    DESTAN_ASSERT(!s_instance, "Application has already been set!");
    s_instance = this;

    Initialize_Core();
}

Application::~Application()
{
    Shutdown_Core();
    s_instance = nullptr;
}

void Application::Initialize_Core()
{
    // Initialize platform time
    Platform_Time::Initialize();

    // Create main window
    Window_Props props;
    m_main_window = Main_Window::Create(props);
    m_main_window->Set_Event_Callback(DESTAN_BIND_EVENT_FN(Application::On_Event));

    // Create layer stack
    m_layer_stack = Layer_Stack();
}

void Application::Shutdown_Core()
{
    // Shutdown in reverse order
    m_layer_stack.Clear();
    m_main_window.reset();
    Platform_Time::Shutdown();
}

void Application::Run()
{
    On_Init();

    while (m_running)
    {
        float delta_time = Calculate_Delta_Time();

        Process_Events();

        // Update core services TODO_EREN: DO LATER!
        // m_physics_server->Update(delta_time)
        // m_render_server->Update(delta_time)

        // Update main window
        if (m_main_window)
        {
            m_main_window->On_Update(delta_time);
        }

        On_Update(delta_time);
    }

    On_Shutdown();
}

void Application::Close()
{
    m_running = false;
}

void Application::Process_Events()
{
    // Process window and input elements
    if (m_main_window)
    {
        m_main_window->Process_Events();
    }
}

float Application::Calculate_Delta_Time()
{
    float time = Get_Time(); // Platform specific time implementation
    float delta_time = time - m_last_frame_time;
    m_last_frame_time = time;
    return delta_time;
}

void Application::Push_Layer(Layer* layer)
{
    m_layer_stack.Push_Layer(layer);
}

void Application::Push_Overlay(Layer* overlay)
{
    m_layer_stack.Push_Overlay(overlay);
}

void Application::Pop_Layer(Layer* layer)
{
    m_layer_stack.Pop_Layer(layer);
}

void Application Pop_Overlay(Layer* overlay)
{
    m_layer_stack.Pop_Overlay(overlay);
}

void Application::On_Event(Event& event)
{
    Event_Dispatcher dispatcher(event);
    
    // Handle window events
    dispatcher.Dispatch<Window_Close_Event>(DESTAN_BIND_EVENT_FN(Application::On_Window_Close));
    dispatcher.Dispatch<Window_Resize_Event>(DESTAN_BIND_EVENT_FN(Application::On_Window_Resize));

    // Propagate events to layers in reverse order
    for (auto it = m_layer_stack.rbegin(); it != m_layer_stack.rend(); ++it)
    {
        if (event.handled)
        {
            break;
        }
        (*it)->On_Event(event);
    }
}

bool Application::On_Window_Close(Window_Close_Event& e)
{
    m_running = false;
    return true;
}

bool Application::On_Window_Resize(Window_Resize_Event& e)
{
    if (e.Get_Width() == 0 || e.Get_Height() == 0)
    {
        m_minimized = true;
        return false;
    }
    
    m_minimized = false;
    // Notify layers of resize
    return false;
}


} // namespace destan::core
