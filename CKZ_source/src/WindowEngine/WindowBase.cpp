// WindowBase.cpp - Window: the window tree, its layout (simple alignment and tables), easing
// animations, event dispatch, the state stack, painting and the image drawing helpers; plus the
// resource handles (SmartRes) and WindowColor.
#include <OpenGLES/ES1/gl.h>
#include "WindowBase.h"
#include "WindowApp.h"
#include "MathLib.h"
#include "CDrawUtil.h"
#include "CApplet.h"
#include "CApp.h"
#include "CStdUtil_Cocoa.h"
#include "CStrWChar.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "ICRasterizerState.h"
#include "ICRenderSurface.h"
#include "ICResourceManager.h"
#include "CFont.h"
#include "CSwerve.h"
#include "CSwerveGraphics3D.h"
#include "tws.h"  // added by fx2 (swerve::Graphics3D handle in RunPaint3D)

using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;
using namespace com::glu::platform::systems;

// Notification ids of WINDOW_NOTIFY events sent to the event listeners.
static const int NOTIFY_STARTUP = 0x7fffff9e;       // first layout of the window
static const int NOTIFY_RECT_CHANGED = 0x7fffff9d;  // position or size changed
// Parameter of the Back easing curves (Penner's default).
static const float OVERSHOOT = 1.70158f;

/** Rasterizer state of the device. */
static inline ICRasterizerState* RasterizerState()
{
    return ICGraphics::GetInstance()->GetRasterizerState();
}

/** The resource manager of the application. */
static inline ICResourceManager* ResourceManager()
{
    return (ICResourceManager*)CApplet::m_pApp->m_resourceManager;
}

Window::EasyParams::EasyParams(Window* window, EasyLib::EMethod method)
{
    Init(window, method, EasyLib::ReverseMethod(method));
}

Window::EasyParams::EasyParams(Window* window, EasyLib::EMethod method, EasyLib::EMethod reverseMethod)
{
    Init(window, method, reverseMethod);
}

void Window::EasyParams::Init(Window* window, EasyLib::EMethod method, EasyLib::EMethod reverseMethod)
{
    m_started = false;
    m_fromX = m_toX = window->m_x;
    m_fromY = m_toY = window->m_y;
    m_fromWidth = m_toWidth = window->m_width;
    m_fromHeight = m_toHeight = window->m_height;
    m_fullTime = m_reverseTime = 0;
    m_delayX = m_delayY = 0;
    m_startX = m_startY = 0;
    m_overshootX = OVERSHOOT;
    m_overshootY = OVERSHOOT;
    m_opacityFrom = m_opacityTo = 1.0f;
    m_opacityDelay = m_opacityCut = 0;
    m_opacityStart = 0;
    m_scaleFrom = m_scaleTo = 1.0f;
    m_scaleDelay = m_scaleCut = 0;
    m_scaleStart = 0;
    m_methodX[0] = m_methodY[0] = method;
    m_methodX[1] = m_methodY[1] = reverseMethod;
    m_methodOpacity[0] = m_methodOpacity[1] = EasyLib::LINEAR;
    m_methodScale[0] = m_methodScale[1] = EasyLib::LINEAR;
    m_state = 0;
    m_clearFlagsOnStart = m_clearFlagsOnFinish = 0;
    m_setFlagsOnStart = m_setFlagsOnFinish = 0;
    m_userData = 0;
    m_moving = false;
    m_resizing = false;
}

void Window::EasyParams::SetFullTime(float time)
{
    m_fullTime = m_reverseTime = time;
    m_scaleCut = m_opacityCut = time * 0.3f;
}

void Window::EasyParams::SetFullTimes(float time, float reverseTime)
{
    m_fullTime = time;
    m_reverseTime = reverseTime;
    m_scaleCut = m_opacityCut = time * 0.3f;
}

void Window::EasyParams::SetDelayTime(float delay)
{
    m_delayX = m_delayY = delay;
    m_opacityDelay = m_scaleDelay = delay;
}

void Window::EasyParams::SetDelayTimes(float delayX, float delayY)
{
    float firstDelay = delayY;
    if (delayX < delayY) {
        firstDelay = delayX;
    }
    m_delayX = delayX;
    m_delayY = delayY;
    m_opacityDelay = firstDelay;
    m_scaleDelay = firstDelay;
}

void Window::EasyParams::SetMoving(int fromX, int fromY, int toX, int toY)
{
    m_fromX = fromX;
    m_toX = toX;
    m_fromY = fromY;
    m_toY = toY;
    m_moving = m_fromX != m_toX || m_fromY != m_toY;
}

void Window::EasyParams::SetOpacity(float from, float to)
{
    m_opacityFrom = from;
    m_opacityTo = to;
}

void Window::EasyParams::SetScale(float from, float to)
{
    m_scaleFrom = from;
    m_scaleTo = to;
}

int Window::EasyParams::RunEasy(EasyLib::EMethod* methods, float time, short* values, float overshoot)
{
    int from;
    int to;
    EasyLib::EMethod method;
    float duration;
    if (m_state == 2) {
        from = values[1];
        to = values[0];
        method = methods[1];
        duration = m_reverseTime;
    } else {
        from = values[0];
        to = values[1];
        method = methods[0];
        duration = m_fullTime;
    }
    return MathLib::Round(EasyLib::Run(EasyLib::DIR_FORWARD, method, time, (float)from, (float)(to - from),
                                       duration, overshoot));
}

float Window::EasyParams::RunOpacity(float time)
{
    float from;
    float change;
    float duration;
    EasyLib::EMethod* method;
    if (m_state == 2) {
        from = m_opacityTo;
        change = m_opacityFrom - m_opacityTo;
        duration = m_reverseTime;
        method = &m_methodOpacity[1];
    } else {
        duration = m_fullTime - m_opacityCut;
        from = m_opacityFrom;
        change = m_opacityTo - m_opacityFrom;
        method = &m_methodOpacity[0];
    }
    return EasyLib::Run(EasyLib::DIR_FORWARD, *method, time, from, change, duration, OVERSHOOT);
}

float Window::EasyParams::RunScale(float time)
{
    float from;
    float change;
    float duration;
    EasyLib::EMethod* method;
    if (m_state == 2) {
        from = m_scaleTo;
        change = m_scaleFrom - m_scaleTo;
        duration = m_reverseTime;
        method = &m_methodScale[1];
    } else {
        duration = m_fullTime - m_scaleCut;
        from = m_scaleFrom;
        change = m_scaleTo - m_scaleFrom;
        method = &m_methodScale[0];
    }
    return EasyLib::Run(EasyLib::DIR_FORWARD, *method, time, from, change, duration, OVERSHOOT);
}

Window::Window(bool debugFill)
{
    m_debugFill = debugFill;
    m_userData = 0;
    m_debugFrame = false;
    m_stateCount = 0;
    m_interrupt = 0;
    m_currentState = -1;
    m_nextState = -1;
    m_stateTransition = STATE_NONE;
    m_parent = NULL;
    m_nextSibling = NULL;
    m_firstChild = NULL;
    m_focusChild = NULL;
    m_x = 0;
    m_y = 0;
    m_width = WindowApp::m_instance->m_screenWidth;
    m_height = WindowApp::m_instance->m_screenHeight;
    m_eventListeners = NULL;
    m_flags = DEFAULT_FLAGS;
    m_align = ALIGN_LEFT | ALIGN_RIGHT | ALIGN_TOP | ALIGN_BOTTOM;
    m_tint = NULL;
    m_opacity = 1.0f;
    m_scale = 1.0f;
    m_easy = NULL;
    m_layout = NULL;
    m_listenerCount = 0;
    SetLayoutType(LAYOUT_SIMPLE);
}

void Window::SetLayoutType(ELayoutType type)
{
    Layout* layout = m_layout;
    if (layout == NULL) {
        if (type == LAYOUT_NONE) {
            return;
        }
        layout = GetLayout();
    }
    if (layout->m_type == type) {
        return;
    }
    layout->m_type = type;
    LayoutChanged();
}

Window::~Window()
{
    delete m_easy;
    delete m_layout;
    delete m_tint;
}

void Window::Close()
{
    if ((m_flags & FLAG_CLOSING) != 0) {
        return;
    }
    ClearAndSetFlags(FLAG_SHOW_AFTER_EASY | FLAG_CLOSE_AFTER_EASY | FLAG_CLOSING, FLAG_CLOSING);
    if (m_parent == NULL) {
        // Not in a window tree: nobody else will delete it.
        delete this;
        return;
    }
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        child->Close();
    }
}

void Window::ClearAndSetFlags(int clearFlags, int setFlags)
{
    m_flags = (m_flags & ~clearFlags) | setFlags;
}

void Window::CloseSafely()
{
    if (this == NULL) {
        return;
    }
    WindowApp::m_instance->m_desktop->CloseWindowIfExists(this);
}

int Window::CloseWindowIfExists(Window* window)
{
    if (IsWindowExisting(window) == 1) {
        window->Close();
        return 1;
    }
    return 0;
}

bool Window::IsChildExisting(Window* window)
{
    if (window == NULL) {
        return false;
    }
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if (child == window) {
            return true;
        }
    }
    return false;
}

int Window::IsWindowExisting(Window* window)
{
    if (window != NULL) {
        for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
            if (child == window) {
                return 1;
            }
        }
        for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
            if (child->IsWindowExisting(window) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

bool Window::IsChildOf(Window* window)
{
    if (window == NULL) {
        return false;
    }
    for (Window* parent = m_parent; parent != NULL; parent = parent->m_parent) {
        if (parent == window) {
            return true;
        }
    }
    return false;
}

int Window::CloseChildIfExists(Window* window)
{
    if (IsChildExisting(window)) {
        window->Close();
        return 1;
    }
    return 0;
}

void Window::CloseChilds()
{
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        child->Close();
    }
}

void Window::DeleteChilds()
{
    WindowApp* app = WindowApp::m_instance;
    m_focusChild = NULL;
    Window* child = m_firstChild;
    while (child != NULL) {
        child->HandleLostFocus();
        if ((child->m_flags & FLAG_SHUT_DOWN) == 0) {
            if ((child->m_flags & FLAG_INITIALIZED) != 0) {
                child->OnClosed();
            }
            child->m_flags |= FLAG_SHUT_DOWN;
        }
        m_firstChild = child->m_nextSibling;
        child->m_nextSibling = NULL;
        int flags = child->m_flags;
        child->m_flags = flags | FLAG_DEAD | FLAG_CLOSING;
        if ((flags & FLAG_DELETE_CHILDREN) != 0) {
            child->DeleteChilds();
        }
        if ((child->m_flags & FLAG_INITIALIZED) != 0) {
            child->m_flags &= ~FLAG_INITIALIZED;
            child->Shutdown();
        }
        child->DeleteChilds();
        child->ShutdownEventListeners();
        child->m_parent = NULL;
        delete child;
        app->OnWindowDestroyed(child);
        child = m_firstChild;
    }
    m_focusChild = NULL;
}

void Window::HandleLostFocus()
{
    if ((m_flags & FLAG_FOCUSED) == 0) {
        return;
    }
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & FLAG_INITIALIZED) != 0) {
            child->HandleLostFocus();
        }
    }
    m_flags &= ~FLAG_FOCUSED;
    OnLostFocus();
    if (m_parent != NULL && m_parent->m_focusChild == this) {
        m_parent->m_focusChild = NULL;
    }
}

void Window::ShutdownEventListeners()
{
    if (m_eventListeners != NULL) {
        RemoveEventListener(NULL, Event::NONE, 0);
        HandleFreeEventListeners();
    }
    if (m_listenerCount == 0) {
        return;
    }
    WindowApp::m_instance->m_desktop->RemoveOwnerEventListener_r(this);
    m_listenerCount = 0;
}

int Window::LocalToGlobalX(int x) const
{
    for (const Window* window = this; window != NULL; window = window->m_parent) {
        x += window->m_x;
    }
    return x;
}

int Window::LocalToGlobalY(int y) const
{
    for (const Window* window = this; window != NULL; window = window->m_parent) {
        y += window->m_y;
    }
    return y;
}

void Window::LocalToGlobal(int& x, int& y) const
{
    for (const Window* window = this; window != NULL; window = window->m_parent) {
        x += window->m_x;
        y += window->m_y;
    }
}

void Window::GlobalToLocal(int& x, int& y) const
{
    int originX = 0;
    int originY = 0;
    LocalToGlobal(originX, originY);
    x -= originX;
    y -= originY;
}

/** A spacing as a float, negative spacing counts as none. */
static inline float PositiveSpacing(int spacing)
{
    if (spacing < 0) {
        spacing = 0;
    }
    return (float)spacing;
}

