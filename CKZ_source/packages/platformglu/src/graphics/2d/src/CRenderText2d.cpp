// CRenderText2d.cpp - the 2d text model of the platform device.
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "CRenderText2d.h"
#include "CSystemFont.h"
#include "ICRenderSurface.h"
#include "CInputStream.h"
#include "CStrChar.h"
#include "CStrWChar.h"
#include "CUtf.h"
#include "CStdUtil_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// ---- constants ----
static const unsigned int RenderText2d_WorkBufferSize = 0x1000;     // decode buffer (bytes)
static const unsigned int RenderText2d_WorkHalf = 0x800;            // second half: the source text

CRenderText2d::CRenderText2d()
    : m_model()
{
    m_model.m_flags = Flag_Hardware;
}

CRenderText2d::~CRenderText2d()
{
    Destroy();
}

void CRenderText2d::Destroy()
{
    if (m_model.m_surface != NULL) {
        delete m_model.m_surface;
        m_model.m_surface = NULL;
    }
    // a text decoded from a stream is owned
    if (!(m_model.m_flags & Flag_TextReferenced)) {
        delete[] (wchar_t*)m_model.m_text;
    }
    m_model.m_length = 0;
    m_model.m_textType = 0;
    m_model.m_text = NULL;
    m_model.m_font = NULL;
    m_model.m_flags &= (Flag_Software | Flag_Hardware);
}

unsigned char CRenderText2d::Load(components::CInputStream& stream, unsigned int type)
{
    stream.Available();
    return 0;
}

unsigned char CRenderText2d::Initialize(const InitParam* params)
{
    unsigned int offset = 0;
    unsigned short span = 0xffff;
    Destroy();

    const void* text = NULL;
    unsigned int textType = 0;
    components::CInputStream* stream = NULL;
    unsigned int streamType = 0;
    void** formatArguments = NULL;
    unsigned char invalid = 0;
    const InitParam* param = params;
    while (!invalid && param->m_key != InitParam_End) {
        switch (param->m_key) {
        case InitParam_Font:
            m_model.m_font = (CFont*)param->m_value;
            break;
        case InitParam_Text:
            text = (const void*)param->m_value;
            break;
        case InitParam_TextType:
            // a referenced text excludes a stream
            invalid = 1;
            if (textType == 0 && stream == NULL && streamType == 0 && formatArguments == NULL) {
                textType = param->m_value;
                invalid = 0;
            }
            break;
        case InitParam_Stream:
            stream = (components::CInputStream*)param->m_value;
            break;
        case InitParam_StreamType:
            invalid = 1;
            if (streamType == 0 && text == NULL && textType == 0) {
                streamType = param->m_value;
                invalid = 0;
            }
            break;
        case InitParam_FormatArguments:
            invalid = 1;
            if (formatArguments == NULL && text == NULL && textType == 0) {
                formatArguments = (void**)param->m_value;
                invalid = 0;
            }
            break;
        case InitParam_Offset:
            offset = param->m_value;
            break;
        case InitParam_Length:
            invalid = 1;
            if (param->m_value < 0x10000) {
                span = param->m_value;
                invalid = 0;
            }
            break;
        }
        param++;
    }

    if (m_model.m_font != NULL) {
        unsigned int fontType = m_model.m_font->GetType();
        if (fontType == ICFont::Type_System) {
            m_model.m_flags |= Flag_SystemFont;
        } else if (fontType == ICFont::Type_Bitmap) {
            m_model.m_flags |= Flag_BitmapFont;
        } else {
            Destroy();
            return 0;
        }
        if (!invalid) {
            if (text != NULL) {
                // reference the caller's text
                if (textType == MimeType_Char || textType == MimeType_WChar) {
                    ClipOffsetAndSpan(text, textType, offset, span);
                    if (span != 0) {
                        if (textType == MimeType_WChar) {
                            m_model.m_text = (const wchar_t*)text + offset;
                        } else if (textType == MimeType_Char) {
                            m_model.m_text = (const char*)text + offset;
                        }
                    }
                    m_model.m_length = span;
                    m_model.m_textType = textType;
                    m_model.m_flags |= Flag_TextReferenced;
                    if (!(m_model.m_flags & Flag_SystemFont) || AssocSystemFont()) {
                        m_model.m_flags |= Flag_Initialized;
                        return 1;
                    }
                }
            } else if (stream != NULL && streamType != 0) {
                // decode the text from the stream
                wchar_t* modelText;
                CreateModelWStr(&modelText, *stream, streamType, offset, span, formatArguments);
                if (modelText != NULL) {
                    m_model.m_length = wcslen(modelText);
                    m_model.m_text = modelText;
                }
                m_model.m_textType = MimeType_WChar;
                if (!(m_model.m_flags & Flag_SystemFont) || AssocSystemFont()) {
                    m_model.m_flags |= Flag_Initialized;
                    return 1;
                }
            }
        }
    }
    Destroy();
    return 0;
}

void CRenderText2d::ClipOffsetAndSpan(const void* text, unsigned int type, unsigned int& offset, unsigned short& span)
{
    unsigned int length;
    switch (type) {
    case MimeType_StrChar:
        // measure the string object's buffer
        text = ((const components::CStrChar*)text)->m_string;
    case MimeType_Char:
        length = strlen((const char*)text);
        break;
    default:
        offset = 0;
        span = 0;
        return;
    case MimeType_StrWChar:
    case MimeType_Utf8:
        text = ((const components::CStrWChar*)text)->m_string;
    case MimeType_WChar:
        length = wcslen((const wchar_t*)text);
        break;
    }
    if (offset > length) {
        offset = 0;
        span = 0;
        return;
    }
    if (offset + span > length) {
        span = length - offset;
    }
}

