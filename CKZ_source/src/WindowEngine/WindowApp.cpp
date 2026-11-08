// WindowApp.cpp - the window application: event queue, frame timing, paint stacks and the soft keys
// container window.
#include "WindowApp.h"
#include "App.h"
#include "CStdUtil_Cocoa.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "ICRasterizerState.h"
#include "ICRenderSurface.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;

/** Soft keys bar: a window that stays open while its owner window exists. */
class SoftkeysContainer : public Window {
public:
    SoftkeysContainer(Window* owner);

    virtual ~SoftkeysContainer()
    {
    }

    virtual void Init();
    virtual void Shutdown();
    virtual void OnClosed();
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);

    Window* m_owner;        // 0xac
    bool m_active;          // 0xb0
};

WindowApp* WindowApp::m_instance;

WindowApp::WindowApp()
{
    m_randomSeed = 0;
    m_instance = this;
    reinit();
    m_restartDelay = 0;
}

void WindowApp::reinit()
{
    m_frameStartTime = -10;
    m_averageFrameTime = 100;
    m_time = 0;
    m_updateTime = 0;
    m_updateFrameTime = 0;
    m_field38 = 0;
    m_timeSeconds = 0;
    m_frameTimeSeconds = 0;
    for (int frame = 0; frame < 8; frame++) {
        m_frameTimes[frame] = 0x7fffffff;
    }
    m_eventsDuration = 0;
    m_updateDuration = 0;
    m_paintDuration = 0;
    m_fps = 0;
    m_currentEvents = NULL;
    m_eventsHead = NULL;
    m_eventsTail = NULL;
    m_freeEvents = NULL;
    m_freeEventCount = 0;
    m_desktop = NULL;
    m_softkeysOwner = NULL;
    for (int pointer = 0; pointer < MAX_POINTERS; pointer++) {
        m_pointerTimes[pointer] = 0;
        m_pointerCapture[pointer] = NULL;
        m_pointerIds[pointer] = -1;
        m_pointerPrevPos[pointer].x = 0;
        m_pointerPos[pointer].x = 0;
        m_pointerPos[pointer].y = 0;
        m_pointerPrevPos[pointer].y = 0;
    }
    m_screenWidth = 0;
    m_screenHeight = 0;

    Color_RGBA white;
    white.r = 1.0f;
    white.g = 1.0f;
    white.b = 1.0f;
    white.a = 1.0f;
    m_colorStack.addElement(white);
    float scale = 1.0f;
    m_scaleStack.addElement(scale);

    m_randomSeed = CStdUtil_Cocoa::GetTimeSeconds() ^ CStdUtil_Cocoa::GetUpTimeMS();
    m_restartPending = 0;
}

WindowApp::~WindowApp()
{
    ClearAllEvents();
    Event* event = m_freeEvents;
    while (event != NULL) {
        Event* next = event->m_next;
        event->m_next = NULL;
        delete event;
        event = next;
    }
    m_freeEvents = NULL;
    m_freeEventCount = 0;
    m_instance = NULL;
}

void WindowApp::ClearAllEvents()
{
    WindowApp* app = m_instance;
    Event* event = app->m_currentEvents;
    while (event != NULL) {
        Event* next = event->m_next;
        event->m_next = app->m_freeEvents;
        app->m_freeEvents = event;
        app->m_freeEventCount++;
        event = next;
    }
    app->m_currentEvents = NULL;
    event = app->m_eventsHead;
    while (event != NULL) {
        Event* next = event->m_next;
        event->m_next = app->m_freeEvents;
        app->m_freeEvents = event;
        app->m_freeEventCount++;
        event = next;
    }
    app->m_eventsHead = NULL;
    app->m_eventsTail = NULL;
}

void WindowApp::PutEvent(Event::EType type, int id, int param1, int param2, int param3)
{
    WindowApp* app = m_instance;
    // Key events carry the raw key in param1 and the remapped key in param2.
    if (type == Event::KEY_PRESSED || type == Event::KEY_RELEASED) {
        if (param1 == 0) {
            param1 = app->RemapKey(id);
        }
        param2 = param1;
        param1 = id;
    }
    Event* event = app->m_freeEvents;
    if (event != NULL) {
        app->m_freeEvents = event->m_next;
        app->m_freeEventCount--;
    } else {
        event = (Event*)malloc(sizeof(Event));
    }
    new (event) Event(type, id, param1, param2, param3);
    if (app->m_eventsTail != NULL) {
        app->m_eventsTail->m_next = event;
    } else {
        app->m_eventsHead = event;
    }
    app->m_eventsTail = event;
}