bool Window::IsLocalPos(int x, int y) const
{
    Layout* layout = m_layout;
    if (layout != NULL && (m_flags & FLAG_HIT_OUTSET) != 0) {
        // The outset spacing belongs to the window too.
        float localX = (float)x;
        float localY = (float)y;
        return localX >= -PositiveSpacing(layout->m_outset.m_left) &&
               localX < (float)m_width + PositiveSpacing(layout->m_outset.m_right) &&
               localY >= -PositiveSpacing(layout->m_outset.m_top) &&
               localY < (float)m_height + PositiveSpacing(layout->m_outset.m_bottom);
    }
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

bool Window::IsVisible() const
{
    if ((m_flags & (FLAG_DEAD | FLAG_INITIALIZED | FLAG_STARTING | FLAG_CLOSING | FLAG_VISIBLE)) !=
        (FLAG_INITIALIZED | FLAG_VISIBLE)) {
        return false;
    }
    if (m_x + m_width <= 0) {
        return false;
    }
    if (m_y + m_height <= 0) {
        return false;
    }
    int parentWidth;
    if (m_parent != NULL) {
        parentWidth = m_parent->m_width;
    } else {
        parentWidth = WindowApp::m_instance->m_screenWidth;
    }
    if (m_x >= parentWidth) {
        return false;
    }
    int parentHeight;
    if (m_parent != NULL) {
        parentHeight = m_parent->m_height;
    } else {
        parentHeight = WindowApp::m_instance->m_screenHeight;
    }
    return m_y < parentHeight;
}

void Window::AddModal(Window* child)
{
    child->m_flags |= FLAG_MODAL;
    AddToFront(child);
}

void Window::SetFlags(int flags)
{
    m_flags |= flags;
}

void Window::AddToFront(Window* child)
{
    if ((child->m_flags & (FLAG_STARTING | FLAG_MODAL)) == (FLAG_STARTING | FLAG_MODAL)) {
        // A starting modal window goes behind the modal windows already in front.
        Window* lastModal = NULL;
        for (Window* sibling = m_firstChild; sibling != NULL && (sibling->m_flags & FLAG_MODAL) != 0;
             sibling = sibling->m_nextSibling) {
            lastModal = sibling;
        }
        if (lastModal != NULL) {
            child->m_parent = this;
            child->m_nextSibling = lastModal->m_nextSibling;
            lastModal->m_nextSibling = child;
            child->LayoutChanged();
            return;
        }
    }
    child->m_parent = this;
    child->m_nextSibling = m_firstChild;
    m_firstChild = child;
    child->LayoutChanged();
}

void Window::AddToFront(Window* child, int cellX, int cellY)
{
    child->SetCellPos(cellX, cellY, 1, 1);
    AddToFront(child);
}

void Window::SetCellPos(int cellX, int cellY, int colSpan, int rowSpan)
{
    Layout* layout = GetLayout();
    if (layout->m_type == LAYOUT_TABLE && layout->m_cellX == cellX && layout->m_cellY == cellY &&
        layout->m_colSpan == colSpan && layout->m_rowSpan == rowSpan) {
        return;
    }
    layout->m_type = LAYOUT_TABLE;
    m_layout->m_cellX = cellX;
    m_layout->m_cellY = cellY;
    m_layout->m_colSpan = colSpan;
    m_layout->m_rowSpan = rowSpan;
    LayoutChanged();
}

void Window::LayoutChanged()
{
    // The parent has to lay out again; so has its parent when its size depends on its children.
    Window* parent = m_parent;
    if (parent != NULL) {
        parent->m_flags |= FLAG_LAYOUT_DIRTY;
    }
    if (parent != NULL && parent->m_layout != NULL &&
        (parent->m_layout->m_type == LAYOUT_TABLE || parent->m_layout->m_width.m_unit == UNIT_CONTENT ||
         parent->m_layout->m_height.m_unit == UNIT_CONTENT)) {
        parent->LayoutChanged();
    }
}

void Window::Remove(Window* child)
{
    Window** link = &m_firstChild;
    while (*link != NULL) {
        if (*link == child) {
            *link = child->m_nextSibling;
            child->m_parent = NULL;
            child->m_nextSibling = NULL;
            return;
        }
        link = &(*link)->m_nextSibling;
    }
}

void Window::ClearFlags(int flags)
{
    m_flags &= ~flags;
}

Window::ELayoutType Window::LayoutType() const
{
    if (m_layout != NULL) {
        return (ELayoutType)m_layout->m_type;
    }
    return LAYOUT_NONE;
}

void Window::SetAlign(int align)
{
    if (m_align != align) {
        m_align = align;
        LayoutChanged();
    }
}

/** `opacity` limited to 0..1. */
static inline float ClampOpacity(float opacity)
{
    if (opacity < 0.0f) {
        return 0.0f;
    }
    return (1.0f < opacity) ? 1.0f : opacity;  // ?: gives the original vmin
}

void Window::SetOpacity(float opacity)
{
    if (m_tint == NULL) {
        m_opacity = ClampOpacity(opacity);
    } else {
        m_tint->a = ClampOpacity(opacity);
    }
}

void Window::SetColor(Color_RGBA& color)
{
    Color_RGBA* tint = m_tint;
    if (tint == NULL) {
        tint = new Color_RGBA;
        tint->r = 1.0f;
        tint->g = 1.0f;
        tint->b = 1.0f;
        tint->a = 1.0f;
        m_tint = tint;
    }
    *tint = color;
}

void Window::SetScale(float scale)
{
    if (scale <= 0.0f) {
        scale = 0.001f;
    }
    m_scale = scale;
}

void Window::SetCorner(int x, int y)
{
    SetRect(x, y, m_width, m_height, false);
}

void Window::SetCenter(int x, int y)
{
    SetRect(x - m_width / 2, y - m_height / 2, m_width, m_height, false);
}

bool Window::Layout::Table::UnitValues::Add(const UnitValue& value, int spacing)
{
    bool changed = false;
    if (value.m_min > 0) {
        int size = value.m_min + spacing;
        if (m_min < size) {
            m_min = size;
            changed = true;
        }
    }
    if (value.m_max > 0) {
        int size = value.m_max + spacing;
        if (m_max == 0 || m_max > size) {
            changed = true;
            m_max = size;
        }
    }
    switch (value.m_unit) {
    case UNIT_PIXELS:
        if (value.m_value > 0) {
            int size = value.m_value + spacing;
            if (m_pixels < size) {
                m_pixels = size;
                changed = true;
            }
        }
        break;
    case UNIT_PERCENT: {
        float percent = (float)value.m_value * 0.01f;
        if (m_percent < percent) {
            m_percent = percent;
            changed = true;
        }
        break;
    }
    }
    return changed;
}

Window::Layout::Table::~Table()
{
    Reset();
}

void Window::Layout::Table::Reset()
{
    for (int row = m_rowList.size() - 1; row >= 0; row--) {
        delete m_rowList[row];
    }
    m_rowList.setSize(0);
    m_columnCount = 0;
    m_columns.setSize(0);
    m_rows.setSize(0);
    m_width = 0;
    m_height = 0;
}

Window::Layout::Table::Cell& Window::Layout::Table::SetCell(int column, int row, const Cell& cell)
{
    while (m_rowList.size() <= row) {
        Row* newRow = new Row();
        m_rowList.addElement(newRow);
    }
    Vector<Cell>& cells = m_rowList[row]->m_cells;
    if (cells.size() <= column) {
        int columnCount = column + 1;
        if (m_columnCount > columnCount) {
            columnCount = m_columnCount;
        }
        m_columnCount = columnCount;
        cells.setSize(columnCount);
    }
    Cell& target = cells[column];
    target = cell;
    return target;
}

void Window::Layout::Table::PopulateCellWindows(Window* window)
{
    for (Window* child = window->m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & (FLAG_DEAD | FLAG_INITIALIZED | FLAG_CLOSING)) != FLAG_INITIALIZED) {
            continue;
        }
        Layout* layout = child->m_layout;
        if (layout == NULL || layout->m_type != LAYOUT_TABLE) {
            continue;
        }
        EasyParams* easy = child->m_easy;
        if (easy != NULL && easy->m_state != 0 && (easy->m_moving || easy->m_resizing)) {
            // An animated window keeps its own rectangle.
            continue;
        }
        Cell cell;
        cell.m_window = child;
        SetCell(layout->m_cellX, layout->m_cellY, cell);
    }
}

int Window::Layout::Table::BalanceDim(int size, Vector<UnitValues>& values, bool byContent)
{
    int count = values.size();
    int minTotal = 0;
    int pixelTotal = 0;
    float percentTotal = 0;
    UnitValues* value;  // the last one stays in use after the loop (as in the original)
    for (int index = 0; index < count; index++) {
        value = &values[index];
        int min = value->m_min;
        int max = value->m_max;
        if (max > 0 && max < min) {
            max = 0;
            value->m_max = 0;
        }
        int pixels = value->m_pixels;
        if (pixels < min) {
            value->m_pixels = value->m_min;
            pixels = min;
        }
        if (max > 0 && max < pixels) {
            value->m_max = 0;
        }
        minTotal += min;
        pixelTotal += pixels;
        value->m_size = min;
        percentTotal += value->m_percent;
    }
    if (size < pixelTotal && byContent) {
        size = pixelTotal;
        if (value->m_max > 0 && value->m_max < pixelTotal) {
            size = value->m_max;
        }
    }
    if (size <= minTotal) {
        return minTotal;
    }
    if (size <= 0) {
        for (int index = 0; index < count; index++) {
            values[index].m_size = 0;
        }
        return 0;
    }
    if (percentTotal > 1.0f) {
        float normalize = 1.0f / percentTotal;
        for (int index = 0; index < count; index++) {
            values[index].m_percent *= normalize;
        }
        percentTotal = 1.0f;
    }
    float sizeValue = (float)size;
    int freeCount = 0;
    for (int index = 0; index < count; index++) {
        UnitValues& current = values[index];
        if (current.m_percent > 0) {
            int percentSize = MathLib::Round(current.m_percent * sizeValue);
            if (percentSize < current.m_pixels) {
                // The fixed size wins over the percentage.
                percentTotal -= current.m_percent;
                current.m_percent = 0;
            } else {
                pixelTotal -= current.m_pixels;
                current.m_pixels = 0;
            }
        } else {
            freeCount += current.m_pixels < 1;
        }
    }
    int wanted = MathLib::Round(sizeValue * percentTotal) + pixelTotal;
    int used = 0;
    if (wanted <= size) {
        if (freeCount >= 1) {
            // The free columns share what is left.
            int share = (size - wanted) / freeCount;
            for (int index = 0; index < count - 1; index++) {
                UnitValues& current = values[index];
                if (current.m_pixels >= 1) {
                    current.m_size = current.m_pixels;
                } else if (current.m_percent > 0) {
                    current.m_size = MathLib::Round(sizeValue * current.m_percent);
                } else {
                    current.m_size = share;
                }
                if (current.m_max >= 1 && current.m_size > current.m_max) {
                    current.m_size = current.m_max;
                }
                used += current.m_size;
            }
        } else {
            // Stretch everything to fill the size.
            float stretch = sizeValue / (float)wanted;
            for (int index = 0; index < count - 1; index++) {
                UnitValues& current = values[index];
                float currentSize;
                if (current.m_pixels >= 1) {
                    currentSize = (float)current.m_pixels;
                } else if (current.m_percent > 0) {
                    currentSize = sizeValue * current.m_percent;
                } else {
                    current.m_size = 0;
                    used += 0;
                    continue;
                }
                current.m_size = (int)(currentSize * stretch);
                if (current.m_max >= 1 && current.m_size > current.m_max) {
                    current.m_size = current.m_max;
                }
                used += current.m_size;
            }
        }
    } else if (pixelTotal <= size) {
        // The fixed sizes fit: the percentages share the rest.
        float rest = (float)(size - pixelTotal);
        for (int index = 0; index < count - 1; index++) {
            UnitValues& current = values[index];
            if (current.m_pixels >= 1) {
                current.m_size = current.m_pixels;
            } else if (current.m_percent > 0) {
                current.m_size = MathLib::Round(rest * current.m_percent);
            } else {
                current.m_size = 0;
            }
            if (current.m_max >= 1 && current.m_size > current.m_max) {
                current.m_size = current.m_max;
            }
            used += current.m_size;
        }
    } else {
        // Not even the fixed sizes fit: shrink them towards their minimum.
        float ratio = (float)(size - minTotal) / (float)(wanted - minTotal);
        for (int index = 0; index < count - 1; index++) {
            UnitValues& current = values[index];
            if (current.m_pixels >= 1) {
                current.m_size = current.m_min + (int)((float)(current.m_pixels - current.m_min) * ratio);
            } else {
                current.m_size = 0;
            }
            if (current.m_max >= 1 && current.m_size > current.m_max) {
                current.m_size = current.m_max;
            }
            used += current.m_size;
        }
    }
    // The last one gets the remainder.
    UnitValues& last = values[values.size() - 1];
    last.m_size = size - used;
    if (last.m_max >= 1 && last.m_size > last.m_max) {
        last.m_size = last.m_max;
    }
    return size;
}

/** The cell at (column, row), NULL outside the table. */
static inline Window::Layout::Table::Cell* GetCell(Window::Layout::Table* table, int column, int row)
{
    if (row < table->m_rowList.size()) {
        Window::Layout::Table::Row* rowItem = table->m_rowList[row];
        if (column < rowItem->m_cells.size()) {
            return &rowItem->m_cells[column];
        }
    }
    return NULL;
}

void Window::Layout::Table::PopulateWidthAndHeight(int width, int height)
{
    m_columns.setSize(m_columnCount);
    m_rows.setSize(m_rowList.size());
    for (int row = m_rows.size() - 1; row >= 0; row--) {
        for (int column = m_columns.size() - 1; column >= 0; column--) {
            Cell* cell = GetCell(this, column, row);
            if (cell == NULL || cell->m_window == NULL) {
                continue;
            }
            Layout* layout = cell->m_window->m_layout;
            if (layout->m_colSpan == 1) {
                m_columns[column].Add(layout->m_width, layout->m_outset.m_right + layout->m_outset.m_left);
            }
            if (layout->m_rowSpan == 1) {
                m_rows[row].Add(layout->m_height, layout->m_outset.m_top + layout->m_outset.m_bottom);
            }
        }
    }
}

void Window::SetLayoutWidth(const UnitValue& width)
{
    Layout* layout = GetLayout();
    if (layout->m_width.m_min == width.m_min && layout->m_width.m_value == width.m_value &&
        layout->m_width.m_unit == width.m_unit && layout->m_width.m_max == width.m_max) {
        return;
    }
    layout->m_width = width;
    LayoutChanged();
}

void Window::SetDesiredWidth(int width)
{
    SetLayoutWidth(UnitValue(width, width, 0, UNIT_PIXELS));
}

void Window::SetDesiredWidth(int width, int minWidth, int maxWidth)
{
    SetLayoutWidth(UnitValue(width, minWidth, maxWidth, UNIT_PIXELS));
}

void Window::SetPercentWidth(int percent, int minWidth, int maxWidth)
{
    SetLayoutWidth(UnitValue(percent, minWidth, maxWidth, UNIT_PERCENT));
}

void Window::SetWidthByContent(int minWidth, int maxWidth)
{
    SetLayoutWidth(UnitValue(0, minWidth, maxWidth, UNIT_CONTENT));
}

