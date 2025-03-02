#pragma once
#include <core/defines.h>
#include <core/events/event.h>

namespace destan::core
{

    struct Window_Props
    {
        String title;
        u32 width;
        u32 height;
        bool vsync;

        Window_Props(const String& title = "Destan Engine",
            u32 width = 1280,
            u32 height = 720,
            bool vsync = true)
        : title(title), width(width), height(height), vsync(vsync) {}
    };

    class Main_Window
    {
    public:
        using Event_Callback_Fn = std::function<void(Event&)>;

        static Scope<Main_Window> Create(const Window_Props& props = Window_Props());
        ~Main_Window() = default;

        void On_Update(float delta_time);
        void Process_Events();

        u32 Get_Width() const;
        u32 Get_Height() const;

        void Set_Event_Callback(const Event_Callback_Fn& callback);
        void Set_VSync(bool enabled);
        bool Is_VSync() const;

        virtual void* Get_Native_Window() const = 0;
        virtual void* Get_Native_Context() const = 0;

    protected:
        Event_Callback_Fn m_event_callback;
    };

}