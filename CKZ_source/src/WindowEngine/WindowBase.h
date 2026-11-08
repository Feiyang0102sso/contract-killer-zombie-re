// WindowBase.h - the game's window tree: Window (a rectangle with children, layout, easing
// animations, a state stack and event handling), its events and the resource handles it draws with.
// Owner: src/WindowEngine/WindowBase.cpp (b045x; first draft by b057x).
// Virtual slot order from the vtable of Window; field offsets from Window::Window(bool) and the
// accessors in WindowBase.cpp / WindowApp.cpp.
#ifndef WINDOWBASE_H
#define WINDOWBASE_H

#include <stddef.h>
#include <wchar.h>
#include "xString.h"
#include "Vector_ss.h"
#include "CResource.h"
#include "CRectangle.h"
#include "Interpolation.h"
#include "ICGraphics.h"
#include "ICRasterizerState.h"

namespace com { namespace glu { namespace platform {
namespace components {
struct Color_RGBA;
}
namespace systems {
class CResource;
}
namespace graphics {
class ICGraphics2d;
class ICRenderSurface;
class CFont;
}
} } }

namespace swerve {
template <class ErrorHandler> class Graphics3D;
}
class CSwerveAssertingErrorHandler;
class Window;

/** A window event: queued by WindowApp (linked list) or built on the stack and dispatched. */
class Event {
public:
    /** Event kinds; the key ids reuse the platform event ids. */
    enum EType {
        NONE = 0,
        KEY_PRESSED = (int)0xc7424be6,
        KEY_RELEASED = 0x43a39818,
        POINTER_PRESSED = 0x43a39819,
        POINTER_DRAGGED = 0x43a3981a,
        POINTER_RELEASED = 0x43a3981b,
        COMMAND = 0x43a3981c,
        TUNNEL_COMMAND = 0x43a3981d,     // command sent to every child (not only the focused ones)
        WINDOW_NOTIFY = 0x43a3981e       // window notifications for the event listeners
    };

    Event(EType type, int id, int param1, int param2, int param3)
    {
        m_type = type;
        m_id = id;
        m_param1 = param1;
        m_param2 = param2;
        m_param3 = param3;
        m_handler = NULL;
        m_next = NULL;
        m_handled = false;
    }

    /** Mark the event as consumed (type 0: nobody handles it any more). */
    void Clear()
    {
        m_type = NONE;
        m_id = 0;
        m_param1 = 0;
        m_param2 = 0;
        m_param3 = 0;
        m_handler = NULL;
        m_handled = false;
    }

    Event* m_next;          // 0x00 queue link
    EType m_type;           // 0x04 NONE once consumed
    bool m_handled;         // 0x08 a pointer event outside the window: do not pass it to the children
    int m_id;               // 0x0c key code / command id / pointer id
    int m_param1;           // 0x10 x / raw key / first command parameter
    int m_param2;           // 0x14 y / remapped key / second command parameter
    int m_param3;           // 0x18 pointer index / third command parameter
    Window* m_handler;      // 0x1c window that consumed the event
};

/** Counted handle of a resource of the application's resource manager. */
class SmartRes {
public:
    SmartRes();
    SmartRes(const char* name);
    SmartRes(const SmartRes& other);
    virtual ~SmartRes();

    SmartRes& operator=(const SmartRes& other);

    /** Acquire the resource `name`; returns true when it exists. */
    bool Create(const char* name);
    /** Give the resource back. */
    void Release();
    /** Name of the resource (NULL without one). */
    const char* GetResourceId() const;

    com::glu::platform::systems::CResource* m_resource;     // 0x04
};

/** Handle of a render surface resource. */
class ImageRes : public SmartRes {
public:
    // b074i: Tab CreateInfo original 121878 calls SmartRes(name) and then sets the ImageRes address point.
    ImageRes(const char* name) : SmartRes(name) {}
    ImageRes()
    {
    }

    virtual ~ImageRes()
    {
    }

    ImageRes& operator=(const ImageRes& other);

    /** The surface of the resource (NULL without one). */
    com::glu::platform::graphics::ICRenderSurface* GetSurface() const
    {
        if (m_resource != NULL) {
            return (com::glu::platform::graphics::ICRenderSurface*)m_resource->GetData();
        }
        return NULL;
    }
};

/** Handle of a media (sound) resource. */
class MediaRes : public SmartRes {
public:
    MediaRes()
    {
    }

