// SimpleTextArea.h - a window that shows a text broken into lines; the text can be dragged
// up and down with the pointer. Used for the text of SimpleDialog.
// Owner: src/general/SimpleTextArea.cpp (b058). Slot order from the vtable of SimpleTextArea,
// fields from SimpleTextArea::SimpleTextArea and SimpleTextArea::SetText.
#ifndef SIMPLETEXTAREA_H
#define SIMPLETEXTAREA_H

#include "WindowBase.h"
#include "Vector_ss.h"
#include "xString.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class CFont;
class ICGraphics2d;
} } } }

class SimpleTextArea : public Window {
public:
    SimpleTextArea(com::glu::platform::graphics::CFont* font);
    virtual ~SimpleTextArea();

    virtual void Init();
    virtual void OnLayout();
    virtual void OnCommand(Event& event);
    virtual void OnPointerEvent(Event& event);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);

    virtual void SetFont(com::glu::platform::graphics::CFont* font);
    /** Set the text and break it into lines that fit the width of the window. */
    virtual void SetText(const XString& text);

    virtual int LinesNumber() const
    {
        return m_lines.size();
    }

    /** Height of one line (the font height). */
    virtual int LineHeight() const;

    /** Smallest useful height: one line. */
    virtual int MinHeight() const
    {
        return LineHeight();
    }

    virtual int MinWidth() const
    {
        return 0;
    }

    /** Draw line `line` at `y`. */
    virtual void DrawLine(com::glu::platform::graphics::ICGraphics2d& graphics, int y, int line);

    /** Scroll position 0..1 (1 when the whole text is visible). */
    float ScrollPos() const;

    com::glu::platform::graphics::CFont* m_font;    // 0xac
    XString m_text;                                 // 0xb0
    Vector<XString> m_lines;                        // 0xb4 m_text broken into lines
    int m_textHeight;                               // 0xc4 LinesNumber() * LineHeight()
    int m_scrollY;                                  // 0xc8 first visible pixel row of the text
    bool m_dragging;                                // 0xcc the pointer is pressed on the text
};

#endif
