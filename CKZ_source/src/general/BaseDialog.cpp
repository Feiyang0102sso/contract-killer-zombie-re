// BaseDialog.cpp - basic building blocks of the dialogs: an image window, a multi-line text
// window and a container whose children can be scrolled by dragging (with inertia).
#include "BaseDialog.h"
#include <math.h>
#include "App.h"
#include "CBH_SurfaceManager.h"
#include "CFont.h"
#include "ICGraphics2d.h"
#include "CStdUtil_Cocoa.h"
#include "MathLib.h"

using namespace com::glu::platform::graphics;

/** The larger of two values (both are evaluated once, in order). */
static inline int MaxOf(int first, int second)
{
    if (first > second) {
        return first;
    }
    return second;
}

/** Blinking: period (ms) and brightness curve of BeginBlend. */
static const unsigned int BLINK_PERIOD = 600;
static const float BLINK_DIVISOR = 360.0f;
static const float BLINK_MIN = 0.2f;
/** Scroll bar: smallest thumb, as a part of the track. */
static const float MIN_THUMB_RATIO = 0.15f;
/** Scroll commands (key up / key down) and their speed, in screen heights per second. */
static const int COMMAND_SCROLL_UP = (int)0x972028c1;
static const int COMMAND_SCROLL_DOWN = 0x3f972120;
static const float SCROLL_STEP = 0.1f;
static const float SCROLL_MAX_SPEED = 0.9f;
/** Inertia: speed kept per frame and the speed below which scrolling stops. */
static const float FRICTION_X = 0.9f;
static const float FRICTION_Y = 0.95f;
static const float MIN_SPEED = 1.0f;

ImageWindow::ImageWindow()
    : Window(false), m_blinkCommand(0)
{
    m_scaleY = 0.0f;
    m_scaleX = 0.0f;
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_fieldC0 = 0;
    m_fieldC4 = 0;
    m_fillType = FILL_NONE;
    m_surface = NULL;
    m_blinking = false;
    SetDesiredWidth(0);
    SetDesiredHeight(0);
}

ImageWindow::ImageWindow(EFillType fillType, const ImageRes& image)
    : Window(false), m_blinkCommand(0)
{
    m_scaleY = 0.0f;
    m_scaleX = 0.0f;
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_image = image;
    m_surface = m_image.GetSurface();
    m_fieldC0 = 0;
    m_fieldC4 = 0;
    SetFillType(fillType);
    m_blinking = false;
}

void ImageWindow::SetFillType(EFillType fillType)
{
    m_fillType = fillType;
    int width = ImageWidth(m_surface);
    int height = ImageHeight(m_surface);
    if (m_scaleX > 0.0f) {
        width = (int)(width * m_scaleX);
    }
    if (m_scaleY > 0.0f) {
        height = (int)(height * m_scaleY);
    }
    switch (m_fillType) {
    case FILL_NONE:
        SetDesiredWidth(width);
        SetDesiredHeight(height);
        break;
    case FILL_HORIZONTAL:
        SetPercentWidth(100, 0, 0);
        SetDesiredHeight(height, 0, 0);
        break;
    case FILL_VERTICAL:
        SetDesiredWidth(width, 0, 0);
        SetPercentHeight(100, 0, 0);
        break;
    case FILL_BOTH:
        SetPercentWidth(100, 0, 0);
        SetPercentHeight(100, 0, 0);
        break;
    }
}

void ImageWindow::SetImageById(const char* id)
{
    m_surface = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(id, true, false);
    m_image.Release();
    m_surfaceId = XString(id);
    SetFillType(m_fillType);
}

void ImageWindow::SetImage(const ImageRes& image)
{
    m_image = image;
    m_surface = m_image.GetSurface();
    SetFillType(m_fillType);
}

void ImageWindow::SetImage(ICRenderSurface* surface)
{
    m_surface = surface;
    SetFillType(m_fillType);
}