    MediaRes(const char* name)  // added by b057x
        : SmartRes(name)  // added by b057x
    {  // added by b057x
    }  // added by b057x

    virtual ~MediaRes()
    {
    }
};

class Window {
public:
    /** How the window is placed by its parent. */
    enum ELayoutType {
        LAYOUT_NONE = 0,
        LAYOUT_SIMPLE = 1,      // placed by its alignment inside the parent
        LAYOUT_TABLE = 2        // placed in a cell of the parent's table
    };

    /** Unit of a UnitValue. */
    enum EUnit {
        UNIT_NONE = 0,
        UNIT_PIXELS = 1,
        UNIT_PERCENT = 2,
        UNIT_CONTENT = 3        // computed from the content (children)
    };

    /** Alignment bits (m_align and the align argument of the Draw* helpers). */
    enum EAlign {
        ALIGN_LEFT = 0x01,
        ALIGN_RIGHT = 0x02,
        ALIGN_HCENTER = 0x04,
        ALIGN_TOP = 0x08,
        ALIGN_BOTTOM = 0x10,
        ALIGN_VCENTER = 0x20,
        ALIGN_NONE = 0x40       // keep the position (no alignment)
    };

    /** Window flags (m_flags). Names are inferred from their use in WindowBase.cpp. */
    enum EFlags {
        FLAG_VISIBLE = 0x00000001,
        FLAG_UPDATE = 0x00000002,           // Update() is called every frame
        FLAG_RECT_PENDING = 0x00000004,     // got its rectangle from the parent's layout this pass
        FLAG_EVENTS_WHILE_SHOWING = 0x00000008, // gets events while easing in (FLAG_SHOW_AFTER_EASY)
        FLAG_EVENTS_WHILE_CLOSING = 0x00000010, // gets events while easing out (FLAG_CLOSE_AFTER_EASY)
        FLAG_CHILDREN_FIRST = 0x00000020,   // events go to the children before the window
        FLAG_FOCUSABLE = 0x00000040,
        FLAG_MODAL = 0x00000080,            // no events for the windows behind
        FLAG_SHOW_AFTER_EASY = 0x00000100,
        FLAG_CLOSE_AFTER_EASY = 0x00000200,
        FLAG_CLOSING = 0x00000400,
        FLAG_STARTING = 0x00000800,         // waits until it is the front window
        FLAG_PAINT_3D = 0x00001000,         // painted by RunPaint3D / Paint3D
        FLAG_ENABLED = 0x00002000,          // handles events
        FLAG_EVENTS_TO_CHILDREN = 0x00004000,
        FLAG_REMAPPED_KEYS = 0x00020000,    // key events use the remapped key code
        FLAG_DELETE_CHILDREN = 0x00040000,
        FLAG_PAINT_CHILDREN = 0x00080000,
        FLAG_POINTER_HIT_TEST = 0x00100000, // pointer presses outside IsLocalPos are not handled
        FLAG_POINTER_EXCLUSIVE = 0x00200000,    // only the capturing pointer is handled
        FLAG_NO_CLIP = 0x00400000,
        FLAG_CLIP_SCREEN = 0x00800000,
        FLAG_HIT_OUTSET = 0x01000000,       // IsLocalPos includes the outset spacing
        FLAG_LAYOUT_DIRTY = 0x02000000,
        FLAG_IN_SET_RECT = 0x04000000,
        FLAG_IN_EASY = 0x08000000,
        FLAG_INITIALIZED = 0x10000000,
        FLAG_FOCUSED = 0x20000000,
        FLAG_SHUT_DOWN = 0x40000000,
        FLAG_DEAD = (int)0x80000000         // deleted by the parent's next HandleUpdate
    };

    /** Initial flags of a new window. */
    enum { DEFAULT_FLAGS = 0x003e6163 };

    /** How Update() moves through the state stack (ChangeState). */
    enum eStateTransitionType {
        STATE_NONE = 0,
        STATE_PUSH = 1,
        STATE_REPLACE = 2,
        STATE_RESET = 3,        // pop everything, then push
        STATE_POP = 4,
        STATE_GOTO = 5          // pop down to the state (push it when not found)
    };

    /** A position. */
    struct Point {
        int x;
        int y;
    };