void WindowApp::HandleEvent(Event::EType type, int id, int param1, int param2, int param3)
{
    m_instance->m_desktop->HandleEvent(type, id, param1, param2, param3);
}

void WindowApp::HandleCommand(int id, int param1, int param2, int param3)
{
    m_instance->m_desktop->HandleCommand(id, param1, param2, param3);
}

void WindowApp::HandleTunnelCommand(int id, int param1, int param2, int param3)
{
    m_instance->m_desktop->HandleTunnelCommand(id, param1, param2, param3);
}

void WindowApp::AddModal(Window* window)
{
    m_instance->m_desktop->AddModal(window);
}

void WindowApp::OnWindowDestroyed(Window* window)
{
    for (int pointer = 0; pointer < MAX_POINTERS; pointer++) {
        if (m_pointerCapture[pointer] == window) {
            m_pointerCapture[pointer] = NULL;
        }
    }
    if (m_softkeysOwner == window) {
        m_softkeysOwner = NULL;
    }
}

Window* WindowApp::PointerCapture(int pointer)
{
    if ((unsigned int)pointer < MAX_POINTERS) {
        return m_instance->m_pointerCapture[pointer];
    }
    return NULL;
}

void WindowApp::SetPointerCapture(int pointer, Window* window)
{
    if ((unsigned int)pointer < MAX_POINTERS) {
        m_instance->m_pointerCapture[pointer] = window;
    }
}

void WindowApp::GetPointerPos(int pointer, int& x, int& y)
{
    if ((unsigned int)pointer < MAX_POINTERS) {
        WindowApp* app = m_instance;
        x = app->m_pointerPos[pointer].x;
        y = app->m_pointerPos[pointer].y;
        return;
    }
    y = 0;
    x = 0;
}

void WindowApp::GetPointerOffset(int pointer, int& dx, int& dy)
{
    if ((unsigned int)pointer < MAX_POINTERS) {
        WindowApp* app = m_instance;
        dx = app->m_pointerPos[pointer].x - app->m_pointerPrevPos[pointer].x;
        dy = app->m_pointerPos[pointer].y - app->m_pointerPrevPos[pointer].y;
        return;
    }
    dy = 0;
    dx = 0;
}

Window* WindowApp::UpdateSoftkeysCapture()
{
    WindowApp* app = m_instance;
    Window* previousOwner = app->m_softkeysOwner;
    app->m_softkeysOwner = NULL;
    app->m_softkeysOwner = m_instance->m_desktop->HandleCommand(COMMAND_FIND_SOFTKEYS_OWNER, 0, 0, 0);
    if (previousOwner != app->m_softkeysOwner) {
        m_instance->m_desktop->HandleTunnelCommand(COMMAND_SOFTKEYS_OWNER_CHANGED, (int)app->m_softkeysOwner, 0, 0);
    }
    return app->m_softkeysOwner;
}

void WindowApp::InitializeRestartApplication(int delay)
{
    m_restartPending = 1;
    m_field1c8 = 0;
    m_restartDelay = delay;
}

void WindowApp::RestartApplication()
{
    HandleDestroy();
    App::ClearGameData();
    reinit();
    HandleCreate();
    ((App*)m_instance)->m_timeScale = 1.0f;
}

void WindowApp::HandleDestroy()
{
    Stop();
    m_desktop->HandleLostFocus();
    m_desktop->DeleteChilds();
    if ((m_desktop->m_flags & Window::FLAG_INITIALIZED) != 0) {
        m_desktop->ClearFlags(Window::FLAG_INITIALIZED);
        m_desktop->Shutdown();
    }
    m_desktop->Close();
    m_desktop = NULL;
    Shutdown();
}