void Window::SetLayoutHeight(const UnitValue& height)
{
    Layout* layout = GetLayout();
    if (layout->m_height.m_min == height.m_min && layout->m_height.m_value == height.m_value &&
        layout->m_height.m_unit == height.m_unit && layout->m_height.m_max == height.m_max) {
        return;
    }
    layout->m_height = height;
    LayoutChanged();
}

void Window::SetDesiredHeight(int height)
{
    SetLayoutHeight(UnitValue(height, height, 0, UNIT_PIXELS));
}

void Window::SetDesiredHeight(int height, int minHeight, int maxHeight)
{
    SetLayoutHeight(UnitValue(height, minHeight, maxHeight, UNIT_PIXELS));
}

void Window::SetPercentHeight(int percent, int minHeight, int maxHeight)
{
    SetLayoutHeight(UnitValue(percent, minHeight, maxHeight, UNIT_PERCENT));
}

void Window::SetHeightByContent(int minHeight, int maxHeight)
{
    SetLayoutHeight(UnitValue(0, minHeight, maxHeight, UNIT_CONTENT));
}

void Window::SetOutsetSpacing(const Spacing& spacing)
{
    Layout* layout = GetLayout();
    if (layout->m_outset.m_top == spacing.m_top && layout->m_outset.m_right == spacing.m_right &&
        layout->m_outset.m_bottom == spacing.m_bottom && layout->m_outset.m_left == spacing.m_left) {
        return;
    }
    layout->m_outset = spacing;
    LayoutChanged();
}

void Window::SetOutsetSpacing(int spacing)
{
    SetOutsetSpacing(Spacing(spacing, spacing, spacing, spacing));
}

void Window::SetOutsetSpacing(int vertical, int horizontal)
{
    SetOutsetSpacing(Spacing(vertical, horizontal, vertical, horizontal));
}

void Window::SetOutsetSpacing(int top, int right, int bottom, int left)
{
    SetOutsetSpacing(Spacing(top, right, bottom, left));
}

void Window::SetInsetSpacing(const Spacing& spacing)
{
    Layout* layout = GetLayout();
    if (layout->m_inset.m_top == spacing.m_top && layout->m_inset.m_right == spacing.m_right &&
        layout->m_inset.m_bottom == spacing.m_bottom && layout->m_inset.m_left == spacing.m_left) {
        return;
    }
    layout->m_inset = spacing;
    LayoutChanged();
}

void Window::SetInsetSpacing(int spacing)
{
    SetInsetSpacing(Spacing(spacing, spacing, spacing, spacing));
}

void Window::SetInsetSpacing(int vertical, int horizontal)
{
    SetInsetSpacing(Spacing(vertical, horizontal, vertical, horizontal));
}

void Window::SetInsetSpacing(int top, int right, int bottom, int left)
{
    SetInsetSpacing(Spacing(top, right, bottom, left));
}

void Window::FreeLayout()
{
    if (m_layout == NULL) {
        return;
    }
    delete m_layout;
    m_layout = NULL;
    LayoutChanged();
}

void Window::Layout::Table::Reflow(Window* window, int width, int height, bool force)
{
    PopulateCellWindows(window);
    if (m_columnCount <= 0) {
        return;
    }
    int insetTop = 0;
    int insetRight = 0;
    int insetBottom = 0;
    int insetLeft = 0;
    if (window->m_layout != NULL) {
        insetTop = window->m_layout->m_inset.m_top;
        insetRight = window->m_layout->m_inset.m_right;
        insetBottom = window->m_layout->m_inset.m_bottom;
        insetLeft = window->m_layout->m_inset.m_left;
    }
    PopulateWidthAndHeight(width, height);
    int innerHeight = height - (insetBottom + insetTop);
    int innerWidth = width - (insetLeft + insetRight);
    bool changed;
    do {
        Layout* layout = window->m_layout;
        if (layout == NULL) {
            m_width = BalanceDim(innerWidth, m_columns, false);
            m_height = BalanceDim(innerHeight, m_rows, false);
        } else {
            bool widthByContent = layout->m_width.m_unit == UNIT_CONTENT;
            bool heightByContent = layout->m_height.m_unit == UNIT_CONTENT;
            m_width = BalanceDim(innerWidth, m_columns, widthByContent);
            m_height = BalanceDim(innerHeight, m_rows, heightByContent);
            if (widthByContent && innerWidth < m_width) {
                innerWidth = m_width;
            }
            if (heightByContent && innerHeight < m_height) {
                innerHeight = m_height;
            }
        }
        m_width = insetLeft + insetRight + m_width;
        m_height = m_height + insetBottom + insetTop;
        int rowCount = m_rowList.size();
        if (rowCount < 1) {
            return;
        }
        int columnCount = m_columnCount;
        changed = false;
        int y = insetTop;
        for (int row = 0;; row++) {
            if (columnCount >= 1) {
                int x = insetLeft;
                for (int column = 0;;) {
                    Cell* cell = GetCell(this, column, row);
                    if (cell != NULL && cell->m_window != NULL) {
                        Window* child = cell->m_window;
                        Layout* childLayout = child->m_layout;
                        int cellWidth = m_columns[column].m_size;
                        int cellHeight = m_rows[row].m_size;
                        for (int span = 1; span < childLayout->m_colSpan && column + span < columnCount; span++) {
                            cellWidth += m_columns[column + span].m_size;
                        }
                        for (int span = 1; span < childLayout->m_rowSpan && row + span < rowCount; span++) {
                            cellHeight += m_rows[row + span].m_size;
                        }
                        Spacing& outset = childLayout->m_outset;
                        int childHeight = cellHeight - outset.m_top - outset.m_bottom;
                        int childWidth = cellWidth - outset.m_right - outset.m_left;
                        child->SetRect(x + outset.m_left, outset.m_top + y, childWidth, childHeight, force);
                        if ((child->m_flags & (FLAG_DEAD | FLAG_CLOSING)) == 0) {
                            child->m_flags |= FLAG_RECT_PENDING;
                            // The child did not fit: widen the column / row and lay out again.
                            if (childLayout->m_colSpan == 1 && childWidth < child->m_width) {
                                UnitValues& columnValues = m_columns[column];
                                columnValues.Add(childLayout->m_width, outset.m_right + outset.m_left);
                                int needed = outset.m_left + child->m_width + outset.m_right;
                                float minSize = (float)columnValues.m_min;
                                float neededSize = (float)needed;
                                if (neededSize < minSize) {
                                    neededSize = minSize;
                                }
                                columnValues.m_min = (short)neededSize;
                                if (columnValues.m_size < needed) {
                                    columnValues.m_size = needed;
                                    changed = true;
                                }
                            }
                            if (childLayout->m_rowSpan == 1 && childHeight < child->m_height) {
                                UnitValues& rowValues = m_rows[row];
                                rowValues.Add(childLayout->m_height, outset.m_bottom + outset.m_top);
                                int needed = outset.m_top + child->m_height + outset.m_bottom;
                                float minSize = (float)rowValues.m_min;
                                float neededSize = (float)needed;
                                if (neededSize < minSize) {
                                    neededSize = minSize;
                                }
                                rowValues.m_min = (short)neededSize;
                                if (m_rows[row].m_size < needed) {
                                    m_rows[row].m_size = needed;
                                    changed = true;
                                }
                            }
                        }
                    }
                    columnCount = m_columnCount;
                    column++;
                    if (column >= columnCount) {
                        break;
                    }
                    x += m_columns[column - 1].m_size;
                }
            }
            if (rowCount - 1 == row) {
                break;
            }
            y += m_rows[row].m_size;
        }
    } while (changed);
}

void Window::ComputeContentRect(int& x, int& y, int& width, int& height, bool skipLayouted)
{
    Window* child = m_firstChild;
    if (child != NULL) {
        int bottom = -9999999;
        int right = -9999999;
        int top = 9999999;
        int left = 9999999;
        do {
            if ((child->m_flags & (FLAG_DEAD | FLAG_INITIALIZED | FLAG_CLOSING)) == FLAG_INITIALIZED) {
                int outsetTop = 0;
                int outsetLeft = 0;
                int outsetBottom = 0;
                int outsetRight = 0;
                Layout* layout = child->m_layout;
                if (layout != NULL && layout->m_type != LAYOUT_NONE) {
                    if (skipLayouted) {
                        child = child->m_nextSibling;
                        continue;
                    }
                    outsetRight = layout->m_outset.m_right;
                    outsetTop = layout->m_outset.m_top;
                    outsetBottom = layout->m_outset.m_bottom;
                    outsetLeft = layout->m_outset.m_left;
                }
                int childTop = child->m_y - outsetTop;
                if (top < childTop) {
                    childTop = top;
                }
                int childLeft = child->m_x - outsetLeft;
                if (left < childLeft) {
                    childLeft = left;
                }
                int childBottom = child->m_y + outsetBottom + child->m_height;
                if (childBottom < bottom) {
                    childBottom = bottom;
                }
                int childRight = child->m_x + outsetRight + child->m_width;
                if (childRight < right) {
                    childRight = right;
                }
                top = childTop;
                left = childLeft;
                right = childRight;
                bottom = childBottom;
            }
            child = child->m_nextSibling;
        } while (child != NULL);
        if (left <= right) {
            x = left;
            y = top;
            width = right - left;
            height = bottom - top;
            if (height == 0 && width == 0) {
                return;
            }
            int insetTop = 0;
            int insetRight = 0;
            int insetBottom = 0;
            int insetLeft = 0;
            if (m_layout != NULL) {
                insetRight = m_layout->m_inset.m_right;
                insetTop = m_layout->m_inset.m_top;
                insetBottom = m_layout->m_inset.m_bottom;
                insetLeft = m_layout->m_inset.m_left;
            }
            x -= insetLeft;
            y -= insetTop;
            width = insetRight + insetLeft + width;
            height = insetTop + insetBottom + height;
            return;
        }
    }
    height = 0;
    width = 0;
    y = 0;
    x = 0;
}

void Window::MoveSimpleLayoutedChilds(int dx, int dy, bool clearAlign)
{
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
            continue;
        }
        Layout* layout = child->m_layout;
        if (layout == NULL || layout->m_type == LAYOUT_NONE) {
            if (clearAlign) {
                child->SetAlign(0);
            }
            child->SetRect(child->m_x + dx, child->m_y + dy, child->m_width, child->m_height, false);
        }
    }
}

void Window::ComputeSizeByContent(int& width, int& height, bool layoutedOnly)
{
    int insetTop = 0;
    int insetRight = 0;
    int insetBottom = 0;
    int insetLeft = 0;
    if (m_layout != NULL) {
        insetBottom = m_layout->m_inset.m_bottom;
        insetLeft = m_layout->m_inset.m_left;
        insetRight = m_layout->m_inset.m_right;
        insetTop = m_layout->m_inset.m_top;
    }
    height = 0;
    width = 0;
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & (FLAG_DEAD | FLAG_INITIALIZED | FLAG_CLOSING)) != FLAG_INITIALIZED) {
            continue;
        }
        Layout* layout = child->m_layout;
        bool layouted = layout != NULL && layout->m_type != LAYOUT_NONE;
        if (!layouted && layoutedOnly) {
            continue;
        }
        Spacing outset;
        if (layouted) {
            outset = layout->m_outset;
        }
        int right;
        if (layouted && (child->m_align & (ALIGN_LEFT | ALIGN_RIGHT | ALIGN_HCENTER | ALIGN_NONE)) != 0) {
            // Aligned: only the size counts.
            right = child->m_width + outset.m_right + insetLeft + insetRight + outset.m_left;
        } else {
            right = child->m_x + outset.m_right + insetRight + child->m_width;
        }
        if (right < width) {
            right = width;
        }
        width = right;
        int bottom;
        if (layouted && (child->m_align & (ALIGN_TOP | ALIGN_BOTTOM | ALIGN_VCENTER | ALIGN_NONE)) != 0) {
            bottom = outset.m_top + outset.m_bottom + insetBottom + insetTop + child->m_height;
        } else {
            bottom = child->m_height + outset.m_bottom + insetBottom + child->m_y;
        }
        if (bottom < height) {
            bottom = height;
        }
        height = bottom;
    }
}

void Window::LayoutTable(bool force)
{
    Layout::Table table;
    table.Reflow(this, m_width, m_height, force);
    if (table.m_columnCount != 0 && m_layout != NULL) {
        Layout* layout = m_layout;
        if (layout->m_width.m_unit == UNIT_CONTENT || layout->m_height.m_unit == UNIT_CONTENT) {
            if (m_easy == NULL || m_easy->m_state == 0 || !m_easy->m_resizing) {
                if (layout->m_width.m_unit == UNIT_CONTENT &&
                    (layout->m_type != LAYOUT_TABLE || table.m_width > m_width)) {
                    m_width = table.m_width;
                    short maxWidth = layout->m_width.m_max;
                    if (maxWidth > 0 && (short)table.m_width > maxWidth) {
                        m_width = maxWidth;
                    }
                }
                if (layout->m_height.m_unit == UNIT_CONTENT &&
                    (layout->m_type != LAYOUT_TABLE || table.m_height > m_height)) {
                    m_height = table.m_height;
                    short maxHeight = layout->m_height.m_max;
                    if (maxHeight > 0 && (short)table.m_height > maxHeight) {
                        m_height = maxHeight;
                    }
                }
            }
        }
    }
}

