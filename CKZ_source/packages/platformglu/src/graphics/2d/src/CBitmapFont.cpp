// CBitmapFont.cpp - font drawn from a glyph sheet (a render surface) and a metrics table.
#include <string.h>
#include <wchar.h>
#include "CBitmapFont.h"
#include "ICGraphics2d.h"
#include "ICRenderSurface.h"
#include "CInputStream.h"
#include "CArrayInputStream.h"
#include "CIdToObjectRouter.h"
#include "CRectangle.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// ---- constants ----
// Load() tags: what follows (a resource key or a NUL terminated name) and the InitParam it fills
static const unsigned int BitmapFont_TagImageById = 0xa6601847;
static const unsigned int BitmapFont_TagImageByName = 0x1cfda15a;
static const unsigned int BitmapFont_TagImageStreamById = 0xa0ca9e08;
static const unsigned int BitmapFont_TagImageStreamByName = 0x31c237d3;
static const unsigned int BitmapFont_TagDataById = 0x2b66d8a6;
static const unsigned int BitmapFont_TagDataByName = 0x01cf0337;
static const unsigned int BitmapFont_TagDataStreamById = 0x7d4bae73;
static const unsigned int BitmapFont_TagDataStreamByName = 0xc45a54ee;

static const unsigned char BitmapFont_FormatJMUtfCodes = 2;     // FontHeader::m_format: codes as one JM UTF string

/** InitParam key filled by a Load() tag. */
static inline unsigned int BitmapFont_GetInitParamKey(unsigned int tag)
{
    unsigned int key = ICFont::InitParam_End;
    switch (tag) {
    case BitmapFont_TagImageById:
    case BitmapFont_TagImageByName:
        key = ICFont::InitParam_Image;
        break;
    case BitmapFont_TagImageStreamById:
    case BitmapFont_TagImageStreamByName:
        key = ICFont::InitParam_ImageStream;
        break;
    case BitmapFont_TagDataById:
    case BitmapFont_TagDataByName:
        key = ICFont::InitParam_Data;
        break;
    case BitmapFont_TagDataStreamById:
    case BitmapFont_TagDataStreamByName:
        key = ICFont::InitParam_DataStream;
        break;
    }
    return key;
}

/** Read a NUL terminated name into `name`. */
static inline void BitmapFont_ReadName(components::CInputStream& stream, char* name)
{
    unsigned char character;
    do {
        character = stream.ReadUInt8();
        *name = character;
        name++;
    } while (character != 0);
}

CBitmapFont::CBitmapFont()
{
    m_data = NULL;
    m_image = NULL;
    m_header = NULL;
    m_charDescs = NULL;
    m_controlCharDescs = NULL;
    m_sharedImage = NULL;
}

CBitmapFont::~CBitmapFont()
{
    Destroy();
}

void CBitmapFont::Destroy()
{
    if (m_sharedImage != NULL) {
        delete m_sharedImage;
        m_sharedImage = NULL;
    }
    if (m_header != NULL) {
        delete m_header;
        m_header = NULL;
    }
    if (m_charDescs != NULL) {
        delete[] m_charDescs;
        m_charDescs = NULL;
    }
    if (m_controlCharDescs != NULL) {
        delete[] m_controlCharDescs;
        m_controlCharDescs = NULL;
    }
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    m_image = NULL;
}

