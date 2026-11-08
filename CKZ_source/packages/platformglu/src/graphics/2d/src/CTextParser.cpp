// CTextParser.cpp - breaks a text into lines that fit a width with a given font.
#include "CTextParser.h"
#include "CFont.h"

namespace com { namespace glu { namespace platform { namespace graphics {

CTextParser::CTextParser()
{
    m_classId = ClassId;
    m_font = NULL;
    m_text = NULL;
    m_width = 0;
    m_numLines = 0;
}

CTextParser::~CTextParser()
{
}

int CTextParser::GetNumLines() const
{
    return m_numLines;
}

int CTextParser::GetIndexForLine(int line) const
{
    int index = 0;
    if (m_numLines > line) {
        // high half of the line entry
        const unsigned short* entry = (const unsigned short*)&m_lines.m_data[line];
        index = entry[1];
    }
    return index;
}

int CTextParser::GetNumCharsForLine(int line) const
{
    int numChars = 0;
    if (m_numLines > line) {
        // low half of the line entry
        const unsigned short* entry = (const unsigned short*)&m_lines.m_data[line];
        numChars = entry[0];
    }
    return numChars;
}

int CTextParser::Parse()
{
    m_numLines = 0;
    m_lines.Clear();
    if (m_font != NULL && m_text != NULL && m_width >= 1) {
        int fitCount = 0;
        int index = 0;
        int length = wcslen(m_text);
        int emptyLines = 0;
        while (index < length) {
            int tokenType = m_font->GetTokenType(m_text[index]);
            if (tokenType == CFont::TokenType_End) {
                break;
            }
            if (tokenType == CFont::TokenType_NewLine) {
                // count the line feeds; the empty lines are added before the next text
                emptyLines++;
                index++;
            } else {
                while (emptyLines > 0) {
                    int emptyLine = (index & 0xff) << 16;
                    m_lines.Add(emptyLine);
                    m_numLines++;
                    emptyLines--;
                }
                int width = m_font->MeasureTextWidth(m_text + index, length - index, m_width, &fitCount);
                fitCount = ParseLine(index, fitCount, width);
                if (fitCount < 1) {
                    break;
                }
                int line = (index << 16) | (fitCount & 0xffff);
                m_lines.Add(line);
                index += fitCount;
                m_numLines++;
                // a space or line feed ending the line is consumed with it
                tokenType = m_font->GetTokenType(m_text[index]);
                if (tokenType == CFont::TokenType_Space || tokenType == CFont::TokenType_NewLine) {
                    index++;
                }
            }
        }
    }
    return 0;
}

int CTextParser::ParseLine(int start, int count, int measuredWidth)
{
    int lineLength = count;
    unsigned char searching = 1;
    for (int position = start + count; position > start; position--) {
        int tokenType = m_font->GetTokenType(m_text[position]);
        switch (tokenType) {
        case CFont::TokenType_End:
        case CFont::TokenType_NewLine:
            lineLength = position - start;
            searching = 0;
            break;
        case CFont::TokenType_Space:
            // break before the space
            if (searching) {
                lineLength = position - start;
            }
            searching = 0;
            break;
        case CFont::TokenType_Break:
            // break after the break character when it still fits
            if (searching) {
                int charWidth = m_font->GetWidth(m_text[position]);
                int spacing = m_font->GetMinCharSpacing();
                if (position - start + 1 <= lineLength || m_width - measuredWidth >= charWidth + spacing) {
                    lineLength = position - start + 1;
                    searching = 0;
                }
            }
            break;
        }
    }
    return lineLength;
}

} } } }
