// WindowApp.h - the game's window application: owns the window tree (desktop window), queues the
// input events, keeps the frame timing and the paint stacks (color / scale / translation).
// Owner: src/WindowEngine/WindowApp.cpp (b045x; first drafts by b042 and b057x).
// Layout from WindowApp::WindowApp(), reinit(), HandleUpdate() and HandleEvents().
#ifndef WINDOWAPP_H
#define WINDOWAPP_H

#include <stddef.h>
#include "WindowBase.h"
#include "CColor.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class ICGraphics2d;
} } } }
class RandomValue;

class WindowApp {
public:
    /** Original Ballistics uses this same four-byte LCG state at +0x1c4. */
    RandomValue& GetRandomValue()
    {
        return *(RandomValue*)&m_randomSeed;
    }
    /** Growable array of plain values (paint stacks): doubles its capacity, copies with memcpy. */
    template <class T>
    class FastSimpleVector {
    public:
        FastSimpleVector()
        {
            m_capacity = 8;
            m_data = new T[m_capacity];
            m_size = 0;
        }

        ~FastSimpleVector()
        {
            delete[] m_data;
        }

        void addElement(const T& element);

        T* m_data;              // 0x00
        unsigned int m_capacity;    // 0x04
        int m_size;             // 0x08
    };

    /** Number of tracked pointers (touches). */
    enum { MAX_POINTERS = 10 };

    /** Commands of the soft keys (UpdateSoftkeysCapture). */
    enum {
        COMMAND_FIND_SOFTKEYS_OWNER = 0x649999bc,
        COMMAND_SOFTKEYS_OWNER_CHANGED = 0x7e3d3cc1,
        COMMAND_END_OF_FRAME = 0x7fffff9c   // HandleEvents stops at this command
    };

    WindowApp();
    virtual ~WindowApp();

    virtual void Init()
    {
    }

    virtual void Shutdown();

    /** The desktop window (root of the window tree). */
    virtual Window* NewDisplayWindow()
    {
        return new WindowColor();
    }

    virtual void Start()
    {
    }