void ImageWindow::Paint(ICGraphics2d& graphics)
{
    if (m_scaleX > 0.0f && m_scaleY > 0.0f) {
        graphics.Scale(m_scaleX, m_scaleY);
    }
    if (m_blinking) {
        int phase = CStdUtil_Cocoa::GetUpTimeMS() % BLINK_PERIOD - BLINK_PERIOD / 2;
        float brightness = fabsf((float)phase) / BLINK_DIVISOR + BLINK_MIN;
        BeginBlend(graphics, brightness, 1.0f, 1.0f, 0.0f, true, false);
    }
    switch (m_fillType) {
    case FILL_NONE: {
        int align = m_align;
        int x = 0;
        if ((align & ALIGN_LEFT) == 0) {
            if ((align & ALIGN_RIGHT) != 0) {
                x = m_width;
            } else {
                align |= ALIGN_HCENTER;
                x = m_width / 2;
            }
        }
        int y;
        if ((align & ALIGN_TOP) != 0) {
            y = 0;
        } else {
            y = m_height;
            if ((align & ALIGN_BOTTOM) == 0) {
                y = y / 2;
                align |= ALIGN_VCENTER;
            }
        }
        DrawImage(graphics, m_surface, x, y, align, 0);
        break;
    }
    case FILL_HORIZONTAL: {
        int y;
        if ((m_align & ALIGN_BOTTOM) != 0) {
            y = m_height - ImageHeight(m_surface);
        } else if ((m_align & ALIGN_VCENTER) != 0) {
            y = m_height / 2 - ImageHeight(m_surface) / 2;
        } else {
            y = 0;
        }
        DrawTileImageHoriz(graphics, m_surface, 0, y, m_width);
        break;
    }
    case FILL_VERTICAL: {
        int x;
        if ((m_align & ALIGN_RIGHT) != 0) {
            x = m_width - ImageWidth(m_surface);
        } else if ((m_align & ALIGN_HCENTER) != 0) {
            x = m_width / 2 - ImageWidth(m_surface) / 2;
        } else {
            x = 0;
        }
        DrawTileImageVert(graphics, m_surface, x, 0, m_height);
        break;
    }
    case FILL_BOTH:
        DrawTileImageRect(graphics, m_surface);
        break;
    }
    if (m_blinking) {
        EndBlend(graphics);
    }
    if (m_scaleX > 0.0f && m_scaleY > 0.0f) {
        graphics.Scale(1.0f / m_scaleX, 1.0f / m_scaleY);
    }
}

ImageWindow::~ImageWindow()
{
    if (m_surfaceId.ByteSize() != 0) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(XString::AnsiString(m_surfaceId.c_str()));
    }
}

void ImageWindow::OnCommand(Event& event)
{
    if (event.m_id == m_blinkCommand) {
        m_blinking = event.m_param1 != 0;
    }
}

TextWindow::TextWindow()
    : Window(false)
{
    m_textY = 0;
    m_textX = 0;
    m_fonts[1] = NULL;
    m_fonts[0] = NULL;
    m_fontIndex = 0;
    m_textWidth = 0;
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(ALIGN_LEFT | ALIGN_TOP);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}

TextWindow::TextWindow(const XString& text, CFont* font)
    : Window(false)
{
    m_textY = 0;
    m_textX = 0;
    m_fonts[1] = NULL;
    m_fonts[0] = NULL;
    m_fontIndex = 0;
    m_textWidth = 0;
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(ALIGN_LEFT | ALIGN_TOP);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    SetText(text, font, NULL, 0);
}

void TextWindow::SetText(const XString& text, CFont* font)
{
    SetText(text, font, NULL, 0);
}

void TextWindow::SetText(const XString& text, CFont* font, CFont* alternateFont, int fontIndex)
{
    if (m_fonts[0] != font || m_fonts[1] != alternateFont
        || XString::CmpData(m_text.c_str(), m_text.ByteSize(), text.c_str(), text.ByteSize(), 0x7fffffff) != 0) {
        m_fonts[0] = font;
        m_fonts[1] = alternateFont;
        m_text = text;
        m_fontIndex = fontIndex;
        m_lines.setSize(0);
        m_textWidth = 0;
    } else {
        if (m_fontIndex == fontIndex) {
            return;
        }
        m_fontIndex = fontIndex;
    }
    LayoutChanged();
}

TextWindow::TextWindow(const wchar_t* text, CFont* font)
    : Window(false)
{
    m_textY = 0;
    m_textX = 0;
    m_fonts[1] = NULL;
    m_fonts[0] = NULL;
    m_fontIndex = 0;
    m_textWidth = 0;
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(ALIGN_LEFT | ALIGN_TOP);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    SetText(XString(text), font, NULL, 0);
}