void WindowApp::HandleCreate()
{
    ICRenderSurface* screen = ICGraphics::GetInstance()->GetTargetSurface();
    unsigned int width;
    unsigned int height;
    screen->GetWidthAndHeight(width, height);
    m_screenWidth = width;
    m_screenHeight = height;
    Init();
    m_desktop = NewDisplayWindow();
    if ((m_desktop->m_flags & Window::FLAG_INITIALIZED) == 0) {
        m_desktop->SetFlags(Window::FLAG_INITIALIZED);
        m_desktop->Init();
    }
    if ((m_desktop->m_flags & Window::FLAG_FOCUSED) == 0) {
        m_desktop->SetFlags(Window::FLAG_FOCUSED);
        m_desktop->OnGetFocus();
    }
    m_desktop->SetFlags(Window::FLAG_NO_CLIP);
    HandleDeviceOrientationChanged();
    Start();
    HandleUpdate(1);
}

void WindowApp::HandleDeviceOrientationChanged()
{
    ICRenderSurface* screen = ICGraphics::GetInstance()->GetTargetSurface();
    unsigned int width;
    unsigned int height;
    screen->GetWidthAndHeight(width, height);
    if (m_screenWidth != (int)width || m_screenHeight != (int)height) {
        m_screenWidth = width;
        m_screenHeight = height;
        m_desktop->SetRect(0, 0, width, height, true);
    }
}

void WindowApp::HandleUpdate(int elapsed)
{
    m_frameStartTime = CStdUtil_Cocoa::GetUpTimeMS();
    int previousTime = m_time;
    m_time += elapsed;
    int total = 0;
    for (int frame = 0; frame < 7; frame++) {
        m_frameTimes[frame] = m_frameTimes[frame + 1];
        total += m_frameTimes[frame + 1];
    }
    m_frameTimes[7] = m_time - previousTime;
    m_averageFrameTime = (total + m_frameTimes[7]) >> 3;
    m_fps = 1000.0f / (float)m_averageFrameTime;
    UpdateTime();
    m_timeSeconds = (float)m_updateTime * 0.001f;
    m_frameTimeSeconds = (float)m_updateFrameTime * 0.001f;
    HandleEvents();
    m_desktop->HandleUpdate(true);
    UpdateSoftkeysCapture();
    Update();
    m_updateDuration = CStdUtil_Cocoa::GetUpTimeMS() - m_frameStartTime;
    if (m_restartPending == 0) {
        return;
    }
    m_restartPending = 0;
    RestartApplication();
}

void WindowApp::HandlePause()
{
    OnPause();
    m_desktop->HandlePause();
}

void WindowApp::HandleResume()
{
    OnResume();
    m_desktop->HandleResume();
}

void WindowApp::HandleEnterBackground()
{
    OnEnterBackground();
    m_desktop->HandleEnterBackground();
}

void WindowApp::HandleEnterForeground()
{
    OnEnterForeground();
    m_desktop->HandleEnterForeground();
}

/** Index of the pointer of `event` (looked up by id when the event has none). */
static inline int FindPointer(WindowApp* app, Event& event)
{
    if (event.m_param3 < 0) {
        for (int pointer = 0; pointer < WindowApp::MAX_POINTERS; pointer++) {
            if (app->m_pointerIds[pointer] == event.m_id) {
                event.m_param3 = pointer;
                break;
            }
        }
    }
    return event.m_param3;
}

/** Remember the previous position of the pointer of `event` and store its new position. */
static inline void MovePointer(WindowApp* app, Event& event)
{
    int pointer = event.m_param3;
    app->m_pointerPrevPos[pointer] = app->m_pointerPos[pointer];
    app->m_pointerPos[event.m_param3].x = event.m_param1;
    app->m_pointerPos[event.m_param3].y = event.m_param2;
}

