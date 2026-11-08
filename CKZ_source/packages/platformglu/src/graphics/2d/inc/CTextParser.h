// CTextParser.h - breaks a text into lines that fit a width with a given font (created by b034x).
#ifndef CTEXTPARSER_H
#define CTEXTPARSER_H

#include <stddef.h>
#include <wchar.h>
#include "CClass.h"
#include "CVector.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CFont;

class CTextParser : public core::CClass {
public:
    enum { ClassId = 0xf166a300 };

    CTextParser();
    virtual ~CTextParser();

    int GetNumLines() const;
    /** First character of `line` (0 when out of range). */
    int GetIndexForLine(int line) const;
    /** Characters of `line` (0 when out of range). */
    int GetNumCharsForLine(int line) const;
    /** Split m_text into lines of at most m_width pixels. */
    int Parse();
    /**
     * Characters of the line starting at `start`, `count` of which fit in the width (measured as
     * `measuredWidth` pixels): moves the break back to the last space / break character.
     */
    int ParseLine(int start, int count, int measuredWidth);

    CFont* m_font;                  // 0x08
    const wchar_t* m_text;          // 0x0c
    int m_width;                    // 0x10 line width in pixels
    int m_numLines;                 // 0x14
    core::CVector<int> m_lines;     // 0x18 per line: characters (low 16 bits), first index (high 16 bits)
};

} } } }

#endif