TextWindow::TextWindow(CFont* font)
    : Window(false)
{
    m_textY = 0;
    m_textX = 0;
    m_fonts[1] = NULL;
    m_fonts[0] = NULL;
    m_fontIndex = 0;
    m_textWidth = 0;
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(ALIGN_LEFT | ALIGN_TOP);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    SetText(m_text, font, NULL, 0);
}

void TextWindow::SetFont(CFont* font)
{
    SetText(m_text, font, NULL, 0);
}

void TextWindow::SetText(const XString& text)
{
    SetText(text, m_fonts[0], m_fonts[1], m_fontIndex);
}

void TextWindow::SetRect(int x, int y, int width, int height, bool force)
{
    int textWidth;
    if (m_layout->m_width.m_unit != UNIT_CONTENT) {
        m_text.Split(m_lines, width, m_fonts[m_fontIndex]);
        textWidth = width;
    } else {
        if (m_textWidth == 0) {
            // measure the widest line
            m_text.SplitLines(m_lines);
            for (int index = 0; index < m_lines.size(); index++) {
                XString line = m_lines[index];
                if (m_fonts[0] != NULL) {
                    m_textWidth = MaxOf(m_textWidth, m_fonts[0]->MeasureTextWidth(line.c_str(), -1, -1, NULL));
                }
                if (m_fonts[1] != NULL) {
                    m_textWidth = MaxOf(m_textWidth, m_fonts[1]->MeasureTextWidth(line.c_str(), -1, -1, NULL));
                }
            }
        }
        int maxWidth = m_layout->m_width.m_max;
        bool wrap = false;
        if (maxWidth > 0 && m_textWidth > maxWidth) {
            m_textWidth = maxWidth;
            wrap = true;
        }
        ELayoutType layoutType = LayoutType();
        textWidth = m_textWidth;
        if (layoutType == LAYOUT_TABLE) {
            if (textWidth < width) {
                textWidth = width;
            }
        }
        if (wrap) {
            m_text.Split(m_lines, textWidth, m_fonts[m_fontIndex]);
        }
    }
    int textHeight;
    if (m_layout->m_height.m_unit != UNIT_CONTENT || m_fonts[m_fontIndex] == NULL) {
        textHeight = height;
    } else {
        int lineHeight = m_fonts[m_fontIndex]->GetFontHeight();
        int maxHeight = m_layout->m_height.m_max;
        textHeight = m_lines.size() * lineHeight;
        if (maxHeight > 0 && maxHeight < textHeight) {
            textHeight = maxHeight;
        }
        if (LayoutType() == LAYOUT_TABLE) {
            if (textHeight < height) {
                textHeight = height;
            }
        }
    }
    Window::SetRect(x, y, textWidth, textHeight, force);
}

void TextWindow::Paint(ICGraphics2d& graphics)
{
    Window::Paint(graphics);
    CFont* font = m_fonts[m_fontIndex];
    if (font == NULL) {
        return;
    }
    int lineHeight = font->GetFontHeight();
    int y;
    if ((m_align & ALIGN_VCENTER) != 0) {
        int centered = (m_height - m_lines.size() * lineHeight) / 2;
        y = 0;
        if (centered >= 0) {
            y = centered;
        }
    } else {
        y = 0;
        if ((m_align & (ALIGN_TOP | ALIGN_BOTTOM)) == ALIGN_BOTTOM) {
            y = m_height - m_lines.size() * lineHeight;
            if (y < 0) {
                y = 0;
            }
        }
    }
    for (int index = 0; index < m_lines.size(); index++) {
        const XString& line = m_lines[index];
        int x;
        if ((m_align & ALIGN_HCENTER) != 0) {
            x = (m_width - font->MeasureTextWidth(line.c_str(), -1, -1, NULL)) / 2;
        } else if ((m_align & (ALIGN_LEFT | ALIGN_RIGHT)) == ALIGN_RIGHT) {
            x = m_width - font->MeasureTextWidth(line.c_str(), -1, -1, NULL);
        } else {
            x = 0;
        }
        DrawFontText(graphics, font, line.c_str(), x + m_textX, m_textY + y, 0);
        y += lineHeight;
    }
}