void WindowApp::HandleEvents()
{
    int startTime = CStdUtil_Cocoa::GetUpTimeMS();
    // Keep at most 4 recycled events.
    while (m_freeEventCount > 4) {
        Event* unused = m_freeEvents;
        m_freeEvents = unused->m_next;
        m_freeEventCount--;
        unused->m_next = NULL;
        delete unused;
    }
    Event* event = m_currentEvents;
    if (event == NULL) {
        event = m_eventsHead;
        m_currentEvents = event;
        m_eventsHead = NULL;
        m_eventsTail = NULL;
    }
    while (event != NULL) {
        m_currentEvents = event->m_next;
        event->m_next = NULL;
        switch (event->m_type) {
        case Event::KEY_PRESSED:
            OnKeyPressed(*event);
            break;
        case Event::KEY_RELEASED:
            OnKeyReleased(*event);
            break;
        case Event::POINTER_PRESSED: {
            int pointer = event->m_param3;
            if (pointer < 0) {
                // A known pointer id, else a free slot, else the oldest slot.
                int slot;
                for (slot = 0; slot < MAX_POINTERS; slot++) {
                    if (m_pointerIds[slot] == event->m_id) {
                        m_pointerTimes[slot] = m_updateTime;
                        event->m_param3 = slot;
                        pointer = slot;
                        break;
                    }
                }
                if (slot == MAX_POINTERS) {
                    int freeSlot;
                    for (freeSlot = 0; freeSlot < MAX_POINTERS; freeSlot++) {
                        if (m_pointerCapture[freeSlot] == NULL) {
                            m_pointerTimes[freeSlot] = m_updateTime;
                            m_pointerIds[freeSlot] = event->m_id;
                            event->m_param3 = freeSlot;
                            pointer = freeSlot;
                            break;
                        }
                    }
                    if (freeSlot == MAX_POINTERS) {
                        int oldest = 0;
                        int oldestTime = 0x7fffffff;
                        for (int candidate = 0; candidate < MAX_POINTERS; candidate++) {
                            if (oldestTime > m_pointerTimes[candidate]) {
                                oldest = candidate;
                                oldestTime = m_pointerTimes[candidate];
                            }
                        }
                        m_pointerTimes[oldest] = m_updateTime;
                        m_pointerIds[oldest] = event->m_id;
                        event->m_param3 = oldest;
                        pointer = oldest;
                    }
                }
            }
            if ((unsigned int)pointer < MAX_POINTERS) {
                m_pointerCapture[pointer] = NULL;
                MovePointer(this, *event);
                OnPointerPressed(*event);
            } else {
                event->Clear();
            }
            break;
        }
        case Event::POINTER_DRAGGED: {
            int pointer = FindPointer(this, *event);
            if ((unsigned int)pointer < MAX_POINTERS) {
                MovePointer(this, *event);
                OnPointerDragged(*event);
            } else {
                event->Clear();
            }
            break;
        }
        case Event::POINTER_RELEASED: {
            int pointer = FindPointer(this, *event);
            if ((unsigned int)pointer < MAX_POINTERS) {
                MovePointer(this, *event);
                int releasedPointer = event->m_param3;
                OnPointerReleased(*event);
                if (event->m_type != Event::NONE) {
                    m_desktop->HandleEvent(*event);
                    event->Clear();
                }
                m_pointerCapture[releasedPointer] = NULL;
            } else {
                event->Clear();
            }
            break;
        }
        case Event::COMMAND:
        case Event::TUNNEL_COMMAND:
            if (event->m_id == COMMAND_END_OF_FRAME) {
                // The rest of the queue is handled next frame.
                event->m_next = m_freeEvents;
                m_freeEvents = event;
                m_freeEventCount++;
                m_eventsDuration = CStdUtil_Cocoa::GetUpTimeMS() - startTime;
                return;
            }
            OnCommand(*event);
            break;
        default:
            break;
        }
        if (event->m_type != Event::NONE) {
            m_desktop->HandleEvent(*event);
        }
        event->m_next = m_freeEvents;
        m_freeEvents = event;
        m_freeEventCount++;
        event = m_currentEvents;
    }
    m_eventsDuration = CStdUtil_Cocoa::GetUpTimeMS() - startTime;
}

void WindowApp::UpdateTime()
{
    m_updateTime = m_time;
    m_updateFrameTime = m_frameTimes[7];
}

float WindowApp::Opacity() const
{
    return m_colorStack.m_data[m_colorStack.m_size - 1].a;
}

const Color_RGBA& WindowApp::Color() const
{
    return m_colorStack.m_data[m_colorStack.m_size - 1];
}

Color_RGBA& WindowApp::PushColor(Color_RGBA& color)
{
    color.a = Opacity() * color.a;
    m_colorStack.addElement(color);
    return color;
}

float WindowApp::PushOpacity(float opacity)
{
    Color_RGBA color = Color();
    color.a *= opacity;
    m_colorStack.addElement(color);
    return color.a;
}

float WindowApp::PushOpacityExact(float opacity)
{
    Color_RGBA color = Color();
    color.a = opacity;
    m_colorStack.addElement(color);
    return color.a;
}

void WindowApp::PopColorStack()
{
    m_colorStack.m_size--;
}