void Window::SetRect(int x, int y, int width, int height, bool force)
{
    short oldX = m_x;
    short oldY = m_y;
    unsigned short oldHeight = m_height;
    unsigned short oldWidth = m_width;
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
    if ((m_flags & FLAG_IN_SET_RECT) != 0) {
        return;
    }
    int setFlags = FLAG_IN_SET_RECT;
    if (force) {
        setFlags = FLAG_IN_SET_RECT | FLAG_RECT_PENDING;
    }
    int deltaHeight = (unsigned short)height - oldHeight;
    int deltaWidth = (unsigned short)width - oldWidth;
    m_flags |= setFlags;
    bool notifyStartup = true;
    unsigned short previousHeight = oldHeight;
    unsigned short previousWidth = oldWidth;
    int contentWidth = width;
    int contentHeight = height;
    while (true) {
        do {
            if (!force && (short)(deltaWidth | deltaHeight) == 0) {
                if ((m_flags & FLAG_LAYOUT_DIRTY) == 0) {
                    goto finished;
                }
            } else {
                if (m_easy != NULL && (m_flags & FLAG_IN_EASY) == 0) {
                    OnLayoutEasy(oldX, oldY, (short)previousWidth, (short)previousHeight);
                }
                OnLayout();
            }
            m_flags &= ~FLAG_LAYOUT_DIRTY;
            if (m_firstChild == NULL) {
                goto finished;
            }
            LayoutTable(force);
            int innerWidth = m_width;
            int innerHeight = m_height;
            int insetTop = 0;
            int insetLeft = 0;
            if (m_layout != NULL) {
                insetTop = m_layout->m_inset.m_top;
                insetLeft = m_layout->m_inset.m_left;
                innerHeight = m_height - insetTop - m_layout->m_inset.m_bottom;
                innerWidth = m_width - insetLeft - m_layout->m_inset.m_right;
            }
            deltaHeight = m_height - previousHeight;
            deltaWidth = m_width - previousWidth;
            for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
                if ((child->m_flags & (FLAG_DEAD | FLAG_INITIALIZED | FLAG_CLOSING | FLAG_RECT_PENDING)) !=
                    FLAG_INITIALIZED) {
                    child->m_flags &= ~FLAG_RECT_PENDING;
                    continue;
                }
                Layout* layout = child->m_layout;
                int layoutType = LAYOUT_NONE;
                if (layout != NULL) {
                    layoutType = layout->m_type;
                    if (layoutType == LAYOUT_TABLE) {
                        // Placed by LayoutTable.
                        continue;
                    }
                }
                if (!force) {
                    EasyParams* easy = child->m_easy;
                    if (easy != NULL && easy->m_state != 0 && (easy->m_moving || easy->m_resizing)) {
                        continue;
                    }
                }
                Spacing outset;
                if (layout != NULL) {
                    outset = layout->m_outset;
                }
                if ((child->m_align & ALIGN_NONE) != 0) {
                    // Fill the content area.
                    int childX = insetLeft;
                    int childY = insetTop;
                    int childWidth = innerWidth;
                    int childHeight = innerHeight;
                    if (layoutType != LAYOUT_NONE) {
                        childY = outset.m_top + insetTop;
                        childHeight = innerHeight - outset.m_top - outset.m_bottom;
                        childX = outset.m_left + insetLeft;
                        childWidth = innerWidth - outset.m_right - outset.m_left;
                    }
                    if (layout != NULL) {
                        if (layout->m_width.m_unit == UNIT_CONTENT) {
                            childWidth = 0;
                        }
                        if (layout->m_height.m_unit == UNIT_CONTENT) {
                            childHeight = 0;
                        }
                    }
                    child->SetRect(childX, childY, childWidth, childHeight, force);
                } else {
                    bool fixedWidth = false;
                    bool fixedHeight;
                    int childX = child->m_x;
                    int childY = child->m_y;
                    int childWidth = child->m_width;
                    int childHeight = child->m_height;
                    if (layout == NULL) {
                        fixedHeight = false;
                    } else {
                        int unit = layout->m_width.m_unit;
                        if (unit == UNIT_PIXELS) {
                            childWidth = layout->m_width.m_value;
                            fixedWidth = true;
                        } else if (unit == UNIT_CONTENT) {
                            fixedWidth = true;
                            childWidth = 0;
                        } else {
                            fixedWidth = false;
                            if (unit == UNIT_PERCENT) {
                                int spacing = 0;
                                if (layoutType != LAYOUT_NONE) {
                                    spacing = outset.m_right + outset.m_left;
                                }
                                childWidth = MathLib::Round((float)(innerWidth - spacing) *
                                                            ((float)layout->m_width.m_value * 0.01f));
                                fixedWidth = true;
                                layout = child->m_layout;
                            }
                        }
                        unit = layout->m_height.m_unit;
                        if (unit == UNIT_PIXELS) {
                            fixedHeight = true;
                            childHeight = layout->m_height.m_value;
                        } else if (unit == UNIT_PERCENT) {
                            int spacing = 0;
                            if (layoutType != LAYOUT_NONE) {
                                spacing = outset.m_top + outset.m_bottom;
                            }
                            childHeight = MathLib::Round((float)(innerHeight - spacing) *
                                                         ((float)layout->m_height.m_value * 0.01f));
                            fixedHeight = true;
                        } else {
                            fixedHeight = false;
                            if (unit == UNIT_CONTENT) {
                                childHeight = 0;
                                fixedHeight = true;
                            }
                        }
                    }
                    int areaY = outset.m_top + insetTop;
                    int areaX = outset.m_left + insetLeft;
                    int areaHeight = innerHeight - outset.m_top - outset.m_bottom;
                    int areaWidth = innerWidth - outset.m_right - outset.m_left;
                    for (int pass = 0; pass < 2; pass++) {
                        int align = child->m_align;
                        if ((align & ALIGN_HCENTER) != 0) {
                            if (layoutType == LAYOUT_NONE) {
                                childX = insetLeft + ((innerWidth - childWidth) >> 1);
                            } else {
                                int centered = areaX + ((areaWidth - childWidth) >> 1);
                                childX = areaX;
                                if (areaX < centered) {
                                    childX = centered;
                                }
                            }
                        } else if ((align & (ALIGN_LEFT | ALIGN_RIGHT)) == (ALIGN_LEFT | ALIGN_RIGHT)) {
                            if (layoutType == LAYOUT_NONE) {
                                if (!fixedWidth) {
                                    childWidth = childWidth + (short)deltaWidth;
                                }
                            } else {
                                childX = areaX;
                                if (!fixedWidth) {
                                    childWidth = areaWidth;
                                }
                            }
                        } else if ((align & ALIGN_LEFT) != 0) {
                            if (layoutType != LAYOUT_NONE) {
                                childX = areaX;
                            }
                        } else if ((align & ALIGN_RIGHT) != 0) {
                            if (layoutType == LAYOUT_NONE) {
                                childX = childX + (short)deltaWidth;
                            } else {
                                childX = innerWidth + insetLeft - outset.m_right - childWidth;
                            }
                        }
                        if ((align & ALIGN_VCENTER) != 0) {
                            if (layoutType == LAYOUT_NONE) {
                                childY = insetTop + ((innerHeight - childHeight) >> 1);
                            } else {
                                int centered = areaY + ((areaHeight - childHeight) >> 1);
                                childY = areaY;
                                if (areaY < centered) {
                                    childY = centered;
                                }
                            }
                        } else if ((align & (ALIGN_TOP | ALIGN_BOTTOM)) == (ALIGN_TOP | ALIGN_BOTTOM)) {
                            if (layoutType == LAYOUT_NONE) {
                                if (!fixedHeight) {
                                    childHeight = childHeight + (short)deltaHeight;
                                }
                            } else {
                                childY = areaY;
                                if (!fixedHeight) {
                                    childHeight = areaHeight;
                                }
                            }
                        } else if ((align & ALIGN_TOP) != 0) {
                            if (layoutType != LAYOUT_NONE) {
                                childY = areaY;
                            }
                        } else if ((align & ALIGN_BOTTOM) != 0) {
                            if (layoutType == LAYOUT_NONE) {
                                childY = childY + (short)deltaHeight;
                            } else {
                                childY = innerHeight + insetTop - outset.m_bottom - childHeight;
                            }
                        }
                        // Respect the minimum size of the layout.
                        int finalHeight = childHeight;
                        Layout* childLayout = child->m_layout;
                        if (childLayout != NULL) {
                            int minWidth = childLayout->m_width.m_min;
                            int minHeight = childLayout->m_height.m_min;
                            if (minWidth > 0 && childWidth < minWidth) {
                                childWidth = minWidth;
                            }
                            if (minHeight > 0 && minHeight > childHeight) {
                                finalHeight = minHeight;
                            }
                        }
                        child->SetRect(childX, childY, childWidth, finalHeight, force);
                        // Centered / right / bottom aligned children are placed again when they
                        // chose another size.
                        if ((child->m_align & (ALIGN_RIGHT | ALIGN_HCENTER | ALIGN_BOTTOM | ALIGN_VCENTER)) == 0) {
                            break;
                        }
                        int newWidth = child->m_width;
                        childHeight = child->m_height;
                        if (newWidth == childWidth && childHeight == finalHeight) {
                            break;
                        }
                        fixedHeight = fixedHeight | (childHeight != finalHeight);
                        fixedWidth = fixedWidth | (newWidth != childWidth);
                        childWidth = newWidth;
                    }
                }
                child->m_flags &= ~FLAG_RECT_PENDING;
            }
        } while ((m_flags & FLAG_LAYOUT_DIRTY) != 0);
        // A size computed from the content may grow with the children.
        Layout* layout = m_layout;
        if (layout == NULL) {
            goto finished;
        }
        if ((layout->m_width.m_unit != UNIT_CONTENT && layout->m_height.m_unit != UNIT_CONTENT) ||
            (m_flags & FLAG_IN_EASY) != 0) {
            goto finished;
        }
        ComputeSizeByContent(contentWidth, contentHeight, true);
        previousWidth = m_width;
        previousHeight = m_height;
        unsigned short newWidth = previousWidth;
        unsigned short newHeight;
        bool grew = false;
        if (layout->m_width.m_unit == UNIT_CONTENT && (short)previousWidth < contentWidth) {
            int maxWidth = layout->m_width.m_max;
            if (maxWidth < 1 || contentWidth <= maxWidth) {
                grew = true;
                notifyStartup = false;
                m_width = contentWidth;
                newWidth = contentWidth;
            } else {
                contentWidth = maxWidth;
                if ((short)previousWidth < maxWidth) {
                    m_width = layout->m_width.m_max;
                    notifyStartup = false;
                    grew = true;
                    newWidth = maxWidth;
                }
            }
        }
        if (layout->m_height.m_unit == UNIT_CONTENT && (short)previousHeight < contentHeight) {
            int maxHeight = layout->m_height.m_max;
            if (maxHeight < 1 || contentHeight <= maxHeight) {
                m_height = contentHeight;
                notifyStartup = false;
                newHeight = contentHeight;
            } else {
                contentHeight = maxHeight;
                if (maxHeight <= (short)previousHeight) {
                    if (!grew) {
                        break;
                    }
                    newHeight = previousHeight;
                } else {
                    m_height = layout->m_height.m_max;
                    notifyStartup = false;
                    newHeight = maxHeight;
                }
            }
        } else {
            if (!grew) {
                break;
            }
            newHeight = previousHeight;
        }
        deltaHeight = newHeight - previousHeight;
        deltaWidth = newWidth - previousWidth;
    }
finished:
    if ((m_flags & FLAG_IN_EASY) == 0 && m_parent != NULL && (m_parent->m_flags & FLAG_LAYOUT_DIRTY) == 0) {
        EasyParams* easy = m_easy;
        if (easy == NULL || (easy->m_state == 1 && !easy->m_started)) {
            if (notifyStartup || (m_align & (ALIGN_RIGHT | ALIGN_HCENTER | ALIGN_BOTTOM | ALIGN_VCENTER)) == 0) {
                OnStartupEasy();
                DispatchEventListeners(Event::WINDOW_NOTIFY, NOTIFY_STARTUP);
            }
        }
    }
    bool moved = oldX != m_x || oldY != m_y;
    bool resized = oldWidth != (unsigned short)m_width || oldHeight != (unsigned short)m_height;
    if (moved || resized) {
        OnRectChanged();
        if (m_parent != NULL) {
            m_parent->OnChildRectChanged(this);
        }
        DispatchEventListeners(Event::WINDOW_NOTIFY, NOTIFY_RECT_CHANGED);
        if (resized) {
            OnSizeChanged();
        }
    }
    OnSetRectFinished();
    m_flags &= ~FLAG_IN_SET_RECT;
}

int Window::DispatchEventListeners(Event::EType type, int id)
{
    if (m_eventListeners == NULL) {
        return 0;
    }
    Event event(type, id, 0, 0, 0);
    return DispatchEventListeners(event);
}

void Window::SetSize(int width, int height)
{
    SetRect(m_x, m_y, width, height, false);
}

void Window::Paint(ICGraphics2d& graphics)
{
    if (m_debugFill) {
        unsigned int color = 0xffff0000;
        if (((WindowApp::m_instance->m_updateTime / 500) & 1) == 0) {
            color = 0xff00ff00;
        }
        CDrawUtil::Fill(0, 0, m_width, m_height, color, (ICGraphics2d::Primitive)3);
    }
    if (m_debugFrame) {
        CDrawUtil::Fill(0, 0, m_width - 1, 0, 0xffff0000, (ICGraphics2d::Primitive)1);
        CDrawUtil::Fill(m_width - 1, 0, m_width - 1, m_height - 1, 0xffff0000, (ICGraphics2d::Primitive)1);
        CDrawUtil::Fill(m_width - 1, m_height - 1, 0, m_height - 1, 0xffff0000, (ICGraphics2d::Primitive)1);
        CDrawUtil::Fill(0, m_height - 1, 0, 0, 0xffff0000, (ICGraphics2d::Primitive)1);
    }
}

int Window::DispatchEventListeners(Event& event)
{
    int type = event.m_type;
    if (type == Event::NONE) {
        return 0;
    }
    for (EventListener* listener = m_eventListeners; listener != NULL; listener = listener->m_next) {
        if ((m_flags & FLAG_CLOSING) != 0) {
            break;
        }
        if (listener->m_type == type && (listener->m_id == 0 || listener->m_id == event.m_id)) {
            listener->OnEvent(this, event);
            type = event.m_type;
            if (type == Event::NONE) {
                return 1;
            }
        }
    }
    return 0;
}