void CRenderText2d::CreateModelWStr(wchar_t** text, components::CInputStream& stream, unsigned int type,
                                    unsigned int offset, unsigned int span, void** formatArguments)
{
    *text = NULL;
    if (span == 0) {
        return;
    }
    unsigned int available = stream.Available();
    if (type == MimeType_Utf8 || type == MimeType_WChar) {
        if (formatArguments != NULL) {
            // first half: the formatted text; second half (4 byte aligned): the format
            unsigned char* block = (unsigned char*)malloc(RenderText2d_WorkBufferSize);
            wchar_t* format = (wchar_t*)(block + ((0 - (unsigned int)block) & 3) + RenderText2d_WorkHalf);
            if (type == MimeType_Utf8) {
                stream.Read(block, available);
                core::CUtf::Utf8ToWcs(block, available, format, RenderText2d_WorkHalf);
                unsigned int length = wcslen(format) + 1;
                if (length <= offset) {
                    free(block);
                    return;
                }
                if (offset + span > length) {
                    span = length - offset;
                }
                if (span == 0) {
                    free(block);
                    return;
                }
                format[offset + span] = 0;
                CStdUtil_Cocoa::VSWPrintF((wchar_t*)block, format + offset, *formatArguments);
            } else {
                stream.Skip(offset * 2);
                unsigned int size = stream.Available();
                if (size > span / 2) {
                    size = span * 2 + 2;
                } else {
                    span = (size - 1) / 2;
                }
                if (size == 0) {
                    free(block);
                    return;
                }
                stream.Read((unsigned char*)format, size);
                format[span] = 0;
                CStdUtil_Cocoa::VSWPrintF((wchar_t*)block, format, *formatArguments);
            }
            unsigned int bytes = wcslen((wchar_t*)block) * 4 + 4;
            if (bytes != 0) {
                *text = (wchar_t*)malloc(bytes);
                wcscpy(*text, (wchar_t*)block);
            }
            free(block);
        } else if (type == MimeType_Utf8) {
            // first half: the decoded text; second half (4 byte aligned): the UTF-8 bytes
            unsigned char* block = (unsigned char*)malloc(RenderText2d_WorkBufferSize);
            unsigned char* source = block + ((0 - (unsigned int)block) & 3) + RenderText2d_WorkHalf;
            stream.Read(source, available);
            core::CUtf::Utf8ToWcs(source, available, (wchar_t*)block, RenderText2d_WorkHalf);
            // (measures the source half, as the original does)
            unsigned int length = wcslen((wchar_t*)source) + 1;
            if (length > offset) {
                if (offset + span > length) {
                    span = length - offset;
                }
                if (span != 0) {
                    ((wchar_t*)block)[offset + span] = 0;
                    wchar_t* start = (wchar_t*)block + offset;
                    *text = (wchar_t*)malloc(wcslen(start) * 4 + 4);
                    wcscpy(*text, start);
                }
            }
            free(block);
        } else {
            stream.Skip(offset * 2);
            unsigned int size = stream.Available();
            if (size > span / 2) {
                size = span * 2 + 2;
            } else {
                span = (size - 1) / 2;
            }
            if (size != 0) {
                *text = (wchar_t*)malloc(size);
                stream.Read((unsigned char*)*text, size);
                (*text)[span] = 0;
            }
        }
    } else if (type == MimeType_Char) {
        stream.Skip(offset);
        unsigned int size = stream.Available();
        if (size > span) {
            size = span + 1;
        } else {
            span = size - 1;
        }
        if (size == 0) {
            return;
        }
        char* block = (char*)malloc(RenderText2d_WorkBufferSize);
        if (formatArguments != NULL) {
            char* format = block + RenderText2d_WorkHalf;
            stream.Read((unsigned char*)format, size);
            format[span] = 0;
            CStdUtil_Cocoa::VSPrintF(block, format, *formatArguments);
            size = strlen(block) + 1;
        } else {
            stream.Read((unsigned char*)block, size);
            block[span] = 0;
        }
        *text = (wchar_t*)malloc(size * 2);
        mbstowcs(*text, block, strlen(block) + 1);
        free(block);
    } else {
        *text = NULL;
    }
}

unsigned char CRenderText2d::AssocSystemFont()
{
    if (m_model.m_surface == NULL) {
        m_model.m_surface = ICRenderSurface::CreateInstance(ICRenderSurface::Type_Default,
                                                            ICRenderSurface::Targetability_0);
    }
    CSystemFont* font = (CSystemFont*)m_model.m_font;
    return font->InitializeSurfaceWithText(*m_model.m_surface, (const wchar_t*)m_model.m_text, m_model.m_length,
                                           -1, -1);
}

unsigned char CRenderText2d::GetDescription(Description& description)
{
    unsigned char result;
    if (m_model.m_flags & Flag_Software) {
        description.m_abstraction = ICGraphicsAbstract::Abstraction_Software;
        result = 1;
    } else {
        result = 0;
        if (m_model.m_flags & Flag_Hardware) {
            result = 1;
            description.m_abstraction = ICGraphicsAbstract::Abstraction_Hardware;
        }
    }
    description.m_font = m_model.m_font;
    description.m_text = m_model.m_text;
    description.m_textType = m_model.m_textType;
    description.m_length = m_model.m_length;
    return result;
}

ICGraphicsAbstract::Abstraction CRenderText2d::GetAbstraction() const
{
    if (m_model.m_flags & Flag_Software) {
        return ICGraphicsAbstract::Abstraction_Software;
    }
    if (m_model.m_flags & Flag_Hardware) {
        return ICGraphicsAbstract::Abstraction_Hardware;
    }
    return ICGraphicsAbstract::Abstraction_None;
}

} } } }