unsigned char CBitmapFont::Load(components::CInputStream& stream, unsigned int type,
                                components::CIdToObjectRouter* router)
{
    // (declared in this order: the original stack layout)
    InitParam params[5];
    char name[256];
    components::CInputStream streams[2];            // [0] glyph image, [1] metrics
    unsigned int pointerTypes[2] = { 0, 0 };
    unsigned int streamTypes[2] = { 0, 0 };
    core::CSharedPtr<void*> data;                   // routed metrics object
    memset(params, 0, sizeof(params));

    unsigned char ok = 1;
    int count = 0;
    while (stream.Available() != 0) {
        unsigned int tag = stream.ReadUInt32();
        switch (tag) {
        case BitmapFont_TagImageById:
            m_sharedImage = new core::CSharedPtr<void*>();
            ok = router->KeyToPointer(stream.ReadUInt32(), *m_sharedImage, pointerTypes[0]);
            if (ok) {
                params[count].m_key = InitParam_Image;
                params[count].m_value = (unsigned int)**m_sharedImage;
            }
            break;
        case BitmapFont_TagImageByName:
            BitmapFont_ReadName(stream, name);
            m_sharedImage = new core::CSharedPtr<void*>();
            ok = router->NameToPointer(name, *m_sharedImage, pointerTypes[0]);
            if (ok) {
                params[count].m_key = BitmapFont_GetInitParamKey(tag);
                params[count].m_value = (unsigned int)**m_sharedImage;
            }
            break;
        case BitmapFont_TagImageStreamById:
            ok = router->KeyToInputStream(stream.ReadUInt32(), streams[0], streamTypes[0]);
            if (ok) {
                params[count].m_key = InitParam_ImageStream;
                params[count].m_value = (unsigned int)&streams[0];
            }
            break;
        case BitmapFont_TagImageStreamByName:
            BitmapFont_ReadName(stream, name);
            ok = router->NameToInputStream(name, streams[0], streamTypes[0]);
            if (ok) {
                params[count].m_key = BitmapFont_GetInitParamKey(tag);
                params[count].m_value = (unsigned int)&streams[0];
            }
            break;
        case BitmapFont_TagDataById:
            ok = router->KeyToPointer(stream.ReadUInt32(), data, pointerTypes[1]);
            if (ok) {
                params[count].m_key = InitParam_Data;
                params[count].m_value = (unsigned int)*data;
            }
            break;
        case BitmapFont_TagDataByName:
            BitmapFont_ReadName(stream, name);
            ok = router->NameToPointer(name, data, pointerTypes[1]);
            if (ok) {
                params[count].m_key = BitmapFont_GetInitParamKey(tag);
                params[count].m_value = (unsigned int)*data;
            }
            break;
        case BitmapFont_TagDataStreamById:
            ok = router->KeyToInputStream(stream.ReadUInt32(), streams[1], streamTypes[1]);
            if (ok) {
                params[count].m_key = InitParam_DataStream;
                params[count].m_value = (unsigned int)&streams[1];
            }
            break;
        case BitmapFont_TagDataStreamByName:
            BitmapFont_ReadName(stream, name);
            ok = router->NameToInputStream(name, streams[1], streamTypes[1]);
            if (ok) {
                params[count].m_key = BitmapFont_GetInitParamKey(tag);
                params[count].m_value = (unsigned int)&streams[1];
            }
            break;
        default:
            // a plain (key, value) pair
            params[count].m_key = InitParam_End;
            params[count].m_value = stream.ReadUInt32();
            break;
        }
        count++;
    }

    // the types reported by the router
    if (pointerTypes[0] != 0) {
        params[count].m_key = InitParam_ImageType;
        params[count].m_value = pointerTypes[0];
        count++;
    }
    if (pointerTypes[1] != 0) {
        params[count].m_key = InitParam_DataType;
        params[count].m_value = pointerTypes[1];
        count++;
    }
    if (streamTypes[0] != 0) {
        params[count].m_key = InitParam_ImageStreamType;
        params[count].m_value = streamTypes[0];
        count++;
    }
    if (streamTypes[1] != 0) {
        params[count].m_key = InitParam_DataStreamType;
        params[count].m_value = streamTypes[1];
        count++;
    }

    unsigned char result = 0;
    if (ok) {
        result = Initialize(params);
    }
    return result;
}