ScrollingContainer::VertScroller::VertScroller(ScrollingContainer* container)
    : Window(false)
{
    m_container = container;
    m_images[IMAGE_BAR_TOP].Create("SUR_IDB__SCROLLBAR_TOP_PNG");
    m_images[IMAGE_BAR_CENTER].Create("SUR_IDB__SCROLLBAR_CENTER_PNG");
    m_images[IMAGE_BAR_BOTTOM].Create("SUR_IDB__SCROLLBAR_BOTTOM_PNG");
    m_images[IMAGE_ARROW_UP].Create("SUR_IDB__SCROLLBAR_ARROW_UP_PNG");
    m_images[IMAGE_ARROW_DOWN].Create("SUR_IDB__SCROLLBAR_ARROW_DOWN_PNG");
    SetDesiredWidth(ImageWidth(m_images[IMAGE_BAR_CENTER].GetSurface()));
    SetPercentHeight(100, 0, 0);
    SetOutsetSpacing(0, 0, 0, 1);
}

void ScrollingContainer::VertScroller::OnPointerEvent(Event& event)
{
}

void ScrollingContainer::VertScroller::Paint(ICGraphics2d& graphics)
{
    ScrollingContainer* container = m_container;
    if (container->IsFullVisible()) {
        return;
    }
    float visibleRatio = (float)container->m_height / (float)container->m_contentHeight;
    float position = container->ScrollPos();
    float thumbRatio = MIN_THUMB_RATIO;
    if (visibleRatio > MIN_THUMB_RATIO) {
        thumbRatio = visibleRatio;
    }
    float track = (float)m_height - (float)ImageHeight(m_images[IMAGE_ARROW_UP].GetSurface())
                  - (float)ImageHeight(m_images[IMAGE_ARROW_DOWN].GetSurface());
    int thumbY = MathLib::Round(track * ((1.0f - thumbRatio) * position));
    int arrowHeight = ImageHeight(m_images[IMAGE_ARROW_UP].GetSurface());
    int thumbLength = MathLib::Round(track * thumbRatio);
    int x = m_width / 2;
    DrawImage(graphics, m_images[IMAGE_ARROW_UP].GetSurface(), x, 0, ALIGN_HCENTER | ALIGN_TOP, 0);
    DrawLineImageVert(graphics, m_images[IMAGE_BAR_TOP].GetSurface(), m_images[IMAGE_BAR_CENTER].GetSurface(), m_images[IMAGE_BAR_BOTTOM].GetSurface(),
                      x - ImageWidth(m_images[IMAGE_BAR_CENTER].GetSurface()) / 2, thumbY + arrowHeight, thumbLength);
    ICRenderSurface* marker = m_images[IMAGE_MARKER].GetSurface();
    int markerY = MathLib::Round((track - (float)ImageHeight(m_images[IMAGE_MARKER].GetSurface())) * position);
    DrawImage(graphics, marker, x, markerY, ALIGN_HCENTER | ALIGN_TOP, 0);
    DrawImage(graphics, m_images[IMAGE_ARROW_DOWN].GetSurface(), x, m_height, ALIGN_HCENTER | ALIGN_BOTTOM, 0);
}

bool ScrollingContainer::IsFullVisible() const
{
    return m_contentHeight <= m_height;
}

float ScrollingContainer::ScrollPos() const
{
    if (m_contentHeight > m_height) {
        return (float)-m_contentY / (float)(m_contentHeight - m_height);
    }
    return 1.0f;
}

ScrollingContainer::ScrollingContainer()
    : WindowTransparent(false)
{
    ClearFlags(FLAG_CHILDREN_FIRST | FLAG_POINTER_EXCLUSIVE);
    m_contentDirty = true;
    m_dragging = false;
    m_movingChildren = false;
    m_contentX = 0;
    m_contentY = 0;
    m_contentWidth = 0;
    m_contentHeight = 0;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
}

void ScrollingContainer::OnChildInitialized(Window* child)
{
    m_contentDirty = true;
}

void ScrollingContainer::OnChildRectChanged(Window* child)
{
    if (!m_movingChildren) {
        m_contentDirty = true;
    }
}

void ScrollingContainer::OnPointerPressed(Event& event)
{
    if (event.m_type != Event::NONE) {
        m_dragging = true;
        m_velocityX = 0.0f;
        m_velocityY = 0.0f;
    }
}