void Window::RemoveEventListener(Window* owner, Event::EType type, int id)
{
    for (EventListener* listener = m_eventListeners; listener != NULL; listener = listener->m_next) {
        if ((type == Event::NONE || listener->m_type == type) && (id == 0 || listener->m_id == id) &&
            (owner == NULL || listener->m_owner == owner)) {
            Window* listenerOwner = listener->m_owner;
            if (listenerOwner != NULL && listenerOwner != this &&
                WindowApp::m_instance->m_desktop->IsWindowExisting(listenerOwner) == 1) {
                listenerOwner->m_listenerCount--;
            }
            listener->m_owner = NULL;
            listener->m_type = Event::NONE;
        }
    }
}

void Window::HandleFreeEventListeners()
{
    EventListener* previous = NULL;
    EventListener* listener = m_eventListeners;
    while (listener != NULL) {
        EventListener* next = listener->m_next;
        if (listener->m_type != Event::NONE) {
            // Still in use: keep it.
        } else {
            if (previous == NULL) {
                m_eventListeners = next;
            } else {
                previous->m_next = next;
            }
            listener->m_type = Event::NONE;
            listener->m_next = NULL;
            delete listener;
        }
        // Note: `previous` also moves past a deleted listener (as in the original).
        previous = listener;
        listener = next;
    }
}

void Window::RemoveOwnerEventListener_r(Window* owner)
{
    RemoveEventListener(owner, Event::NONE, 0);
    for (Window* child = m_firstChild; child != NULL && owner->m_listenerCount != 0; child = child->m_nextSibling) {
        child->RemoveOwnerEventListener_r(owner);
    }
}

/** The key code of a key event that this window uses: raw (param1) or remapped (param2). */
static inline int* EventKey(Window* window, Event& event)
{
    // ?: gives the original select of the two addresses.
    return &(&event.m_param1)[(window->m_flags & Window::FLAG_REMAPPED_KEYS) != 0];
}

int Window::DispatchEvent(Event& event)
{
    switch (event.m_type) {
    case Event::KEY_PRESSED: {
        int* key = EventKey(this, event);
        if (*key == 0) {
            return 0;
        }
        event.m_id = *key;
        OnKeyPressed(event);
        DispatchEventListeners(event);
        *key = event.m_id;
        break;
    }
    case Event::KEY_RELEASED: {
        int* key = EventKey(this, event);
        if (*key == 0) {
            return 0;
        }
        event.m_id = *key;
        OnKeyReleased(event);
        DispatchEventListeners(event);
        *key = event.m_id;
        break;
    }
    case Event::POINTER_PRESSED:
    case Event::POINTER_DRAGGED:
    case Event::POINTER_RELEASED: {
        // Work in local coordinates, restore the global ones afterwards.
        short globalX = event.m_param1;
        short globalY = event.m_param2;
        short pointer = event.m_param3;
        int originX = 0;
        int originY = 0;
        LocalToGlobal(originX, originY);
        event.m_param1 = event.m_param1 - originX;
        event.m_param2 = event.m_param2 - originY;
        Window* capture = WindowApp::PointerCapture(event.m_param3);
        bool capturing;
        if (event.m_type != Event::POINTER_PRESSED) {
            if ((m_flags & FLAG_POINTER_EXCLUSIVE) != 0 && capture != this) {
                goto notHandled;
            }
        }
        if (capture != this || event.m_type == Event::POINTER_PRESSED) {
            if ((m_flags & FLAG_POINTER_HIT_TEST) != 0) {
                int inside = IsLocalPos(event.m_param1, event.m_param2);
                event.m_handled = inside ^ 1;
                if (inside != 1) {
                    goto notHandled;
                }
            }
        }
        if (event.m_type == Event::POINTER_DRAGGED) {
            OnPointerEvent(event);
            capturing = true;
            if (event.m_type == Event::NONE) {
                goto handled;
            }
            OnPointerDragged(event);
            DispatchEventListeners(event);
            capturing = true;
        } else if (event.m_type == Event::POINTER_PRESSED) {
            OnPointerEvent(event);
            capturing = true;
            if (event.m_type == Event::NONE) {
                goto handled;
            }
            OnPointerPressed(event);
            DispatchEventListeners(event);
            capturing = true;
        } else {
            OnPointerEvent(event);
            capturing = false;
            if (event.m_type == Event::NONE) {
                goto handled;
            }
            OnPointerReleased(event);
            DispatchEventListeners(event);
            capturing = false;
        }
        if (event.m_type != Event::NONE) {
        notHandled:
            event.m_param1 = globalX;
            event.m_param2 = globalY;
            return 0;
        }
    handled:
        event.m_param1 = globalX;
        event.m_param2 = globalY;
        if (event.m_handler != NULL) {
            return 1;
        }
        event.m_handler = this;
        if (capturing) {
            WindowApp::SetPointerCapture(pointer, this);
        } else {
            WindowApp::SetPointerCapture(pointer, NULL);
        }
        return 1;
    }
    case Event::COMMAND:
    case Event::TUNNEL_COMMAND:
        OnCommand(event);
        DispatchEventListeners(event);
        break;
    default:
        break;
    }
    if (event.m_type != Event::NONE) {
        return 0;
    }
    if (event.m_handler == NULL) {
        event.m_handler = this;
    }
    return 1;
}

Window* Window::HandleEvent(Event::EType type, int id, int param1, int param2, int param3)
{
    Event event(type, id, param1, param2, param3);
    return HandleEvent(event);
}

Window* Window::HandleEvent(Event& event)
{
    if ((m_flags & FLAG_ENABLED) == 0) {
        return event.m_handler;
    }
    event.m_handled = false;
    unsigned char initialFlags = *reinterpret_cast<const unsigned char*>(&m_flags);
    if ((initialFlags & FLAG_CHILDREN_FIRST) == 0) {
        if (DispatchEvent(event) != 0 || event.m_handled) {
            return event.m_handler;
        }
    }
    int flags = m_flags;
    if ((flags & FLAG_EVENTS_TO_CHILDREN) != 0) {
        Window* child = m_focusChild;
        if (child == NULL) {
            child = m_firstChild;
        }
        for (; child != NULL; child = child->m_nextSibling) {
            int childFlags = child->m_flags;
            if ((childFlags & FLAG_INITIALIZED) == 0) {
                continue;
            }
            if (event.m_type == Event::TUNNEL_COMMAND) {
                if ((childFlags & (FLAG_DEAD | FLAG_CLOSING)) == 0) {
                    child->HandleEvent(event);
                    if (event.m_type == Event::NONE) {
                        return event.m_handler;
                    }
                }
            } else if ((childFlags & (FLAG_DEAD | FLAG_STARTING | FLAG_CLOSING)) == 0) {
                // Windows easing in / out only get events when they ask for them.
                if ((childFlags & (FLAG_SHOW_AFTER_EASY | FLAG_EVENTS_WHILE_SHOWING)) != FLAG_SHOW_AFTER_EASY &&
                    (childFlags & (FLAG_CLOSE_AFTER_EASY | FLAG_EVENTS_WHILE_CLOSING)) != FLAG_CLOSE_AFTER_EASY) {
                    child->HandleEvent(event);
                    if (event.m_type == Event::NONE) {
                        return event.m_handler;
                    }
                    childFlags = (unsigned char)child->m_flags;
                }
                if ((childFlags & FLAG_MODAL) != 0) {
                    return event.m_handler;
                }
            }
        }
        flags = m_flags;
    }
    if ((flags & FLAG_CHILDREN_FIRST) != 0) {
        DispatchEvent(event);
    }
    int type = event.m_type;
    if (type != Event::NONE) {
        int pointer = event.m_param3;
        OnUnhandledEvent(event);
        if (event.m_type == Event::NONE && event.m_handler == NULL) {
            event.m_handler = this;
            if (type != Event::POINTER_RELEASED) {
                if (type == Event::POINTER_PRESSED || type == Event::POINTER_DRAGGED) {
                    WindowApp::SetPointerCapture((short)pointer, this);
                }
            } else {
                WindowApp::SetPointerCapture((short)pointer, NULL);
            }
        }
    }
    return event.m_handler;
}

Window* Window::HandleCommand(int id, int param1, int param2, int param3)
{
    Event event(Event::COMMAND, id, param1, param2, param3);
    return HandleEvent(event);
}

Window* Window::HandleTunnelCommand(int id, int param1, int param2, int param3)
{
    Event event(Event::TUNNEL_COMMAND, id, param1, param2, param3);
    return HandleEvent(event);
}

float Window::EasyTime()
{
    return (float)(unsigned int)CStdUtil_Cocoa::GetUpTimeMS() * 0.001f;
}

/** Set the flags of an easing start; a show request cancels a close request and vice versa. */
static inline void ApplyEasyStartFlags(Window* window, int flags)
{
    Window::EasyParams* easy = window->m_easy;
    window->ClearAndSetFlags(easy->m_clearFlagsOnStart, easy->m_setFlagsOnStart | flags);
    if ((flags & Window::FLAG_SHOW_AFTER_EASY) != 0) {
        window->m_flags &= ~Window::FLAG_CLOSE_AFTER_EASY;
    } else if ((flags & Window::FLAG_CLOSE_AFTER_EASY) != 0) {
        window->m_flags &= ~Window::FLAG_SHOW_AFTER_EASY;
    }
}

/** End the easing: apply the finish flags, then show or close the window as requested. */
static inline void EndEasy(Window* window)
{
    window->m_easy->m_state = 0;
    window->ClearAndSetFlags(window->m_easy->m_clearFlagsOnFinish, window->m_easy->m_setFlagsOnFinish);
    if ((window->m_flags & Window::FLAG_SHOW_AFTER_EASY) != 0) {
        window->m_flags &= ~Window::FLAG_SHOW_AFTER_EASY;
    } else if ((window->m_flags & Window::FLAG_CLOSE_AFTER_EASY) != 0) {
        window->Close();
    }
}

/** Finish at once when nothing would move. */
static inline void FinishStaticEasy(Window* window)
{
    Window::EasyParams* easy = window->m_easy;
    if (!easy->m_moving && !easy->m_resizing && easy->m_opacityFrom == easy->m_opacityTo &&
        easy->m_scaleFrom == easy->m_scaleTo) {
        EndEasy(window);
        if (window->m_easy->m_started) {
            window->OnEasyFinished();
        }
    }
}

void Window::StartEasy(const EasyParams& params, int flags)
{
    if (m_easy == NULL) {
        m_easy = new EasyParams(params);
    } else {
        *m_easy = params;
    }
    float now = EasyTime();
    m_easy->m_started = false;
    m_easy->m_startX = now + m_easy->m_delayX;
    m_easy->m_startY = now + m_easy->m_delayY;
    m_easy->m_opacityStart = now + m_easy->m_opacityDelay;
    m_easy->m_scaleStart = now + m_easy->m_scaleDelay;
    m_easy->m_state = 1;
    ApplyEasyStartFlags(this, flags);
    FinishStaticEasy(this);
}

void Window::ReverseEasy(int flags)
{
    if (m_easy == NULL) {
        if ((flags & FLAG_CLOSE_AFTER_EASY) != 0) {
            Close();
        }
        return;
    }
    if (m_easy->m_state == 2) {
        // Already reversing: only take the flags.
        if (flags == 0) {
            return;
        }
        m_flags |= flags;
        if ((flags & FLAG_SHOW_AFTER_EASY) != 0) {
            m_flags &= ~FLAG_CLOSE_AFTER_EASY;
            return;
        }
        if ((flags & FLAG_CLOSE_AFTER_EASY) != 0) {
            m_flags &= ~FLAG_SHOW_AFTER_EASY;
        }
        return;
    }
    float now = EasyTime();
    m_easy->m_started = false;
    m_easy->m_opacityStart = now + m_easy->m_opacityCut;
    m_easy->m_scaleStart = now + m_easy->m_scaleCut;
    m_easy->m_startX = now + m_easy->m_delayX;
    m_easy->m_startY = now + m_easy->m_delayY;
    EasyParams* easy = m_easy;
    float shift = easy->m_delayY;
    if (easy->m_delayX < easy->m_delayY) {
        shift = easy->m_delayX;
    }
    if (easy->m_state == 0 || easy->m_fullTime <= 0) {
        // Not running: go back from the current rectangle.
        easy->m_toX = m_x;
        m_easy->m_toY = m_y;
        m_easy->m_toWidth = m_width;
        m_easy->m_toHeight = m_height;
        easy = m_easy;
    } else {
        // Running forward: start the reverse run from the point reached.
        float start = easy->m_startX;
        if (easy->m_startY < start) {
            start = easy->m_startY;
        }
        float elapsed = now - start;
        if (elapsed > 0) {
            shift = shift + easy->m_reverseTime * (1.0f - elapsed / easy->m_fullTime);
        }
    }
    easy->m_startX = easy->m_startX - shift;
    m_easy->m_startY = m_easy->m_startY - shift;
    m_easy->m_state = 2;
    ApplyEasyStartFlags(this, flags);
    FinishStaticEasy(this);
}

void Window::StopEasy()
{
    if (m_easy != NULL && m_easy->m_state != 0) {
        EndEasy(this);
        OnEasyStopped();
    }
}

void Window::ResetEasy()
{
    StopEasy();
    delete m_easy;
    m_easy = NULL;
}

