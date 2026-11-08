// CResourcePalette.cpp - resource wrapping a components::Color_Palette.
#include "CResourcePalette.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourcePalette::CResourcePalette()
{
    m_classId = ClassId;
    m_palette = NULL;
    CResource::ResetMembers();
}

CResourcePalette::~CResourcePalette()
{
    CResourcePalette::Destroy();
}

void CResourcePalette::Destroy()
{
    if (m_palette != NULL) {
        delete m_palette;
        m_palette = NULL;
    }
}

void CResourcePalette::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourcePalette::GetSize() const
{
    if (m_palette == NULL) {
        return 0;
    }
    return m_palette->m_count * sizeof(unsigned int) + sizeof(components::Color_Palette);
}

void* CResourcePalette::GetData() const
{
    return m_palette;
}

unsigned int CResourcePalette::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                           unsigned int mimeType, components::CIdToObjectRouter* router,
                                           unsigned char copyName)
{
    m_handle = handle;
    m_palette = new components::Color_Palette();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_palette->Load(*stream, mimeType) ? CreateResult_Loaded : CreateResult_Failed;
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