unsigned char CBitmapFont::Initialize(const InitParam* params)
{
    components::CInputStream* imageStream = NULL;
    void* data = NULL;
    unsigned int dataType = 0;
    components::CInputStream* dataStream = NULL;
    unsigned int dataStreamType = 0;
    unsigned char done = 0;
    const InitParam* param = params;
    while (!done) {
        switch (param->m_key) {
        case InitParam_End:
            done = 1;
            break;
        case InitParam_Image:
            m_image = (ICRenderSurface*)param->m_value;
            break;
        case InitParam_ImageStream:
            imageStream = (components::CInputStream*)param->m_value;
            break;
        case InitParam_Data:
            data = (void*)param->m_value;
            break;
        case InitParam_DataType:
            dataType = param->m_value;
            break;
        case InitParam_DataStream:
            dataStream = (components::CInputStream*)param->m_value;
            break;
        case InitParam_DataStreamType:
            dataStreamType = param->m_value;
            break;
        }
        param++;
    }

    // the metrics come from a stream of the right type; image streams are not supported
    // (evaluated without short circuits)
    unsigned char valid = ((dataStreamType == MimeType_FontData) | (dataStream == NULL)) &
                          ((dataType == MimeType_FontData) | (data == NULL)) &
                          ((data == NULL) | (dataStream == NULL)) &
                          ((m_image == NULL) | (imageStream == NULL));
    unsigned int size = 0;
    if (dataStream != NULL) {
        size = dataStream->Available();
        m_data = new unsigned char[size];
        dataStream->Read(m_data, size);
        if (dataStream->m_error) {
            Destroy();
            return 0;
        }
    }
    if (!valid || imageStream != NULL) {
        Destroy();
        return 0;
    }
    ParseFontMetrics(m_data, size);
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    return 1;
}

void CBitmapFont::ParseFontMetrics(const unsigned char* data, unsigned int size)
{
    m_header = new FontHeader();
    if (m_header == NULL) {
        return;
    }
    memcpy(m_header, data, sizeof(FontHeader));

    m_charDescs = NULL;
    if (m_header->m_numChars > 0) {
        m_charDescs = new CharDesc[m_header->m_numChars];
    }
    m_controlCharDescs = NULL;
    if (m_header->m_numControlChars > 0) {
        m_controlCharDescs = new ControlCharDesc[m_header->m_numControlChars];
    }

    // room for the character codes of the bigger table
    int codeCount = m_header->m_numControlChars;
    if (m_header->m_numChars > codeCount) {
        codeCount = m_header->m_numChars;
    }
    unsigned int codeCapacity = codeCount + 1;
    wchar_t* codes = new wchar_t[codeCapacity];
    if (codes == NULL) {
        return;
    }

    components::CArrayInputStream stream;
    stream.Open(data + sizeof(FontHeader), size - sizeof(FontHeader));
    if (m_charDescs != NULL) {
        if (m_header->m_format == BitmapFont_FormatJMUtfCodes) {
            stream.ReadJMUtf(codes, codeCapacity);
        }
        for (int index = 0; index < m_header->m_numChars; index++) {
            if (m_header->m_format == BitmapFont_FormatJMUtfCodes) {
                m_charDescs[index].m_code = codes[index];
            } else {
                m_charDescs[index].m_code = stream.ReadInt16();
            }
            m_charDescs[index].m_x = stream.ReadInt16();
            m_charDescs[index].m_y = stream.ReadInt16();
            m_charDescs[index].m_width = stream.ReadInt8();
            m_charDescs[index].m_height = stream.ReadInt8();
            m_charDescs[index].m_offsetX = stream.ReadInt8();
            m_charDescs[index].m_offsetY = stream.ReadInt8();
            m_charDescs[index].m_advance = stream.ReadInt8();
            m_charDescs[index].m_reserved = stream.ReadInt8();
        }
    }
    if (m_controlCharDescs != NULL) {
        if (m_header->m_format == BitmapFont_FormatJMUtfCodes) {
            stream.ReadJMUtf(codes, codeCapacity);
        }
        for (int index = 0; index < m_header->m_numControlChars; index++) {
            m_controlCharDescs[index].m_code = codes[index];
            if (m_header->m_format == BitmapFont_FormatJMUtfCodes) {
                m_controlCharDescs[index].m_code = codes[index];
            } else {
                m_controlCharDescs[index].m_code = stream.ReadInt16();
            }
            m_controlCharDescs[index].m_reserved = stream.ReadInt8();
            m_controlCharDescs[index].m_advance = stream.ReadInt8();
        }
    }
    stream.Close();
    if (codes != NULL) {
        delete[] codes;
    }
}

unsigned int CBitmapFont::GetType() const
{
    return Type_Bitmap;
}

int CBitmapFont::GetFontHeight() const
{
    return m_header->m_ascent + m_header->m_lineSpacing;
}