void Window::UpdateEasy()
{
    if ((m_flags & FLAG_IN_EASY) != 0) {
        return;
    }
    m_flags |= FLAG_IN_EASY;
    if (!m_easy->m_started) {
        m_easy->m_started = true;
        m_easy->m_startX = EasyTime() + m_easy->m_delayX;
        m_easy->m_startY = EasyTime() + m_easy->m_delayY;
        m_easy->m_opacityStart = EasyTime() + m_easy->m_opacityDelay;
        m_easy->m_scaleStart = EasyTime() + m_easy->m_scaleDelay;
        OnEasyStarted();
        if ((m_flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
            return;
        }
    }
    float now = EasyTime();
    EasyParams* easy = m_easy;
    float opacityTime = now - easy->m_opacityStart;
    float scaleTime = now - easy->m_scaleStart;
    float timeX = now - easy->m_startX;
    float timeY = now - easy->m_startY;
    if (easy->m_moving || easy->m_resizing) {
        int x;
        int y;
        if (easy->m_moving) {
            if (easy->m_toX == easy->m_fromX) {
                x = easy->m_fromX;
            } else {
                x = easy->RunEasy(easy->m_methodX, timeX, &easy->m_fromX, easy->m_overshootX);
                easy = m_easy;
            }
            if (easy->m_toY == easy->m_fromY) {
                y = easy->m_fromY;
            } else {
                y = easy->RunEasy(easy->m_methodY, timeY, &easy->m_fromY, easy->m_overshootY);
                easy = m_easy;
            }
        } else {
            x = m_x;
            y = m_y;
        }
        int width;
        int height;
        if (easy->m_resizing) {
            if (easy->m_toWidth == easy->m_fromWidth) {
                width = easy->m_fromWidth;
            } else {
                width = easy->RunEasy(easy->m_methodX, timeX, &easy->m_fromWidth, easy->m_overshootX);
                easy = m_easy;
            }
            if (easy->m_toHeight == easy->m_fromHeight) {
                height = easy->m_fromHeight;
            } else {
                height = easy->RunEasy(easy->m_methodY, timeY, &easy->m_fromHeight, easy->m_overshootY);
            }
        } else {
            width = m_width;
            height = m_height;
        }
        SetRect(x, y, width, height, false);
        easy = m_easy;
    }
    float opacity = easy->m_opacityFrom;
    if (easy->m_opacityTo != opacity) {
        opacity = easy->RunOpacity(opacityTime);
    }
    SetOpacity(opacity);
    float scale = m_easy->m_scaleFrom;
    if (m_easy->m_scaleTo != scale) {
        scale = m_easy->RunScale(scaleTime);
    }
    SetScale(scale);
    m_flags &= ~FLAG_IN_EASY;
    float elapsed = timeY;
    if (timeX < timeY) {
        elapsed = timeX;
    }
    if (elapsed >= (&m_easy->m_fullTime)[m_easy->m_state == 2]) {
        EndEasy(this);
        OnEasyFinished();
    }
}

void Window::OnLayoutEasy(int x, int y, int width, int height)
{
    ResetEasy();
    if (m_tint != NULL) {
        m_tint->a = 1.0f;
    } else {
        m_opacity = 1.0f;
    }
    m_scale = 1.0f;
}

void Window::HandleUpdate(bool updateSelf)
{
    if (m_easy != NULL && m_easy->m_state != 0) {
        UpdateEasy();
        if ((m_flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
            return;
        }
    } else if ((m_flags & FLAG_SHOW_AFTER_EASY) != 0) {
        m_flags &= ~FLAG_SHOW_AFTER_EASY;
    } else if ((m_flags & FLAG_CLOSE_AFTER_EASY) != 0) {
        m_flags &= ~FLAG_CLOSE_AFTER_EASY;
        Close();
        return;
    }
    bool updateChildren = false;
    if (updateSelf && (m_flags & FLAG_UPDATE) != 0) {
        Update();
        updateChildren = updateSelf;
    }
    Window** firstLink = &m_firstChild;
    int pass = 1;
    while (true) {
        HandleFreeEventListeners();
        if ((m_flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
            return;
        }
        Window* firstChild = *firstLink;
        bool layoutChanged = false;
        bool focusFree = true;
        bool frontChild = true;
        bool hasClosing = false;
        for (Window* child = firstChild; child != NULL; child = child->m_nextSibling) {
            int flags = child->m_flags;
            if (flags < 0) {
                // Dead: the parent has to lay out again.
                layoutChanged = true;
                m_flags |= FLAG_LAYOUT_DIRTY;
                continue;
            }
            if ((flags & FLAG_CLOSING) != 0) {
                hasClosing = true;
                continue;
            }
            if ((flags & FLAG_STARTING) != 0) {
                // Only the front window may start.
                if (!frontChild) {
                    frontChild = false;
                    continue;
                }
                flags &= ~FLAG_STARTING;
                child->m_flags = flags;
            }
            bool initialized = false;
            if ((flags & FLAG_INITIALIZED) == 0) {
                child->m_flags = flags | FLAG_INITIALIZED;
                child->Init();
                if ((child->m_flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
                    layoutChanged = true;
                    m_flags |= FLAG_LAYOUT_DIRTY;
                    child->m_flags |= FLAG_DEAD | FLAG_SHUT_DOWN | FLAG_CLOSING;
                    child->OnClosed();
                    continue;
                }
                initialized = true;
                child->LayoutChanged();
                OnChildInitialized(child);
            }
            flags = child->m_flags;
            bool takesFocus;
            if ((m_flags & FLAG_FOCUSED) != 0 && (flags & FLAG_FOCUSABLE) != 0) {
                Window* focusChild = m_focusChild;
                if (focusChild == NULL && focusFree) {
                    takesFocus = true;
                } else {
                    takesFocus = focusChild == child;
                }
            } else {
                takesFocus = false;
            }
            bool stillFree = focusFree;
            if (takesFocus) {
                stillFree = false;
                if ((flags & FLAG_FOCUSED) == 0) {
                    for (Window* other = *firstLink; other != NULL; other = other->m_nextSibling) {
                        if ((other->m_flags & (FLAG_INITIALIZED | FLAG_FOCUSED)) == (FLAG_INITIALIZED | FLAG_FOCUSED)) {
                            other->HandleLostFocus();
                        }
                    }
                    flags = child->m_flags;
                    if ((flags & (FLAG_DEAD | FLAG_CLOSING)) == 0) {
                        child->m_flags = flags | FLAG_FOCUSED;
                        child->OnGetFocus();
                        flags = child->m_flags;
                    }
                    if ((flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
                        m_focusChild = NULL;
                        if (flags < 0) {
                            layoutChanged = true;
                            m_flags |= FLAG_LAYOUT_DIRTY;
                        } else {
                            hasClosing = true;
                        }
                        continue;
                    }
                }
            } else if ((flags & FLAG_FOCUSED) != 0) {
                child->HandleLostFocus();
                flags = child->m_flags;
                if ((flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
                    if (flags < 0) {
                        layoutChanged = true;
                        m_flags |= FLAG_LAYOUT_DIRTY;
                    } else {
                        hasClosing = true;
                    }
                    continue;
                }
            }
            if (pass == 1 || initialized) {
                child->HandleUpdate(updateChildren);
                flags = child->m_flags;
                if ((flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
                    focusFree = stillFree;
                    if (flags < 0) {
                        layoutChanged = true;
                        m_flags |= FLAG_LAYOUT_DIRTY;
                    } else {
                        hasClosing = true;
                    }
                    continue;
                }
            }
            focusFree = stillFree;
            frontChild = false;
        }
        if ((m_flags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
            return;
        }
        if ((m_flags & FLAG_LAYOUT_DIRTY) != 0) {
            bool parentLayouts = false;
            if (m_parent != NULL) {
                if ((m_parent->m_flags & FLAG_LAYOUT_DIRTY) != 0) {
                    parentLayouts = true;
                } else if (m_layout != NULL &&
                           (m_layout->m_type == LAYOUT_TABLE || m_layout->m_width.m_unit == UNIT_CONTENT ||
                            m_layout->m_height.m_unit == UNIT_CONTENT)) {
                    m_parent->m_flags |= FLAG_LAYOUT_DIRTY;
                    parentLayouts = true;
                }
            }
            if (!parentLayouts) {
                SetRect(m_x, m_y, m_width, m_height, false);
            }
        }
        if (hasClosing || layoutChanged) {
            // Shut down the closing children and delete the dead ones.
            bool lostFocus = false;
            WindowApp* app = WindowApp::m_instance;
            Window** link = firstLink;
            Window* child;
            while ((child = *link) != NULL) {
                int oldFlags = child->m_flags;
                if ((oldFlags & (FLAG_DEAD | FLAG_CLOSING)) != 0) {
                    child->HandleLostFocus();
                    if (m_focusChild == child) {
                        m_focusChild = NULL;
                    }
                    int flags = child->m_flags;
                    if ((flags & (FLAG_SHUT_DOWN | FLAG_CLOSING)) == FLAG_CLOSING) {
                        if ((flags & FLAG_INITIALIZED) != 0) {
                            child->OnClosed();
                            flags = child->m_flags;
                        }
                        flags |= FLAG_SHUT_DOWN;
                        child->m_flags = flags;
                    }
                    lostFocus = lostFocus | ((oldFlags & FLAG_FOCUSED) != 0);
                    if (flags < 0) {
                        *link = child->m_nextSibling;
                        child->m_nextSibling = NULL;
                        if ((child->m_flags & FLAG_DELETE_CHILDREN) != 0) {
                            child->DeleteChilds();
                        }
                        if ((child->m_flags & FLAG_INITIALIZED) != 0) {
                            child->m_flags &= ~FLAG_INITIALIZED;
                            child->Shutdown();
                        }
                        child->DeleteChilds();
                        child->ShutdownEventListeners();
                        child->m_parent = NULL;
                        delete child;
                        app->OnWindowDestroyed(child);
                        continue;
                    }
                    child = *link;
                }
                link = &child->m_nextSibling;
            }
            if (lostFocus) {
                continue;
            }
        }
        // Run again when the children changed (at most 10 passes).
        if (firstChild != *firstLink) {
            int currentPass = pass;
            pass++;
            if (currentPass < 10) {
                continue;
            }
        }
        Window* child = *firstLink;
        if (child != NULL && hasClosing) {
            // Closing windows are deleted next frame.
            do {
                if ((child->m_flags & (FLAG_DEAD | FLAG_CLOSING)) == FLAG_CLOSING) {
                    child->m_flags |= FLAG_DEAD;
                }
                child = child->m_nextSibling;
            } while (child != NULL);
        }
        return;
    }
}

void Window::HandlePause()
{
    OnPause();
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & FLAG_INITIALIZED) != 0) {
            child->HandlePause();
        }
    }
}

void Window::HandleResume()
{
    OnResume();
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & FLAG_INITIALIZED) != 0) {
            child->HandleResume();
        }
    }
}

void Window::HandleEnterBackground()
{
    OnEnterBackground();
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & FLAG_INITIALIZED) != 0) {
            child->HandleEnterBackground();
        }
    }
}

void Window::HandleEnterForeground()
{
    OnEnterForeground();
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & FLAG_INITIALIZED) != 0) {
            child->HandleEnterForeground();
        }
    }
}

void Window::HandleLayout()
{
    OnLayout();
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & (FLAG_DEAD | FLAG_INITIALIZED | FLAG_CLOSING | FLAG_RECT_PENDING)) == FLAG_INITIALIZED) {
            child->HandleLayout();
        }
    }
}

void Window::PaintChildren(ICGraphics2d& graphics)
{
    // The children are painted back to front (the list is front to back): the first four directly,
    // the others through the paint queue of the application.
    Window* first = m_firstChild;
    if (first == NULL || (m_flags & FLAG_PAINT_CHILDREN) == 0) {
        return;
    }
    Window* second = first->m_nextSibling;
    if (second != NULL) {
        Window* third = second->m_nextSibling;
        if (third != NULL) {
            Window* fourth = third->m_nextSibling;
            if (fourth != NULL) {
                WindowApp* app = WindowApp::m_instance;
                Window* other = fourth->m_nextSibling;
                if (other != NULL) {
                    int start = app->m_paintQueue.m_size;
                    do {
                        app->m_paintQueue.addElement(other);
                        other = other->m_nextSibling;
                    } while (other != NULL);
                    for (int index = app->m_paintQueue.m_size - 1; index >= start; index--) {
                        app->m_paintQueue.m_data[index]->HandlePaint(graphics);
                    }
                    app->m_paintQueue.m_size = start;
                }
                fourth->HandlePaint(graphics);
            }
            third->HandlePaint(graphics);
        }
        second->HandlePaint(graphics);
    }
    first->HandlePaint(graphics);
}

void Window::HandlePaint(ICGraphics2d& graphics)
{
    if ((m_flags & (FLAG_DEAD | FLAG_INITIALIZED | FLAG_STARTING | FLAG_CLOSING | FLAG_VISIBLE)) !=
        (FLAG_INITIALIZED | FLAG_VISIBLE)) {
        return;
    }
    if ((m_flags & (FLAG_DEAD | FLAG_INITIALIZED | FLAG_LAYOUT_DIRTY | FLAG_CLOSING)) ==
        (FLAG_INITIALIZED | FLAG_LAYOUT_DIRTY)) {
        if (m_parent != NULL && m_layout != NULL &&
            (m_layout->m_type == LAYOUT_TABLE || m_layout->m_width.m_unit == UNIT_CONTENT ||
             m_layout->m_height.m_unit == UNIT_CONTENT)) {
            m_parent->m_flags |= FLAG_LAYOUT_DIRTY;
        } else {
            SetRect(m_x, m_y, m_width, m_height, false);
        }
    }
    if (m_easy != NULL && m_easy->m_state != 0 && !m_easy->m_started) {
        UpdateEasy();
    }
    WindowApp* app = WindowApp::m_instance;
    // A scaled window stays centered on its rectangle.
    float shrink = (1.0f - m_scale) * 0.5f;
    float offsetX = (float)(int)((float)m_x + (float)m_width * shrink);
    float offsetY = (float)(int)((float)m_y + (float)m_height * shrink);
    const Window::Point& translation =
        app->PushTranslation((int)(offsetX * app->WinScale()), (int)(offsetY * app->WinScale()));
    float scale = app->PushWinScale(m_scale);
    CRectangle<short> savedClip;
    savedClip.m_x = 0;
    savedClip.m_y = 0;
    savedClip.m_width = 0;
    savedClip.m_height = 0;
    unsigned char clipEnabled;
    RasterizerState()->GetScissorState(clipEnabled, savedClip.m_x, savedClip.m_y, savedClip.m_width,
                                       savedClip.m_height);
    CRectangle<short> clip = savedClip;
    if (m_width > 0 && m_height > 0) {
        bool setClip;
        if ((m_flags & FLAG_CLIP_SCREEN) != 0) {
            setClip = true;
            clip.m_x = 0;
            clip.m_y = 0;
            clip.m_width = app->m_screenWidth;
            clip.m_height = app->m_screenHeight;
        } else if ((m_flags & FLAG_NO_CLIP) == 0) {
            short translationY = translation.y;
            clip.ClipRegionToRegion(translation.x, (short)(int)((float)m_width * scale), clip.m_x, clip.m_width);
            clip.ClipRegionToRegion(translationY, (short)(int)((float)m_height * scale), clip.m_y, clip.m_height);
            if (clip.m_width < 1 || clip.m_height < 1) {
                app->PopWinScale();
                app->PopTranslation();
                return;
            }
            setClip = true;
        } else {
            setClip = false;
        }
        graphics.Translate(offsetX, offsetY);
        if (m_scale > 0 && m_scale != 1.0f) {
            graphics.Scale(m_scale, m_scale);
        }
        if (setClip) {
            RasterizerState()->SetScissor(clip.m_x, clip.m_y, clip.m_width, clip.m_height);
        }
        if (m_tint == NULL) {
            app->PushOpacity(m_opacity);
        } else {
            app->PushColor(*m_tint);
        }
        app->OnBeginWindowPaint(graphics, this);
        if ((m_flags & FLAG_PAINT_3D) != 0) {
            RunPaint3D(graphics);
        } else {
            Paint(graphics);
        }
        app->OnEndWindowPaint(graphics, this);
        PaintChildren(graphics);
        app->PopColorStack();
        if (m_scale > 0 && m_scale != 1.0f) {
            graphics.Scale(1.0f / m_scale, 1.0f / m_scale);
        }
        graphics.Translate(-offsetX, -offsetY);
        if (setClip) {
            RasterizerState()->SetScissor(savedClip.m_x, savedClip.m_y, savedClip.m_width, savedClip.m_height);
        }
    }
    app->PopWinScale();
    app->PopTranslation();
}

