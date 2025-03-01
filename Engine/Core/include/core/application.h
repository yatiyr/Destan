#pragma once
#include <core/defines.h>
#include <core/platform/time.h>
#include <core/window_system/main_window.h>
#include <core/layer_system/layer_stack.h>
#include <core/events/event.h>

// Main function as forward declaration
extern int main(int argc, char** argv);

namespace destan::core
{

class Application
{
public:
    Application();
    virtual ~Application();

    // Copy and move operators are deleted because
    // this class is a singleton, we don't want to
    // copy or move this class
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;    

    // Lifecycle methods
    virtual void On_Init() = 0;
    virtual void On_Shutdown() = 0;
    virtual void On_Update(float delta_time) = 0;

    // Layer management
    void Push_Layer(Layer* layer);
    void Push_Overlay(Layer* overlay);
    void Pop_Layer(Layer* layer);
    void Pop_Overlay(Layer* overlay);

    // Main functionality
    void Run();
    void Close();

    // Optional methods that can be overriden
    virtual void On_Event(Event& event);
    virtual void On_Window_Close(Window_Close_Event& e);
    virtual void On_Window_Resize(Window_Resize_Event& e);    
    virtual void On_ImGui_Render() {}

    // Getters for the application
    inline bool Is_Running() const { return m_running; }
    inline bool Is_Minimized() const { return m_minimized; }
    inline Main_Window* Get_Main_Window() const { return m_main_window.get(); }
    inline Layer_Stack& Get_Layer_Stack() { return m_layer_stack; }
    inline static Application& Get() { return *s_instance; }

private:
    void Initialize_Core();
    void Shutdown_Core();
    void Process_Events();
    float Calculate_Delta_Time();

private:
    bool m_running = true;
    bool m_minimized = false;
    float m_last_frame_time = 0.0f;

    Scope<Main_Window> m_main_window;
    Layer_Stack m_layer_stack;
    
    static Application* s_instance;
    friend int ::main(int argc, char** argv);

};

// This will be implemented by the client
Application* Create_Application();

} // namespace destan::core