    /** Parameters and running state of an easing animation (position / size / opacity / scale). */
    struct EasyParams {
        EasyParams(Window* window, EasyLib::EMethod method);
        EasyParams(Window* window, EasyLib::EMethod method, EasyLib::EMethod reverseMethod);

        /** Start from the current rectangle of `window` with no movement, full opacity and scale. */
        void Init(Window* window, EasyLib::EMethod method, EasyLib::EMethod reverseMethod);
        void SetFullTime(float time);
        void SetFullTimes(float time, float reverseTime);
        void SetDelayTime(float delay);
        void SetDelayTimes(float delayX, float delayY);
        void SetMoving(int fromX, int fromY, int toX, int toY);
        void SetOpacity(float from, float to);
        void SetScale(float from, float to);
        /** Value of one coordinate at `time`; values[0] / values[1] are the from / to values. */
        int RunEasy(EasyLib::EMethod* methods, float time, short* values, float overshoot);
        float RunOpacity(float time);
        float RunScale(float time);

        short m_fromX;                  // 0x00
        short m_toX;                    // 0x02
        short m_fromY;                  // 0x04
        short m_toY;                    // 0x06
        short m_fromWidth;              // 0x08
        short m_toWidth;                // 0x0a
        short m_fromHeight;             // 0x0c
        short m_toHeight;               // 0x0e
        float m_fullTime;               // 0x10 seconds
        float m_reverseTime;            // 0x14
        float m_delayX;                 // 0x18
        float m_delayY;                 // 0x1c
        float m_overshootX;             // 0x20 parameter of the easing method
        float m_overshootY;             // 0x24
        float m_opacityFrom;            // 0x28
        float m_opacityTo;              // 0x2c
        float m_opacityDelay;           // 0x30
        float m_opacityCut;             // 0x34 the opacity runs this much shorter than the movement
        float m_opacityStart;           // 0x38 EasyTime() of the opacity start
        float m_scaleFrom;              // 0x3c
        float m_scaleTo;                // 0x40
        float m_scaleDelay;             // 0x44
        float m_scaleCut;               // 0x48
        float m_scaleStart;             // 0x4c
        int m_clearFlagsOnStart;        // 0x50
        int m_clearFlagsOnFinish;       // 0x54
        int m_setFlagsOnStart;          // 0x58
        int m_setFlagsOnFinish;         // 0x5c
        int m_userData;                 // 0x60
        float m_startX;                 // 0x64 EasyTime() of the horizontal start
        float m_startY;                 // 0x68
        bool m_moving;                  // 0x6c position changes
        bool m_resizing;                // 0x6d size changes
        EasyLib::EMethod m_methodX[2];          // 0x70 forward / reverse
        EasyLib::EMethod m_methodY[2];          // 0x78
        EasyLib::EMethod m_methodOpacity[2];    // 0x80
        EasyLib::EMethod m_methodScale[2];      // 0x88
        int m_state;                    // 0x90 0 idle, 1 forward, 2 reverse
        bool m_started;                 // 0x94 OnEasyStarted() was called
    };

    /** Width / height rule of the layout: a value with limits and its unit. */
    struct UnitValue {
        UnitValue()
        {
            m_unit = UNIT_NONE;
            m_value = 0;
            m_min = 0;
            m_max = 0;
        }

        UnitValue(int value, int min, int max, int unit)
        {
            m_min = (short)min;
            m_max = (short)max;
            m_value = (short)value;
            m_unit = unit;
        }

        short m_value;      // 0x00 pixels / percent
        short m_min;        // 0x02
        short m_max;        // 0x04 0: no limit
        int m_unit;         // 0x08 EUnit
    };

    /** Spacing around the window (outset) or around its content (inset), CSS order. */
    struct Spacing {
        Spacing()
        {
            m_top = 0;
            m_right = 0;
            m_bottom = 0;
            m_left = 0;
        }

        Spacing(int top, int right, int bottom, int left)
        {
            m_top = (short)top;
            m_right = (short)right;
            m_bottom = (short)bottom;
            m_left = (short)left;
        }

        short m_top;        // 0x00
        short m_right;      // 0x02
        short m_bottom;     // 0x04
        short m_left;       // 0x06
    };

    /** Layout settings (allocated on first use). */
    struct Layout {
        Layout()
        {
            m_type = LAYOUT_NONE;
            m_cellX = 0;
            m_cellY = 0;
            m_colSpan = 0;
            m_rowSpan = 0;
        }

