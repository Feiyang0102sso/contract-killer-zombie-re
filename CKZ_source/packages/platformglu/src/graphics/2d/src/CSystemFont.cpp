// CSystemFont.cpp - font drawn by the platform: the text is rendered into a surface by a 2d text
// renderer and the surface is drawn.
#include <string.h>
#include <wchar.h>
#include "CSystemFont.h"
#include "ICRenderText2d.h"
#include "ICGraphics2d.h"
#include "CInputStream.h"
#include "CArrayInputStream.h"
#include "CIdToObjectRouter.h"
#include "CSharedPtr.h"
#include "CBinary.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// ---- constants ----
// Load() tags: what follows (a resource key or a NUL terminated name) and the InitParam it fills
static const unsigned int SystemFont_TagImageById = 0xa6601847;
static const unsigned int SystemFont_TagImageByName = 0x1cfda15a;
static const unsigned int SystemFont_TagImageStreamById = 0xa0ca9e08;
static const unsigned int SystemFont_TagImageStreamByName = 0x31c237d3;
static const unsigned int SystemFont_TagDataById = 0x2b66d8a6;
static const unsigned int SystemFont_TagDataByName = 0x01cf0337;
static const unsigned int SystemFont_TagDataStreamById = 0x7d4bae73;
static const unsigned int SystemFont_TagDataStreamByName = 0xc45a54ee;

/** InitParam key filled by a Load() tag. */
static inline unsigned int SystemFont_GetInitParamKey(unsigned int tag)
{
    unsigned int key = ICFont::InitParam_End;
    switch (tag) {
    case SystemFont_TagImageById:
    case SystemFont_TagImageByName:
        key = ICFont::InitParam_Image;
        break;
    case SystemFont_TagImageStreamById:
    case SystemFont_TagImageStreamByName:
        key = ICFont::InitParam_ImageStream;
        break;
    case SystemFont_TagDataById:
    case SystemFont_TagDataByName:
        key = ICFont::InitParam_Data;
        break;
    case SystemFont_TagDataStreamById:
    case SystemFont_TagDataStreamByName:
        key = ICFont::InitParam_DataStream;
        break;
    }
    return key;
}

/** Read a NUL terminated name into `name`. */
static inline void SystemFont_ReadName(components::CInputStream& stream, char* name)
{
    unsigned char character;
    do {
        character = stream.ReadUInt8();
        *name = character;
        name++;
    } while (character != 0);
}

void CSystemFont::Text2dCache::Update(const wchar_t* text, int length, int width, int height)
{
    if (m_font == NULL || text == NULL) {
        return;
    }
    // same text in the same box: nothing to do
    if (m_renderText != NULL && m_length == length && m_width == width && m_height == height &&
        m_text.GetWCharPtr() != NULL && wcscmp(m_text.GetWCharPtr(), text) == 0) {
        return;
    }
    m_text = text;
    m_length = length;
    m_width = width;
    m_height = height;
    if (m_renderText == NULL) {
        m_renderText = ICRenderText2d::CreateInstance();
    }

    components::CArrayInputStream stream;
    stream.Open((const unsigned char*)text, (wcslen(text) + 1) * sizeof(wchar_t));
    ICRenderText2d::InitParam params[] = {
        { ICRenderText2d::InitParam_Font, (unsigned int)m_font },
        { ICRenderText2d::InitParam_Stream, (unsigned int)&stream },
        { ICRenderText2d::InitParam_StreamType, ICRenderText2d::MimeType_WChar },
        { ICRenderText2d::InitParam_End, 0 }
    };
    if (!m_renderText->Initialize(params)) {
        m_text = "";
        m_length = 0;
        m_width = 0;
        m_height = 0;
    }
}

void CSystemFont::Text2dCache::Destroy()
{
    if (m_renderText != NULL) {
        delete m_renderText;
        m_renderText = NULL;
    }
    m_text = "";
    m_length = 0;
    m_width = 0;
    m_height = 0;
    m_font = NULL;
}

CSystemFont::CSystemFont()
{
    m_data = NULL;
    m_header = NULL;
    m_antialias = 0;
    m_pointSize = 0;
}

CSystemFont::~CSystemFont()
{
    Destroy();
}

void CSystemFont::Destroy()
{
    m_textCache.Destroy();
    if (m_header != NULL) {
        delete m_header;
        m_header = NULL;
    }
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    m_pointSize = 0;
    m_antialias = 0;
}