void ScrollingContainer::OnPointerDragged(Event& event)
{
    if (event.m_type == Event::NONE || !m_dragging) {
        return;
    }
    if (m_contentDirty) {
        ComputeContentRect(m_contentX, m_contentY, m_contentWidth, m_contentHeight, true);
        m_contentDirty = false;
    }
    if (m_contentHeight <= m_height) {
        // everything fits: move the content back to the origin and stop dragging
        if (m_contentY != 0 || m_contentX != 0) {
            m_movingChildren = true;
            MoveSimpleLayoutedChilds(-m_contentX, -m_contentY, true);
            m_movingChildren = false;
            m_contentX = 0;
            m_contentY = 0;
        }
        m_dragging = false;
    } else {
        int dx;
        int dy;
        WindowApp::GetPointerOffset(0, dx, dy);
        m_velocityX = 0.0f;
        m_velocityY = (float)dy / WindowApp::m_instance->m_frameTimeSeconds;
        int oldY = m_contentY;
        int y = dy + oldY;
        int newY = 0;
        if (y < 1) {
            newY = y;
            if (m_contentHeight + y < m_height) {
                newY = m_height - m_contentHeight;
            }
        }
        if (m_contentX != 0 || oldY != newY) {
            m_movingChildren = true;
            MoveSimpleLayoutedChilds(-m_contentX, newY - oldY, true);
            m_movingChildren = false;
            m_contentX = 0;
            m_contentY = newY;
            event.Clear();
            return;
        }
    }
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
}

void ScrollingContainer::OnPointerReleased(Event& event)
{
    m_dragging = false;
    if (WindowApp::PointerCapture(0) == this) {
        event.Clear();
    }
}

void ScrollingContainer::OnUnhandledEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED || event.m_type == Event::POINTER_DRAGGED) {
        event.Clear();
    } else if (event.m_type == Event::POINTER_RELEASED) {
        m_dragging = false;
        event.Clear();
    }
}

void ScrollingContainer::OnCommand(Event& event)
{
    int id = event.m_id;
    if (id == COMMAND_SCROLL_UP || id == COMMAND_SCROLL_DOWN) {
        float screenHeight = (float)WindowApp::m_instance->m_screenHeight;
        float direction = -1.0f;
        if (id == COMMAND_SCROLL_DOWN) {
            direction = 1.0f;
        }
        // b147: original loads all three constants up front (limits computed before the clamp).
        float velocity = m_velocityY + screenHeight * SCROLL_STEP * direction;
        float minVelocity = screenHeight * -SCROLL_MAX_SPEED;
        float maxVelocity = screenHeight * SCROLL_MAX_SPEED;
        if (velocity < minVelocity) {
            velocity = minVelocity;
        } else if (velocity > maxVelocity) {
            velocity = maxVelocity;
        }
        m_velocityY = velocity;
        event.Clear();
    }
}

void ScrollingContainer::Update()
{
    Window::Update();
    if (m_contentDirty) {
        ComputeContentRect(m_contentX, m_contentY, m_contentWidth, m_contentHeight, true);
        m_contentDirty = false;
    }
    Window* capture = WindowApp::PointerCapture(0);
    if (m_dragging && capture != this) {
        // a drag that moved to a window outside the container ends here
        if (capture == NULL || capture->IsChildOf(this) != true) {
            m_dragging = false;
        }
    }
    if (m_dragging) {
        return;
    }
    if (m_velocityX == 0.0f && m_velocityY == 0.0f) {
        return;
    }
    float frameTime = WindowApp::m_instance->m_frameTimeSeconds;
    int x = m_contentX + MathLib::Round(m_velocityX * frameTime);
    int y = m_contentY + MathLib::Round(m_velocityY * frameTime);
    if (x > 0 || m_contentWidth + x < m_width) {
        x = 0;
        m_velocityX = 0.0f;
    }
    if (y > 0) {
        y = 0;
        m_velocityY = 0.0f;
    } else if (m_contentHeight + y < m_height) {
        m_velocityY = 0.0f;
        y = m_height - m_contentHeight;
    }
    if (x != m_contentX || y != m_contentY) {
        m_movingChildren = true;
        MoveSimpleLayoutedChilds(x - m_contentX, y - m_contentY, true);
        m_movingChildren = false;
        m_contentX = x;
        m_contentY = y;
        m_contentDirty = false;
    }
    m_velocityX = m_velocityX * FRICTION_X;
    m_velocityY = m_velocityY * FRICTION_Y;
    if (fabsf(m_velocityX) <= MIN_SPEED) {
        m_velocityX = 0.0f;
    }
    if (fabsf(m_velocityY) <= MIN_SPEED) {
        m_velocityY = 0.0f;
    }
}