        /** Table layout: the children with LAYOUT_TABLE placed in a grid of cells. */
        class Table {
        public:
            /** One cell of a row. */
            struct Cell {
                // User constructor: Vector<Cell> copies elements without clearing first (as in the original).  // added by fx2
                Cell()  // added by fx2
                {  // added by fx2
                    m_window = NULL;  // added by fx2
                }  // added by fx2

                Window* m_window;
            };

            /** One row: its cells. */
            struct Row {
                Vector<Cell> m_cells;
            };

            /** Accumulated size rules of one column / row. */
            struct UnitValues {
                UnitValues()
                {
                    m_min = 0;
                    m_max = 0;
                    m_pixels = 0;
                    m_percent = 0;
                    m_size = 0;
                }

                /** Merge `value` (plus `spacing`) into the rules; true when something changed. */
                bool Add(const UnitValue& value, int spacing);

                short m_min;        // 0x00
                short m_max;        // 0x02 0: no limit
                short m_pixels;     // 0x04 largest fixed size
                float m_percent;    // 0x08 largest percentage (0..1)
                int m_size;         // 0x0c result of BalanceDim
            };

            Table()
            {
                m_columnCount = 0;
            }

            ~Table();

            void Reset();
            Cell& SetCell(int column, int row, const Cell& cell);
            /** Put the table-layouted children of `window` into their cells. */
            void PopulateCellWindows(Window* window);
            /** Distribute `size` pixels over `values`; returns the size used. */
            int BalanceDim(int size, Vector<UnitValues>& values, bool byContent);
            /** Collect the size rules of the cells into m_columns / m_rows. */
            void PopulateWidthAndHeight(int width, int height);
            /** Lay out the children of `window` in a width x height area. */
            void Reflow(Window* window, int width, int height, bool force);

            Vector<Row*> m_rowList;             // 0x00
            int m_columnCount;                  // 0x10
            Vector<UnitValues> m_columns;       // 0x14
            Vector<UnitValues> m_rows;          // 0x24
            int m_width;                        // 0x34 result of Reflow
            int m_height;                       // 0x38
        };

        Spacing m_outset;           // 0x00
        Spacing m_inset;            // 0x08
        UnitValue m_width;          // 0x10
        UnitValue m_height;         // 0x1c
        int m_type;                 // 0x28 ELayoutType
        unsigned char m_cellX;      // 0x2c table cell
        unsigned char m_cellY;      // 0x2d
        unsigned char m_colSpan;    // 0x2e
        unsigned char m_rowSpan;    // 0x2f
    };

    /** Callback object of AddEventListener: notified of the events dispatched to a window. */
    class EventListener {
    public:
        virtual ~EventListener()
        {
        }

        /** Called with the window the event was dispatched to. */
        virtual void OnEvent(Window* sender, Event& event) = 0;

        Window* m_owner;            // 0x04 window that registered the listener
        int m_id;                   // 0x08 event id filter (0: any)
        Event::EType m_type;        // 0x0c NONE: removed, freed by HandleFreeEventListeners
        EventListener* m_next;      // 0x10
    };

    /** Saves the scissor rectangle of the device and restores it when going out of scope. */
    struct SaveClipRect {
        SaveClipRect(com::glu::platform::graphics::ICGraphics2d& graphics)
        {
            m_graphics = &graphics;
            m_clip.m_x = 0;
            m_clip.m_y = 0;
            m_clip.m_width = 0;
            m_clip.m_height = 0;
            unsigned char enabled;
            com::glu::platform::graphics::ICGraphics::GetInstance()->GetRasterizerState()->GetScissorState(
                enabled, m_clip.m_x, m_clip.m_y, m_clip.m_width, m_clip.m_height);
        }

        ~SaveClipRect()
        {
            com::glu::platform::graphics::ICGraphics::GetInstance()->GetRasterizerState()->SetScissor(
                m_clip.m_x, m_clip.m_y, m_clip.m_width, m_clip.m_height);
        }

        com::glu::platform::graphics::ICGraphics2d* m_graphics;     // 0x00
        com::glu::platform::components::CRectangle<short> m_clip;   // 0x04
    };

    /** One entry of the state stack. */
    struct State {
        int m_id;                   // 0x00
        Window* m_window;           // 0x04
    };

    enum { MAX_STATES = 10 };

    Window(bool debugFill);
    virtual ~Window();