void Window::RunPaint3D(ICGraphics2d& graphics)
{
    WindowApp* app = WindowApp::m_instance;
    app->OnBeginWindowPaint3D(graphics, this);
    CSwerveGraphics3D* renderer = CSwerve::GetInstance()->m_graphics3D;
    ICRenderSurface* target = ICGraphics::GetInstance()->GetTargetSurface();
    unsigned char clipEnabled;  // declared before viewport: original stack slot order (changed by fx2)
    CRectangle<short> viewport;
    viewport.m_x = 0;
    viewport.m_y = 0;
    viewport.m_width = 0;
    viewport.m_height = 0;
    RasterizerState()->GetScissorState(clipEnabled, viewport.m_x, viewport.m_y, viewport.m_width, viewport.m_height);
    renderer->BindTarget(target, &viewport, 0x40);
    const Window::Point& translation = app->Translation();
    // The Graphics3D wrapper of swerve_tw is a handle on the renderer (no reference counting).  // changed by fx2
    swerve::Graphics3D<CSwerveAssertingErrorHandler> graphics3D(CSwerve::GetInstance()->m_graphics3D);  // changed by fx2
    graphics3D.setViewport(translation.x, translation.y, m_width, m_height);  // changed by fx2
    Paint3D(graphics3D);  // changed by fx2
    renderer->ReleaseTarget();
    app->OnEndWindowPaint3D(graphics, this);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glClientActiveTexture(GL_TEXTURE0);
}

bool Window::ClipRect(ICGraphics2d& graphics, int x, int y, int width, int height)
{
    LocalToGlobal(x, y);
    unsigned char clipEnabled;
    CRectangle<short> clip;
    clip.m_x = 0;
    clip.m_y = 0;
    clip.m_width = 0;
    clip.m_height = 0;
    RasterizerState()->GetScissorState(clipEnabled, clip.m_x, clip.m_y, clip.m_width, clip.m_height);
    clip.ClipRegionToRegion(x, width, clip.m_x, clip.m_width);
    clip.ClipRegionToRegion(y, height, clip.m_y, clip.m_height);
    RasterizerState()->SetScissor(clip.m_x, clip.m_y, clip.m_width, clip.m_height);
    return (short)(clip.m_width | clip.m_height) < 1;
}

void Window::GetImageSize(ICRenderSurface* surface, int& width, int& height)
{
    unsigned int surfaceWidth;
    unsigned int surfaceHeight;
    if (surface != NULL) {
        surface->GetWidthAndHeight(surfaceWidth, surfaceHeight);
        width = surfaceWidth;
        height = surfaceHeight;
    } else {
        width = 0;
        height = 0;
    }
}

int Window::ImageWidth(ICRenderSurface* surface)
{
    if (surface != NULL) {
        unsigned int width;
        unsigned int height;
        surface->GetWidthAndHeight(width, height);
        return width;
    }
    return 0;
}

int Window::ImageHeight(ICRenderSurface* surface)
{
    if (surface != NULL) {
        unsigned int width;
        unsigned int height;
        surface->GetWidthAndHeight(width, height);
        return height;
    }
    return 0;
}

void Window::DrawFontText(ICGraphics2d& graphics, CFont* font, const wchar_t* text, int x, int y, int align)
{
    if (font == NULL) {
        return;
    }
    if ((align & ALIGN_NONE) == 0 && align != 0 && align != (ALIGN_LEFT | ALIGN_TOP)) {
        int width = font->MeasureTextWidth(text, -1, -1, NULL);
        int height = font->GetFontHeight();
        if ((align & ALIGN_HCENTER) != 0) {
            x -= width / 2;
        } else if ((align & (ALIGN_LEFT | ALIGN_RIGHT)) != (ALIGN_LEFT | ALIGN_RIGHT) && (align & ALIGN_LEFT) == 0 &&
                   (align & ALIGN_RIGHT) != 0) {
            x -= width;
        }
        if ((align & ALIGN_VCENTER) != 0) {
            y -= height / 2;
        } else if ((align & (ALIGN_TOP | ALIGN_BOTTOM)) != (ALIGN_TOP | ALIGN_BOTTOM) && (align & ALIGN_TOP) == 0 &&
                   (align & ALIGN_BOTTOM) != 0) {
            y -= height;
        }
    }
    font->PaintText(text, -1, x, y, -1, -1, &WindowApp::m_instance->Color());
}

void Window::FillRect(ICGraphics2d& graphics, int x, int y, int width, int height, unsigned int color)
{
    int alpha = (int)((float)(int)(color >> 24) * WindowApp::m_instance->Opacity());
    CDrawUtil::Fill(x, y, width + x, height + y, (color & 0xffffff) | (alpha << 24), (ICGraphics2d::Primitive)3);
}

void Window::BeginBlend(ICGraphics2d& graphics, float opacity, float green, float blue, float alpha, bool additive,
                        bool exact)
{
    if (exact) {
        WindowApp::m_instance->PushOpacityExact(opacity);
    } else {
        WindowApp::m_instance->PushOpacity(opacity);
    }
    RasterizerState()->PushState(ICRasterizerState::State_Blending);
    RasterizerState()->EnableBlending(1);
    RasterizerState()->SetBlendFactors(ICRasterizerState::BlendFactor_SrcAlpha,
                                       ICRasterizerState::BlendFactor_OneMinusSrcAlpha);
}

void Window::EndBlend(ICGraphics2d& graphics)
{
    RasterizerState()->PopState(ICRasterizerState::State_Blending);
    WindowApp::m_instance->PopColorStack();
}

void Window::DrawImage(ICGraphics2d& graphics, ICRenderSurface* surface, int x, int y, int align, int transform)
{
    if (surface == NULL) {
        return;
    }
    if ((align & ALIGN_NONE) == 0 && align != 0 && align != (ALIGN_LEFT | ALIGN_TOP)) {
        unsigned int width;
        unsigned int height;
        surface->GetWidthAndHeight(width, height);
        if ((align & ALIGN_HCENTER) != 0) {
            x -= (int)width / 2;
        } else if ((align & ALIGN_LEFT) == 0 && (align & (ALIGN_LEFT | ALIGN_RIGHT)) != (ALIGN_LEFT | ALIGN_RIGHT) &&
                   (align & ALIGN_RIGHT) != 0) {
            x -= width;
        }
        if ((align & ALIGN_VCENTER) != 0) {
            y -= (int)height / 2;
        } else if ((align & ALIGN_TOP) == 0 && (align & (ALIGN_TOP | ALIGN_BOTTOM)) != (ALIGN_TOP | ALIGN_BOTTOM) &&
                   (align & ALIGN_BOTTOM) != 0) {
            y -= height;
        }
    }
    graphics.PushTransform();
    // J2ME style transforms: mirror and rotation.
    ICGraphics2d::Flip flip = ICGraphics2d::Flip_None;
    switch (transform) {
    case 5:
        graphics.Rotate(90.0f);
        flip = ICGraphics2d::Flip_None;
        break;
    case 3:
        flip = ICGraphics2d::Flip_Both;
        break;
    case 6:
        graphics.Rotate(270.0f);
        flip = ICGraphics2d::Flip_None;
        break;
    case 7:
        graphics.Rotate(90.0f);
        flip = ICGraphics2d::Flip_Horizontal;
        break;
    case 1:
        flip = ICGraphics2d::Flip_Vertical;
        break;
    case 2:
        flip = ICGraphics2d::Flip_Horizontal;
        break;
    case 4:
        graphics.Rotate(270.0f);
        flip = ICGraphics2d::Flip_Horizontal;
        break;
    }
    graphics.Translate((float)x, (float)y);
    graphics.Draw(*surface, NULL, &WindowApp::m_instance->Color(), flip);
    graphics.PopTransform();
}

void Window::DrawTileImageRect(ICGraphics2d& graphics, ICRenderSurface* surface, int x, int y, int width, int height)
{
    if (surface == NULL) {
        return;
    }
    graphics.PushTransform();
    unsigned int tileWidth;
    unsigned int tileHeight;
    surface->GetWidthAndHeight(tileWidth, tileHeight);
    float stepX = (float)tileWidth;
    float stepY = (float)tileHeight;
    CRectangle<short> source = CRectangle<short>();
    source.m_x = 0;
    source.m_y = 0;
    graphics.Translate((float)x - stepX, (float)y - stepY);
    if (height > 0) {
        int bottom = height + y;
        do {
            graphics.Translate(0, stepY);
            int rowHeight = bottom - y;
            if ((int)tileHeight < rowHeight) {
                rowHeight = tileHeight;
            }
            source.m_height = rowHeight;
            graphics.PushTransform();
            int column = x;
            if (width > 0) {
                do {
                    graphics.Translate(stepX, 0);
                    int columnWidth = (width + x) - column;
                    if ((int)tileWidth < columnWidth) {
                        columnWidth = tileWidth;
                    }
                    source.m_width = columnWidth;
                    graphics.Draw(*surface, &source, &WindowApp::m_instance->Color(), ICGraphics2d::Flip_None);
                    column += tileWidth;
                } while (column < width + x);
            }
            graphics.PopTransform();
            y += tileHeight;
        } while (y < bottom);
    }
    graphics.PopTransform();
}

void Window::DrawTileImageRect(ICGraphics2d& graphics, ICRenderSurface* surface)
{
    DrawTileImageRect(graphics, surface, 0, 0, m_width, m_height);
}

void Window::DrawTileImageHoriz(ICGraphics2d& graphics, ICRenderSurface* surface, int x, int y, int width)
{
    if (surface == NULL) {
        return;
    }
    graphics.PushTransform();
    unsigned int tileWidth;
    unsigned int tileHeight;
    surface->GetWidthAndHeight(tileWidth, tileHeight);
    int tileSize = tileWidth;
    CRectangle<short> source = CRectangle<short>();
    source.m_x = 0;
    source.m_y = 0;
    source.m_width = tileWidth;
    source.m_height = tileHeight;
    graphics.Translate((float)x, (float)y);
    int end = width + x;
    int lastTile = end - tileSize;
    float step = (float)(unsigned int)tileSize;
    if (x < lastTile) {
        do {
            graphics.Draw(*surface, &source, &WindowApp::m_instance->Color(), ICGraphics2d::Flip_None);
            graphics.Translate(step, 0);
            x += tileWidth;
        } while (x < lastTile);
    }
    source.m_width = end - x;
    graphics.Draw(*surface, &source, &WindowApp::m_instance->Color(), ICGraphics2d::Flip_None);
    graphics.PopTransform();
}

void Window::DrawLineImageHoriz(ICGraphics2d& graphics, ICRenderSurface* left, ICRenderSurface* middle,
                                ICRenderSurface* right, int x, int y, int width)
{
    int leftWidth = ImageWidth(left);
    int rightWidth = ImageWidth(right);
    if (width < rightWidth + leftWidth) {
        // Too short for both ends: show a part of each.
        SaveClipRect saveClip(graphics);
        if (!ClipRect(graphics, x, 0, width, m_height)) {
            int split = leftWidth * width / (rightWidth + leftWidth);
            DrawImage(graphics, left, split + x, y, ALIGN_RIGHT | ALIGN_TOP, 0);
            DrawImage(graphics, right, split + x, y, ALIGN_LEFT | ALIGN_TOP, 0);
        }
    } else {
        int middleX = x;
        if (left != NULL) {
            middleX = x + leftWidth;
            width -= leftWidth;
            DrawImage(graphics, left, x, y, 0, 0);
        }
        if (right != NULL) {
            DrawImage(graphics, right, width + middleX, y, ALIGN_RIGHT | ALIGN_TOP, 0);
            width -= rightWidth;
        }
        DrawTileImageHoriz(graphics, middle, middleX, y, width);
    }
}

void Window::DrawRectImageHoriz(ICGraphics2d& graphics, ICRenderSurface* left, ICRenderSurface* middle,
                                ICRenderSurface* right, int x, int y, int width, int height)
{
    if (left != NULL) {
        DrawTileImageVert(graphics, left, x, y, height);
        int leftWidth = ImageWidth(left);
        width -= leftWidth;
        x += leftWidth;
    }
    if (right != NULL) {
        int rightWidth = ImageWidth(right);
        int rightEnd = width + x;
        width -= rightWidth;
        DrawTileImageVert(graphics, right, rightEnd - rightWidth, y, height);
    }
    DrawTileImageRect(graphics, middle, x, y, width, height);
}