    virtual void Stop()
    {
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnEnterBackground()
    {
    }

    virtual void OnEnterForeground()
    {
    }

    virtual void OnBeginWindowPaint(com::glu::platform::graphics::ICGraphics2d& graphics, Window* window)
    {
    }

    virtual void OnEndWindowPaint(com::glu::platform::graphics::ICGraphics2d& graphics, Window* window)
    {
    }

    virtual void OnBeginWindowPaint3D(com::glu::platform::graphics::ICGraphics2d& graphics, Window* window)
    {
    }

    virtual void OnEndWindowPaint3D(com::glu::platform::graphics::ICGraphics2d& graphics, Window* window)
    {
    }

    virtual void OnWindowDestroyed(Window* window);

    virtual void OnKeyPressed(Event& event)
    {
    }

    virtual void OnKeyReleased(Event& event)
    {
    }

    virtual void OnCommand(Event& event)
    {
    }

    virtual void OnPointerPressed(Event& event)
    {
    }

    virtual void OnPointerDragged(Event& event)
    {
    }

    virtual void OnPointerReleased(Event& event)
    {
    }

    virtual int RemapKey(int key);

    virtual void Update()
    {
    }

    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
    {
    }

    virtual void UpdateTime();

    /** Reset the timing, the pointers, the event lists and the paint stacks. */
    void reinit();
    /** Move every queued event to the free list. */
    static void ClearAllEvents();
    /** Queue an event for the window tree. */
    static void PutEvent(Event::EType type, int id, int param1, int param2, int param3);
    /** Dispatch an event to the window tree right away. */
    static void HandleEvent(Event::EType type, int id, int param1, int param2, int param3);
    static void HandleCommand(int id, int param1, int param2, int param3);
    static void HandleTunnelCommand(int id, int param1, int param2, int param3);
    /** Show `window` as a modal child of the desktop. */
    static void AddModal(Window* window);
    /** Window capturing pointer `pointer` (NULL: none). */
    static Window* PointerCapture(int pointer);
    static void SetPointerCapture(int pointer, Window* window);
    static void GetPointerPos(int pointer, int& x, int& y);
    /** Movement of pointer `pointer` since its previous event. */
    static void GetPointerOffset(int pointer, int& dx, int& dy);
    /** Ask the window tree which window owns the soft keys; tell the tree when it changed. */
    static Window* UpdateSoftkeysCapture();
    /** Restart the application at the end of the next update. */
    void InitializeRestartApplication(int delay);
    void RestartApplication();
    void HandleDestroy();
    void HandleCreate();
    void HandleDeviceOrientationChanged();
    void HandleUpdate(int elapsed);
    void HandlePause();
    void HandleResume();
    void HandleEnterBackground();
    void HandleEnterForeground();
    void HandleEvents();
    void HandlePaint();

    /** Opacity of the current paint color. */
    float Opacity() const;
    /** Current drawing color (top of the color stack). */
    const com::glu::platform::components::Color_RGBA& Color() const;
    /** Push `color` with its alpha multiplied by the current opacity. */
    com::glu::platform::components::Color_RGBA& PushColor(com::glu::platform::components::Color_RGBA& color);
    /** Push the current color with its opacity multiplied by `opacity`; returns the new opacity. */
    float PushOpacity(float opacity);
    /** Push the current color with exactly `opacity`. */
    float PushOpacityExact(float opacity);
    void PopColorStack();
    float WinScale() const;
    float PushWinScale(float scale);
    void PopWinScale();
    const Window::Point& Translation() const;
    const Window::Point& PushTranslation(int dx, int dy);
    void PopTranslation();

    static WindowApp* m_instance;   // the running window application

    int m_frameTimes[8];            // 0x04 durations of the last frames (ms), [7] the newest
    int m_frameStartTime;           // 0x24 up time at the frame start
    int m_time;                     // 0x28 application time (ms)
    int m_averageFrameTime;         // 0x2c
    int m_updateTime;               // 0x30 m_time when UpdateTime() ran
    int m_updateFrameTime;          // 0x34 m_frameTimes[7] when UpdateTime() ran
    int m_field38;                  // 0x38
    int m_eventsDuration;           // 0x3c time spent in the last HandleEvents
    int m_updateDuration;           // 0x40 time spent in the last HandleUpdate
    int m_paintDuration;            // 0x44 time spent in the last HandlePaint
    float m_timeSeconds;            // 0x48 m_updateTime in seconds
    float m_frameTimeSeconds;       // 0x4c m_updateFrameTime in seconds
    float m_fps;                    // 0x50
    int m_restartDelay;             // 0x54 InitializeRestartApplication
    Event* m_currentEvents;         // 0x58 events being handled this frame
    Event* m_eventsHead;            // 0x5c queued events
    Event* m_eventsTail;            // 0x60
    Event* m_freeEvents;            // 0x64 recycled events
    int m_freeEventCount;           // 0x68
    Window* m_desktop;              // 0x6c root of the window tree
    Window* m_softkeysOwner;        // 0x70
    Window* m_pointerCapture[MAX_POINTERS];     // 0x74
    int m_pointerIds[MAX_POINTERS];             // 0x9c -1: free
    int m_pointerTimes[MAX_POINTERS];           // 0xc4 m_updateTime of the press
    Window::Point m_pointerPos[MAX_POINTERS];   // 0xec
    Window::Point m_pointerPrevPos[MAX_POINTERS];   // 0x13c
    int m_screenWidth;              // 0x18c
    int m_screenHeight;             // 0x190
    FastSimpleVector<com::glu::platform::components::Color_RGBA> m_colorStack;   // 0x194
    FastSimpleVector<float> m_scaleStack;                   // 0x1a0
    FastSimpleVector<Window::Point> m_translationStack;     // 0x1ac
    FastSimpleVector<Window*> m_paintQueue;                 // 0x1b8 windows painted last (PaintChildren)
    unsigned int m_randomSeed;      // 0x1c4
    unsigned char m_field1c8;       // 0x1c8
    unsigned char m_restartPending; // 0x1c9
};

template <class T>
void WindowApp::FastSimpleVector<T>::addElement(const T& element)
{
    if (m_size == m_capacity) {
        unsigned int newCapacity = m_capacity * 2;
        T* newData = new T[newCapacity];
        memcpy(newData, m_data, m_size * sizeof(T));
        delete[] m_data;
        m_data = newData;
        m_capacity = newCapacity;
    }
    m_data[m_size++] = element;
}

#endif