    virtual float EasyTime();
    virtual void StartEasy(const EasyParams& params, int flags);
    virtual void ReverseEasy(int flags);
    virtual void StopEasy();
    virtual void ResetEasy();
    virtual void UpdateEasy();
    virtual void OnLayoutEasy(int x, int y, int width, int height);
    virtual void OnStartupEasy() {}
    virtual void OnEasyStarted() {}
    virtual void OnEasyFinished() {}
    virtual void OnEasyStopped() {}
    virtual bool IsLocalPos(int x, int y) const;
    virtual void SetRect(int x, int y, int width, int height, bool force);
    virtual void OnRectChanged() {}
    virtual void OnSizeChanged() {}
    virtual void OnSetRectFinished() {}
    virtual void Init() {}
    virtual void Shutdown() {}
    virtual void OnClosed() {}
    virtual void OnGetFocus() {}
    virtual void OnLostFocus() {}
    virtual void OnLayout() {}
    virtual void OnKeyPressed(Event& event) {}
    virtual void OnKeyReleased(Event& event) {}
    virtual void OnCommand(Event& event) {}
    virtual void OnUnhandledEvent(Event& event) {}
    virtual void OnPointerEvent(Event& event) {}
    virtual void OnPointerPressed(Event& event) {}
    virtual void OnPointerDragged(Event& event) {}
    virtual void OnPointerReleased(Event& event) {}
    virtual void Update();
    virtual void OnChildInitialized(Window* child) {}
    virtual void OnChildRectChanged(Window* child) {}
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics) {}
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void OnEnterBackground() {}
    virtual void OnEnterForeground() {}
    virtual void SetInterrupt(int interrupt);
    virtual int GetInterrupt();
    virtual void ClearInterrupt();
    virtual int ShouldBeModal() { return 0; }
    virtual void OnCreateState(Window** window, int state) {}
    virtual void OnStateInterrupt(int state, Window* window) {}

    void SetLayoutType(ELayoutType type);
    ELayoutType LayoutType() const;
    void Close();
    void ClearAndSetFlags(int clearFlags, int setFlags);
    /** Close this window if it is still in the window tree. */
    void CloseSafely();
    /** Close `window` when it is somewhere below this window; returns 1 then. */
    int CloseWindowIfExists(Window* window);
    bool IsChildExisting(Window* window);
    /** 1 when `window` is somewhere below this window. */
    int IsWindowExisting(Window* window);
    bool IsChildOf(Window* window);
    int CloseChildIfExists(Window* window);
    void CloseChilds();
    void DeleteChilds();
    void HandleLostFocus();
    void ShutdownEventListeners();
    int LocalToGlobalX(int x) const;
    int LocalToGlobalY(int y) const;
    void LocalToGlobal(int& x, int& y) const;
    void GlobalToLocal(int& x, int& y) const;
    bool IsVisible() const;
    void AddModal(Window* child);
    void SetFlags(int flags);
    void AddToFront(Window* child);
    void AddToFront(Window* child, int cellX, int cellY);
    void SetCellPos(int cellX, int cellY, int colSpan, int rowSpan);
    void LayoutChanged();
    void Remove(Window* child);
    void ClearFlags(int flags);
    void SetAlign(int align);
    void SetOpacity(float opacity);
    void SetColor(com::glu::platform::components::Color_RGBA& color);
    void SetScale(float scale);
    void SetCorner(int x, int y);
    void SetCenter(int x, int y);
    void SetLayoutWidth(const UnitValue& width);
    void SetDesiredWidth(int width);
    void SetDesiredWidth(int width, int minWidth, int maxWidth);
    void SetPercentWidth(int percent, int minWidth, int maxWidth);
    void SetWidthByContent(int minWidth, int maxWidth);
    void SetLayoutHeight(const UnitValue& height);
    void SetDesiredHeight(int height);
    void SetDesiredHeight(int height, int minHeight, int maxHeight);
    void SetPercentHeight(int percent, int minHeight, int maxHeight);
    void SetHeightByContent(int minHeight, int maxHeight);
    void SetOutsetSpacing(const Spacing& spacing);
    void SetOutsetSpacing(int spacing);
    void SetOutsetSpacing(int vertical, int horizontal);
    void SetOutsetSpacing(int top, int right, int bottom, int left);
    void SetInsetSpacing(const Spacing& spacing);
    void SetInsetSpacing(int spacing);
    void SetInsetSpacing(int vertical, int horizontal);
    void SetInsetSpacing(int top, int right, int bottom, int left);
    void FreeLayout();
    void ComputeContentRect(int& x, int& y, int& width, int& height, bool inner);
    void MoveSimpleLayoutedChilds(int dx, int dy, bool force);
    void ComputeSizeByContent(int& width, int& height, bool all);
    void LayoutTable(bool force);
    /** Send a WINDOW_NOTIFY style event (`type`, `id`) to the event listeners. */
    int DispatchEventListeners(Event::EType type, int id);
    void SetSize(int width, int height);
    /** Give `event` to the listeners; returns 1 when one of them consumed it. */
    int DispatchEventListeners(Event& event);
    /** Remove the matching listeners (0 / NULL arguments match anything). */
    void RemoveEventListener(Window* owner, Event::EType type, int id);
    void HandleFreeEventListeners();
    void RemoveOwnerEventListener_r(Window* owner);
    int DispatchEvent(Event& event);
    Window* HandleEvent(Event::EType type, int id, int param1, int param2, int param3);
    Window* HandleEvent(Event& event);
    Window* HandleCommand(int id, int param1, int param2, int param3);
    Window* HandleTunnelCommand(int id, int param1, int param2, int param3);
    void HandleUpdate(bool updateSelf);
    void HandlePause();
    void HandleResume();
    void HandleEnterBackground();
    void HandleEnterForeground();
    void HandleLayout();
    void PaintChildren(com::glu::platform::graphics::ICGraphics2d& graphics);
    void HandlePaint(com::glu::platform::graphics::ICGraphics2d& graphics);
    void RunPaint3D(com::glu::platform::graphics::ICGraphics2d& graphics);
    /** Intersect the scissor rectangle with a rectangle of this window; true when nothing is left. */
    bool ClipRect(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y, int width, int height);

    static void GetImageSize(com::glu::platform::graphics::ICRenderSurface* surface, int& width, int& height);
    static int ImageWidth(com::glu::platform::graphics::ICRenderSurface* surface);
    static int ImageHeight(com::glu::platform::graphics::ICRenderSurface* surface);
    void DrawFontText(com::glu::platform::graphics::ICGraphics2d& graphics, com::glu::platform::graphics::CFont* font,
                      const wchar_t* text, int x, int y, int align);
    void FillRect(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y, int width, int height,
                  unsigned int color);
    void BeginBlend(com::glu::platform::graphics::ICGraphics2d& graphics, float opacity, float green, float blue,
                    float alpha, bool additive, bool exact);
    void EndBlend(com::glu::platform::graphics::ICGraphics2d& graphics);
    void DrawImage(com::glu::platform::graphics::ICGraphics2d& graphics,
                   com::glu::platform::graphics::ICRenderSurface* surface, int x, int y, int align, int transform);
    void DrawTileImageRect(com::glu::platform::graphics::ICGraphics2d& graphics,
                           com::glu::platform::graphics::ICRenderSurface* surface, int x, int y, int width, int height);
    void DrawTileImageRect(com::glu::platform::graphics::ICGraphics2d& graphics,
                           com::glu::platform::graphics::ICRenderSurface* surface);
    void DrawTileImageHoriz(com::glu::platform::graphics::ICGraphics2d& graphics,
                            com::glu::platform::graphics::ICRenderSurface* surface, int x, int y, int width);
    void DrawLineImageHoriz(com::glu::platform::graphics::ICGraphics2d& graphics,
                            com::glu::platform::graphics::ICRenderSurface* left,
                            com::glu::platform::graphics::ICRenderSurface* middle,
                            com::glu::platform::graphics::ICRenderSurface* right, int x, int y, int width);
    void DrawRectImageHoriz(com::glu::platform::graphics::ICGraphics2d& graphics,
                            com::glu::platform::graphics::ICRenderSurface* left,
                            com::glu::platform::graphics::ICRenderSurface* middle,
                            com::glu::platform::graphics::ICRenderSurface* right, int x, int y, int width, int height);
    void DrawTileImageVert(com::glu::platform::graphics::ICGraphics2d& graphics,
                           com::glu::platform::graphics::ICRenderSurface* surface, int x, int y, int height);
    void DrawLineImageVert(com::glu::platform::graphics::ICGraphics2d& graphics,
                           com::glu::platform::graphics::ICRenderSurface* top,
                           com::glu::platform::graphics::ICRenderSurface* middle,
                           com::glu::platform::graphics::ICRenderSurface* bottom, int x, int y, int height);
    void DrawBorderFillRect(com::glu::platform::graphics::ICGraphics2d& graphics,
                            com::glu::platform::graphics::ICRenderSurface* left,
                            com::glu::platform::graphics::ICRenderSurface* topLeft,
                            com::glu::platform::graphics::ICRenderSurface* top,
                            com::glu::platform::graphics::ICRenderSurface* topRight,
                            com::glu::platform::graphics::ICRenderSurface* right,
                            com::glu::platform::graphics::ICRenderSurface* bottomRight,
                            com::glu::platform::graphics::ICRenderSurface* bottom,
                            com::glu::platform::graphics::ICRenderSurface* bottomLeft,
                            com::glu::platform::graphics::ICRenderSurface* center, int x, int y, int width, int height);
    void DrawVertScrollbar(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y, int height,
                           com::glu::platform::graphics::ICRenderSurface* top,
                           com::glu::platform::graphics::ICRenderSurface* bottom,
                           com::glu::platform::graphics::ICRenderSurface* track,
                           com::glu::platform::graphics::ICRenderSurface* thumb, float position, float size,
                           bool alwaysShowEnds);
    void DrawVertScrollbar(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y, int height,
                           com::glu::platform::graphics::ICRenderSurface* top,
                           com::glu::platform::graphics::ICRenderSurface* bottom,
                           com::glu::platform::graphics::ICRenderSurface* track,
                           com::glu::platform::graphics::ICRenderSurface* thumb,
                           com::glu::platform::graphics::ICRenderSurface* thumbTop,
                           com::glu::platform::graphics::ICRenderSurface* thumbBottom, float position, float size,
                           bool alwaysShowEnds);

    /** The localized string resource `name`. */
    static XString ResString(const char* name);
    /** `name` looked up as a string resource; an empty name gives an empty string. */
    static XString ResStringSafe(XString name);

    void fireInterrupt();
    void PopState();
    void PushState(int state, Window* window);
    void GoToState(int state);
    void AddNewState(int state);
    void ChangeState(int state, eStateTransitionType transition);
    int HasModalChild();

    /** The layout settings, created on first use. */
    inline Layout* GetLayout()
    {
        if (m_layout == NULL) {
            m_layout = new Layout();
        }
        return m_layout;
    }

    Window* m_parent;           // 0x04
    Window* m_nextSibling;      // 0x08
    Window* m_firstChild;       // 0x0c
    Window* m_focusChild;       // 0x10 child that has the focus
    short m_x;                  // 0x14 relative to the parent
    short m_y;                  // 0x16
    short m_width;              // 0x18
    short m_height;             // 0x1a
    int m_flags;                // 0x1c EFlags
    int m_align;                // 0x20 EAlign bits
    com::glu::platform::components::Color_RGBA* m_tint;    // 0x24 color (NULL: m_opacity only)
    float m_opacity;            // 0x28
    float m_scale;              // 0x2c
    EasyParams* m_easy;         // 0x30 easing animation (NULL: none yet)
    Layout* m_layout;           // 0x34
    int m_listenerCount;        // 0x38 listeners this window registered on other windows
    EventListener* m_eventListeners;    // 0x3c
    int m_interrupt;            // 0x40
    bool m_debugFill;           // 0x44 debug: blink-fill the window
    bool m_debugFrame;          // 0x45 debug: draw the frame
    State m_states[MAX_STATES]; // 0x48
    int m_stateCount;           // 0x98
    int m_currentState;         // 0x9c -1: none
    int m_nextState;            // 0xa0 pending ChangeState
    int m_stateTransition;      // 0xa4 eStateTransitionType of the pending change
    int m_userData;             // 0xa8
};

/** A window that draws nothing itself. */
class WindowTransparent : public Window {
public:
    WindowTransparent(bool debugFill)
        : Window(debugFill)
    {
    }

    virtual ~WindowTransparent()
    {
    }

    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
    {
    }
};

/** A window filled with one 0xAARRGGBB color. */
class WindowColor : public Window {
public:
    WindowColor(unsigned int color = 0)
        : Window(false)
    {
        m_color = color;
    }

    virtual ~WindowColor()
    {
    }

    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);

    unsigned int m_color;       // 0xac
};

#endif