void Window::DrawTileImageVert(ICGraphics2d& graphics, ICRenderSurface* surface, int x, int y, int height)
{
    if (surface == NULL) {
        return;
    }
    graphics.PushTransform();
    unsigned int tileHeight;
    unsigned int tileWidth;
    surface->GetWidthAndHeight(tileWidth, tileHeight);
    int tileSize = tileHeight;
    CRectangle<short> source = CRectangle<short>();
    source.m_x = 0;
    source.m_y = 0;
    source.m_width = tileWidth;
    source.m_height = tileHeight;
    graphics.Translate((float)x, (float)y);
    int end = height + y;
    int lastTile = end - tileSize;
    if (y < lastTile) {
        float step = (float)(unsigned int)tileSize;
        do {
            graphics.Draw(*surface, &source, &WindowApp::m_instance->Color(), ICGraphics2d::Flip_None);
            graphics.Translate(0, step);
            y += tileHeight;
        } while (y < lastTile);
    }
    source.m_height = end - y;
    graphics.Draw(*surface, &source, &WindowApp::m_instance->Color(), ICGraphics2d::Flip_None);
    graphics.PopTransform();
}

void Window::DrawLineImageVert(ICGraphics2d& graphics, ICRenderSurface* top, ICRenderSurface* middle,
                               ICRenderSurface* bottom, int x, int y, int height)
{
    int topHeight = ImageHeight(top);
    int bottomHeight = ImageHeight(bottom);
    if (height < bottomHeight + topHeight) {
        SaveClipRect saveClip(graphics);
        if (!ClipRect(graphics, 0, y, m_width, height)) {
            int split = topHeight * height / (bottomHeight + topHeight);
            DrawImage(graphics, top, x, split + y, ALIGN_LEFT | ALIGN_BOTTOM, 0);
            DrawImage(graphics, bottom, x, split + y, ALIGN_LEFT | ALIGN_TOP, 0);
        }
    } else {
        if (top != NULL) {
            DrawImage(graphics, top, x, y, 0, 0);
            y += topHeight;
            height -= topHeight;
        }
        if (bottom != NULL) {
            DrawImage(graphics, bottom, x, height + y, ALIGN_LEFT | ALIGN_BOTTOM, 0);
            height -= bottomHeight;
        }
        DrawTileImageVert(graphics, middle, x, y, height);
    }
}

void Window::DrawBorderFillRect(ICGraphics2d& graphics, ICRenderSurface* left, ICRenderSurface* topLeft,
                                ICRenderSurface* top, ICRenderSurface* topRight, ICRenderSurface* right,
                                ICRenderSurface* bottomRight, ICRenderSurface* bottom, ICRenderSurface* bottomLeft,
                                ICRenderSurface* center, int x, int y, int width, int height)
{
    SaveClipRect saveClip(graphics);
    if (!ClipRect(graphics, x, y, width, height)) {
        int imageWidth;
        int imageHeight;
        // Corners.
        int topX = x;
        int leftY = y;
        int topWidth = width;
        int leftHeight = height;
        if (topLeft != NULL) {
            DrawImage(graphics, topLeft, x, y, ALIGN_LEFT | ALIGN_TOP, 0);
            GetImageSize(topLeft, imageWidth, imageHeight);
            leftHeight = height - imageHeight;
            leftY = imageHeight + y;
            topWidth = width - imageWidth;
            topX = imageWidth + x;
        }
        int rightY;
        int rightHeight;
        if (topRight == NULL) {
            rightY = y;
            rightHeight = height;
        } else {
            DrawImage(graphics, topRight, width + x, y, ALIGN_RIGHT | ALIGN_TOP, 0);
            GetImageSize(topRight, imageWidth, imageHeight);
            rightHeight = height - imageHeight;
            rightY = imageHeight + y;
            topWidth -= imageWidth;
        }
        int bottomWidth = width;
        if (bottomRight != NULL) {
            DrawImage(graphics, bottomRight, width + x, height + y, ALIGN_RIGHT | ALIGN_BOTTOM, 0);
            GetImageSize(bottomRight, imageWidth, imageHeight);
            bottomWidth = width - imageWidth;
            rightHeight -= imageHeight;
        }
        int bottomY = height + y;
        int bottomX;
        if (bottomLeft == NULL) {
            bottomX = x;
        } else {
            DrawImage(graphics, bottomLeft, x, bottomY, ALIGN_LEFT | ALIGN_BOTTOM, 0);
            GetImageSize(bottomLeft, imageWidth, imageHeight);
            bottomWidth -= imageWidth;
            bottomX = imageWidth + x;
            leftHeight -= imageHeight;
        }
        // Edges.
        if (top != NULL) {
            DrawTileImageHoriz(graphics, top, topX, y, topWidth);
            y += ImageHeight(top);
        }
        if (bottom != NULL) {
            int bottomEdgeHeight = ImageHeight(bottom);
            bottomY -= bottomEdgeHeight;
            DrawTileImageHoriz(graphics, bottom, bottomX, bottomY, bottomWidth);
        }
        int centerX = x;
        if (left != NULL) {
            DrawTileImageVert(graphics, left, x, leftY, leftHeight);
            centerX = ImageWidth(left) + x;
        }
        int centerRight = width + x;
        if (right != NULL) {
            int rightEdgeWidth = ImageWidth(right);
            centerRight -= rightEdgeWidth;
            DrawTileImageVert(graphics, right, centerRight, rightY, rightHeight);
        }
        // Inside.
        if (center != NULL) {
            DrawTileImageRect(graphics, center, centerX, y, centerRight - centerX, bottomY - y);
        }
    }
}

void Window::DrawVertScrollbar(ICGraphics2d& graphics, int x, int y, int height, ICRenderSurface* top,
                               ICRenderSurface* bottom, ICRenderSurface* track, ICRenderSurface* thumb, float position,
                               float size, bool hideEndsAtLimits)
{
    if (position > 0.0f || !hideEndsAtLimits) {
        DrawImage(graphics, top, x, y, ALIGN_LEFT | ALIGN_TOP, 0);
    }
    if (position < 1.0f || !hideEndsAtLimits) {
        DrawImage(graphics, bottom, x, height + y, ALIGN_LEFT | ALIGN_BOTTOM, 0);
    }
    int topHeight = ImageHeight(top);
    int bottomHeight = ImageHeight(bottom);
    int trackEnd = y + height - 1 - bottomHeight;
    int trackStart = y + topHeight + 1;
    int trackHeight = trackEnd - trackStart;
    DrawTileImageVert(graphics, track, x, trackStart, trackHeight);
    if (size < 1.0f) {
        float thumbSize = 0.15f;
        if (size > 0.15f) {
            thumbSize = size;
        }
        float trackSize = (float)trackHeight;
        int thumbY = MathLib::Round(trackSize * ((1.0f - thumbSize) * position));
        int thumbHeight = MathLib::Round(trackSize * thumbSize);
        DrawTileImageVert(graphics, thumb, x, thumbY + trackStart, thumbHeight);
    }
}

void Window::DrawVertScrollbar(ICGraphics2d& graphics, int x, int y, int height, ICRenderSurface* top,
                               ICRenderSurface* bottom, ICRenderSurface* track, ICRenderSurface* thumb,
                               ICRenderSurface* thumbTop, ICRenderSurface* thumbBottom, float position, float size,
                               bool hideEndsAtLimits)
{
    if (position > 0.0f || !hideEndsAtLimits) {
        DrawImage(graphics, top, x, y, ALIGN_LEFT | ALIGN_TOP, 0);
    }
    if (position < 1.0f || !hideEndsAtLimits) {
        DrawImage(graphics, bottom, x, height + y, ALIGN_LEFT | ALIGN_BOTTOM, 0);
    }
    int topHeight = ImageHeight(top);
    int bottomHeight = ImageHeight(bottom);
    int trackEnd = y + height - 1 - bottomHeight;
    int trackStart = y + topHeight + 1;
    int trackHeight = trackEnd - trackStart;
    DrawTileImageVert(graphics, track, x, trackStart, trackHeight);
    if (size < 1.0f) {
        float thumbSize = 0.15f;
        if (size > 0.15f) {
            thumbSize = size;
        }
        float trackSize = (float)trackHeight;
        int thumbY = trackStart + MathLib::Round(trackSize * ((1.0f - thumbSize) * position));
        int thumbHeight = MathLib::Round(trackSize * thumbSize);
        DrawTileImageVert(graphics, thumb, x, thumbY, thumbHeight);
        // The original passes the thumb height as the align argument here.
        DrawImage(graphics, thumbTop, x + 7, thumbY - 7, thumbHeight, 0);
        DrawImage(graphics, thumbBottom, x + 7, thumbHeight + thumbY - 7, thumbHeight, 0);
    }
}

XString Window::ResString(const char* name)
{
    SmartRes resource;
    resource.Create(name);
    const wchar_t* text = L"";
    if (resource.m_resource != NULL) {
        text = ((CStrWChar*)resource.m_resource->GetData())->m_string;
    }
    return XString(text);
}

XString Window::ResStringSafe(XString name)
{
    if (name.ByteSize() == 0) {
        return name;
    }
    return ResString(XString::AnsiString(name.c_str()));
}

void WindowColor::Paint(ICGraphics2d& graphics)
{
    unsigned int color = m_color;
    int width = m_width;
    int height = m_height;
    int alpha = (int)((float)(int)(color >> 24) * WindowApp::m_instance->Opacity());
    CDrawUtil::Fill(0, 0, width, height, (color & 0xffffff) | (alpha << 24), (ICGraphics2d::Primitive)3);
}

SmartRes::SmartRes()
{
    m_resource = NULL;
}

SmartRes::SmartRes(const char* name)
{
    m_resource = NULL;
    Create(name);
}

bool SmartRes::Create(const char* name)
{
    if (name != NULL) {
        // Take the new resource before giving the old one back (it may be the same).
        CResource* oldResource = m_resource;
        ResourceManager()->CreateResource(name, &m_resource);
        if (m_resource != NULL && m_resource->GetData() == NULL) {
            ResourceManager()->ReleaseResource(&m_resource);
            m_resource = NULL;
        }
        if (oldResource != NULL) {
            ResourceManager()->ReleaseResource(&oldResource);
        }
    }
    return m_resource != NULL;
}

SmartRes::SmartRes(const SmartRes& other)
{
    m_resource = NULL;
    if (other.m_resource != NULL) {
        Create(other.m_resource->m_name);
    }
}

const char* SmartRes::GetResourceId() const
{
    if (m_resource != NULL) {
        return m_resource->m_name;
    }
    return NULL;
}

SmartRes::~SmartRes()
{
    Release();
}

void SmartRes::Release()
{
    if (m_resource == NULL) {
        return;
    }
    ResourceManager()->ReleaseResource(&m_resource);
    m_resource = NULL;
}

SmartRes& SmartRes::operator=(const SmartRes& other)
{
    if (m_resource == NULL || other.m_resource == NULL || m_resource->m_handle != other.m_resource->m_handle) {
        Release();
        if (other.m_resource != NULL) {
            ResourceManager()->CreateResource(other.m_resource->m_handle, &m_resource);
        }
    }
    return *this;
}

ImageRes& ImageRes::operator=(const ImageRes& other)
{
    SmartRes::operator=(other);
    return *this;
}

void Window::SetInterrupt(int interrupt)
{
    m_interrupt = interrupt;
    m_parent->fireInterrupt();
}

void Window::fireInterrupt()
{
    OnStateInterrupt(m_states[m_stateCount - 1].m_id, m_states[m_stateCount - 1].m_window);
}

int Window::GetInterrupt()
{
    return m_interrupt;
}

void Window::ClearInterrupt()
{
    m_interrupt = 0;
}

void Window::PopState()
{
    int current = -1;
    if (m_stateCount > 0) {
        m_stateCount--;
        m_states[m_stateCount].m_window->Close();
        current = -1;
        if (m_stateCount > 0) {
            current = m_states[m_stateCount - 1].m_id;
        }
    }
    m_currentState = current;
}

void Window::PushState(int state, Window* window)
{
    m_states[m_stateCount].m_id = state;
    m_states[m_stateCount].m_window = window;
    m_stateCount++;
    m_currentState = state;
    if (window->ShouldBeModal() == 1) {
        window->m_flags |= FLAG_MODAL;
    }
    AddToFront(window);
}

void Window::GoToState(int state)
{
    while (m_stateCount > 0) {
        if (m_states[m_stateCount - 1].m_id == state) {
            return;
        }
        PopState();
    }
    if (m_stateCount == 0) {
        AddNewState(state);
    }
}

void Window::AddNewState(int state)
{
    Window* window;
    OnCreateState(&window, state);
    if (window != NULL) {
        PushState(state, window);
    }
}

void Window::ChangeState(int state, eStateTransitionType transition)
{
    m_nextState = state;
    m_stateTransition = transition;
}

void Window::Update()
{
    int state = m_nextState;
    int transition = m_stateTransition;
    m_nextState = 0;
    m_stateTransition = STATE_NONE;
    switch (transition) {
    case STATE_PUSH:
        AddNewState(state);
        break;
    case STATE_REPLACE:
        PopState();
        AddNewState(state);
        break;
    case STATE_RESET:
        if (m_stateCount > 0) {
            do {
                PopState();
            } while (m_stateCount > 0);
        }
        AddNewState(state);
        break;
    case STATE_POP:
        PopState();
        break;
    case STATE_GOTO:
        GoToState(state);
        break;
    }
}

int Window::HasModalChild()
{
    for (Window* child = m_firstChild; child != NULL; child = child->m_nextSibling) {
        if ((child->m_flags & FLAG_MODAL) != 0) {
            return 1;
        }
    }
    return 0;
}
