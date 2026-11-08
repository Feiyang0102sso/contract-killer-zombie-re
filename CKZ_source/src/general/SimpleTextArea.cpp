// SimpleTextArea.cpp - a window that shows a text broken into lines; the text can be dragged
// up and down with the pointer.
#include "SimpleTextArea.h"
#include "WindowApp.h"
#include "CFont.h"

using namespace com::glu::platform::graphics;

SimpleTextArea::SimpleTextArea(CFont* font)
    : Window(false)
{
    FreeLayout();
    m_font = NULL;
    m_textHeight = 0;
    m_scrollY = 0;
    m_dragging = false;
    SetFont(font);
}

SimpleTextArea::~SimpleTextArea()
{
}

void SimpleTextArea::Init()
{
}

void SimpleTextArea::SetFont(CFont* font)
{
    m_font = font;
}

void SimpleTextArea::OnLayout()
{
    // break the text again for the new width
    SetText(m_text);
}

void SimpleTextArea::SetText(const XString& text)
{
    m_text = text;
    m_text.Split(m_lines, m_width, m_font);
    m_scrollY = 0;
    m_textHeight = LinesNumber() * LineHeight();
}

int SimpleTextArea::LineHeight() const
{
    return m_font->GetFontHeight();
}

float SimpleTextArea::ScrollPos() const
{
    if (m_textHeight > m_height) {
        return (float)m_scrollY / (float)(m_textHeight - m_height);
    }
    return 1.0f;
}

void SimpleTextArea::DrawLine(ICGraphics2d& graphics, int y, int line)
{
    const XString& text = m_lines[line];
    int x;
    // the original tests the whole flag word here: a window without flags draws left aligned
    if (m_flags == 0) {
        x = 0;
    } else {
        x = (m_width - m_font->MeasureTextWidth(text.c_str(), -1, -1, NULL)) / 2;
    }
    DrawFontText(graphics, m_font, text.c_str(), x, y, 0);
}

void SimpleTextArea::Paint(ICGraphics2d& graphics)
{
    int lineHeight = LineHeight();
    int line = m_scrollY / lineHeight;
    int y = -m_scrollY % lineHeight;
    int lineCount = LinesNumber();
    for (; y < m_height && line < lineCount; line++) {
        DrawLine(graphics, y, line);
        y += lineHeight;
    }
}

void SimpleTextArea::OnCommand(Event& event)
{
}

void SimpleTextArea::OnPointerEvent(Event& event)
{
    int dx;
    int dy;
    WindowApp::GetPointerOffset(event.m_param3, dx, dy);
    switch (event.m_type) {
    case Event::POINTER_PRESSED:
        m_dragging = true;
        break;
    case Event::POINTER_RELEASED:
        m_dragging = false;
        return;
    case Event::POINTER_DRAGGED: {
        int scrollY = m_scrollY - dy;
        if (m_textHeight - scrollY < m_height) {
            scrollY = m_textHeight - m_height;
        }
        if (scrollY < 0) {
            scrollY = 0;
        }
        m_scrollY = scrollY;
        break;
    }
    default:
        return;
    }
    // pressing and dragging are consumed, releasing is passed on
    event.Clear();
}
