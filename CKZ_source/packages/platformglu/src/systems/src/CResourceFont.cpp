// CResourceFont.cpp - resource wrapping a ICFont.
#include "CResourceFont.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceFont::CResourceFont()
{
    m_classId = ClassId;
    m_font = NULL;
    CResource::ResetMembers();
}

CResourceFont::~CResourceFont()
{
    CResourceFont::Destroy();
}

void CResourceFont::Destroy()
{
    if (m_font != NULL) {
        delete m_font;
        m_font = NULL;
    }
}

void CResourceFont::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceFont::GetSize() const
{
    if (m_font == NULL) {
        return 0;
    }
    return 8;
}

void* CResourceFont::GetData() const
{
    return m_font;
}

unsigned int CResourceFont::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName)
{
    m_handle = handle;
    if (mimeType == 0x69e5abca || mimeType == 0x50055753) {
        m_font = graphics::ICFont::CreateInstance(graphics::ICFont::Type_Bitmap);
    } else if (mimeType == 0xc5d54755) {
        m_font = graphics::ICFont::CreateInstance(graphics::ICFont::Type_System);
    }
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_font->Load(*stream, mimeType, router) ? CreateResult_Loaded : CreateResult_Failed;
    }
    if (result != CreateResult_Failed) {
        SetName(name, copyName);
        m_isCreated = 1;
    } else {
        Destroy();
        result = CreateResult_Failed;
    }
    return result;
}

} } } }