float WindowApp::WinScale() const
{
    return m_scaleStack.m_data[m_scaleStack.m_size - 1];
}

float WindowApp::PushWinScale(float scale)
{
    float newScale = WinScale() * scale;
    m_scaleStack.addElement(newScale);
    return newScale;
}

void WindowApp::PopWinScale()
{
    m_scaleStack.m_size--;
}

const Window::Point& WindowApp::Translation() const
{
    return m_translationStack.m_data[m_translationStack.m_size - 1];
}

const Window::Point& WindowApp::PushTranslation(int dx, int dy)
{
    const Window::Point& current = Translation();
    int currentY = current.y;
    Window::Point point;
    point.x = current.x + dx;
    point.y = currentY + dy;
    m_translationStack.addElement(point);
    return Translation();
}

void WindowApp::PopTranslation()
{
    m_translationStack.m_size--;
}

void WindowApp::HandlePaint()
{
    int startTime = CStdUtil_Cocoa::GetUpTimeMS();
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    ICRenderSurface* screen = ICGraphics::GetInstance()->GetTargetSurface();
    graphics->LoadIdentity();

    m_colorStack.m_size = 0;
    Color_RGBA white;
    white.r = 1.0f;
    white.g = 1.0f;
    white.b = 1.0f;
    white.a = 1.0f;
    m_colorStack.addElement(white);
    m_scaleStack.m_size = 0;
    float scale = 1.0f;
    m_scaleStack.addElement(scale);
    m_translationStack.m_size = 0;
    Window::Point origin;
    origin.x = 0;
    origin.y = 0;
    m_translationStack.addElement(origin);
    m_paintQueue.m_size = 0;

    unsigned int width;
    unsigned int height;
    screen->GetWidthAndHeight(width, height);
    ICGraphics::GetInstance()->GetRasterizerState()->SetScissor(0, 0, (short)width, (short)height);
    if (m_screenWidth != (int)width || m_screenHeight != (int)height) {
        m_screenWidth = width;
        m_screenHeight = height;
        m_desktop->SetRect(0, 0, width, height, true);
    }
    m_desktop->HandlePaint(*graphics);
    Paint(*graphics);
    m_paintDuration = CStdUtil_Cocoa::GetUpTimeMS() - startTime;
}

void WindowApp::Shutdown()
{
    ClearAllEvents();
    Event* event = m_freeEvents;
    while (event != NULL) {
        Event* next = event->m_next;
        event->m_next = NULL;
        delete event;
        event = next;
    }
    m_freeEvents = NULL;
    m_freeEventCount = 0;
}

int WindowApp::RemapKey(int key)
{
    return key;
}

SoftkeysContainer::SoftkeysContainer(Window* owner)
    : Window(false)
{
    ClearFlags(FLAG_FOCUSABLE | FLAG_CHILDREN_FIRST);
    SetFlags(FLAG_EVENTS_WHILE_SHOWING | FLAG_EVENTS_WHILE_CLOSING);
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(ALIGN_NONE);
    m_owner = owner;
    m_active = true;
}

void SoftkeysContainer::Init()
{
}

void SoftkeysContainer::OnClosed()
{
}

void SoftkeysContainer::Shutdown()
{
}

void SoftkeysContainer::Update()
{
    if (m_owner == NULL) {
        return;
    }
    if (!m_parent->IsChildExisting(m_owner)) {
        Close();
    }
}

void SoftkeysContainer::OnCommand(Event& event)
{
    switch (event.m_id) {
    case WindowApp::COMMAND_FIND_SOFTKEYS_OWNER:
        // Claim the soft keys while the owner window exists, else go away.
        if (m_owner != NULL && !m_parent->IsChildExisting(m_owner)) {
            m_active = false;
            Close();
        } else {
            event.Clear();
        }
        break;
    case WindowApp::COMMAND_SOFTKEYS_OWNER_CHANGED:
        if ((SoftkeysContainer*)event.m_param1 == this) {
            SetFlags(FLAG_PAINT_CHILDREN | FLAG_EVENTS_TO_CHILDREN);
        } else {
            ClearFlags(FLAG_PAINT_CHILDREN | FLAG_EVENTS_TO_CHILDREN);
        }
        break;
    }
}

void SoftkeysContainer::Paint(ICGraphics2d& graphics)
{
}