int CBitmapFont::GetMinLineSpacing() const
{
    return m_header->m_lineSpacing;
}

int CBitmapFont::GetMinCharSpacing() const
{
    return m_header->m_charSpacing;
}

int CBitmapFont::MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount)
{
    if (length < 0) {
        length = wcslen(text);
    }
    int width = 0;
    int count;
    for (count = 0; count < length; count++) {
        int charWidth = GetWidth(text[count]);
        // Compare the glyph and its advance after the original 32-bit addition.
        int advance = (int)((unsigned int)charWidth + m_header->m_charSpacing);
        if (maxWidth >= 0) {
            // the glyph has to fit, not only its advance
            int needed = advance;
            if (charWidth > advance) {
                needed = charWidth;
            }
            if (needed > maxWidth - width) {
                break;
            }
        }
        width += advance;
    }
    // no spacing after the last character
    if (count >= 1) {
        width -= m_header->m_charSpacing;
    }
    if (fitCount != NULL) {
        *fitCount = count;
    }
    return width;
}

void CBitmapFont::PaintText(const wchar_t* text, int length, int x, int y, int width, int height,
                            const components::Color_RGBA* color)
{
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    if (length < 0) {
        length = wcslen(text);
    }
    int penX = 0;
    for (; length > 0; length--) {
        wchar_t character = *text;
        const ControlCharDesc* control = GetControlCharDesc(character);
        if (control != NULL) {
            penX += control->m_advance + m_header->m_charSpacing;
        } else {
            const CharDesc* desc = GetCharDesc(character);
            if (desc != NULL) {
                // clip the glyph to the box
                int glyphWidth = desc->m_width;
                if (width >= 0) {
                    glyphWidth = width - penX - desc->m_offsetX;
                    if (desc->m_width < glyphWidth) {
                        glyphWidth = desc->m_width;
                    }
                    glyphWidth = (0 <= glyphWidth) ? glyphWidth : 0;
                }
                int glyphHeight = desc->m_height;
                if (height >= 0) {
                    glyphHeight = height - desc->m_offsetY;
                    if (desc->m_height < glyphHeight) {
                        glyphHeight = desc->m_height;
                    }
                    glyphHeight = (0 <= glyphHeight) ? glyphHeight : 0;
                }
                unsigned short sourceY = desc->m_y;
                unsigned short sourceX = desc->m_x;
                components::CRectangle<short> source = {
                    sourceX, sourceY, glyphWidth, glyphHeight
                };
                graphics->PushTransform();
                graphics->Translate((float)(x + penX + desc->m_offsetX), (float)(y + desc->m_offsetY));
                graphics->Draw(*m_image, &source, color, ICGraphics2d::Flip_None);
                graphics->PopTransform();
                penX += desc->m_advance + m_header->m_charSpacing;
            }
        }
        text++;
    }
}

const CBitmapFont::ControlCharDesc* CBitmapFont::GetControlCharDesc(wchar_t character) const
{
    if (m_header->m_numControlChars > 0) {
        const ControlCharDesc* desc = m_controlCharDescs;
        for (int index = 0; index < m_header->m_numControlChars; index++) {
            if (desc->m_code == character) {
                return desc;
            }
            desc++;
        }
    }
    return NULL;
}

const CBitmapFont::CharDesc* CBitmapFont::GetCharDesc(wchar_t character) const
{
    if (m_header->m_numChars > 0) {
        // binary search: the table is sorted by code
        int high = m_header->m_numChars - 1;
        int low = 0;
        while (low <= high) {
            int middle = (high + low) >> 1;
            if (m_charDescs[middle].m_code == character) {
                return &m_charDescs[middle];
            }
            if (m_charDescs[middle].m_code <= character) {
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        }
    }
    return NULL;
}

int CBitmapFont::GetWidth(wchar_t character) const
{
    const ControlCharDesc* control = GetControlCharDesc(character);
    if (control != NULL) {
        return control->m_advance;
    }
    const CharDesc* desc = GetCharDesc(character);
    int width = 0;
    if (desc != NULL) {
        width = desc->m_advance;
    }
    return width;
}

} } } }