unsigned char CSystemFont::Load(components::CInputStream& stream, unsigned int type,
                                components::CIdToObjectRouter* router)
{
    // (declared in this order: the original stack layout)
    InitParam params[5];
    char name[256];
    components::CInputStream streams[2];            // [0] image (unused), [1] font description
    unsigned int pointerTypes[2] = { 0, 0 };
    unsigned int streamTypes[2] = { 0, 0 };
    core::CSharedPtr<void*> data;                   // routed font description object
    memset(params, 0, sizeof(params));

    unsigned char ok = 1;
    int count = 0;
    while (stream.Available() != 0) {
        unsigned int tag = stream.ReadUInt32();
        switch (tag) {
        case SystemFont_TagImageById:
        case SystemFont_TagImageByName:
        case SystemFont_TagImageStreamById:
        case SystemFont_TagImageStreamByName:
            // a system font has no glyph image
            break;
        case SystemFont_TagDataById:
            ok = router->KeyToPointer(stream.ReadUInt32(), data, pointerTypes[1]);
            if (ok) {
                params[count].m_key = InitParam_Data;
                params[count].m_value = (unsigned int)*data;
            }
            break;
        case SystemFont_TagDataByName:
            SystemFont_ReadName(stream, name);
            ok = router->NameToPointer(name, data, pointerTypes[1]);
            if (ok) {
                params[count].m_key = SystemFont_GetInitParamKey(tag);
                params[count].m_value = (unsigned int)*data;
            }
            break;
        case SystemFont_TagDataStreamById:
            ok = router->KeyToInputStream(stream.ReadUInt32(), streams[1], streamTypes[1]);
            if (ok) {
                params[count].m_key = InitParam_DataStream;
                params[count].m_value = (unsigned int)&streams[1];
            }
            break;
        case SystemFont_TagDataStreamByName:
            SystemFont_ReadName(stream, name);
            ok = router->NameToInputStream(name, streams[1], streamTypes[1]);
            if (ok) {
                params[count].m_key = SystemFont_GetInitParamKey(tag);
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

unsigned char CSystemFont::Initialize(const InitParam* params)
{
    components::CBinary* binary = NULL;
    unsigned int binaryType = 0;
    components::CInputStream* stream = NULL;
    unsigned int streamType = 0;
    unsigned char failed = 0;
    const InitParam* param = params;
    while (param->m_key != InitParam_End && !failed) {
        switch (param->m_key) {
        case InitParam_Image:
        case InitParam_ImageType:
        case InitParam_ImageStream:
        case InitParam_ImageStreamType:
            // a system font has no glyph image
            failed = 1;
            break;
        case InitParam_Data:
            binary = (components::CBinary*)param->m_value;
            break;
        case InitParam_DataType:
            binaryType = param->m_value;
            break;
        case InitParam_DataStream:
            stream = (components::CInputStream*)param->m_value;
            break;
        case InitParam_DataStreamType:
            streamType = param->m_value;
            break;
        }
        param++;
    }

    // exactly one description source, of the right type
    if (binary != NULL && stream != NULL) {
        failed = 1;
    }
    if (stream != NULL && streamType != MimeType_FontData) {
        failed = 1;
    }
    const unsigned char* data = NULL;
    unsigned int size = 0;
    if (binary != NULL) {
        if (binaryType == components::CBinary::ClassId) {
            data = binary->m_data;
            size = binary->m_size;
        } else {
            failed = 1;
        }
    }
    if (stream != NULL) {
        size = stream->Available();
        m_data = new unsigned char[size];
        stream->Read(m_data, size);
        data = m_data;
        if (stream->m_error) {
            failed = 1;
        }
    }

    if (!failed && FinalizeInitialization(data, size)) {
        if (m_data != NULL) {
            delete[] m_data;
            m_data = NULL;
        }
        m_textCache.m_font = this;
        return 1;
    }
    Destroy();
    return 0;
}

unsigned int CSystemFont::GetType() const
{
    return Type_System;
}

void CSystemFont::PaintText(const wchar_t* text, int length, int x, int y, int width, int height,
                            const components::Color_RGBA* color)
{
    m_textCache.Update(text, length, width, height);
    if (m_textCache.m_renderText != NULL) {
        ICGraphics2d* graphics = ICGraphics2d::GetInstance();
        graphics->PushTransform();
        graphics->Translate((float)x, (float)y);
        graphics->Draw(*m_textCache.m_renderText, color);
        graphics->PopTransform();
    }
}

unsigned char CSystemFont::Parse(const unsigned char* data, unsigned int size)
{
    m_header = new FontHeader();
    if (m_header == NULL) {
        return 0;
    }
    memcpy(m_header, data, sizeof(FontHeader));
    // antialias flag, point size, then the NUL terminated font name
    components::CArrayInputStream stream;
    stream.Open(data + sizeof(FontHeader), size - sizeof(FontHeader));
    m_antialias = stream.ReadInt8();
    m_pointSize = stream.ReadInt8();
    m_fontName = (const char*)(data + 6);
    return 1;
}

} } } }
