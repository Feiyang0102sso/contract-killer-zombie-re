// CResourceDIB.cpp - resource wrapping a graphics::CDIB.
#include "CResourceDIB.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceDIB::CResourceDIB()
{
    m_classId = ClassId;
    m_dib = NULL;
    CResource::ResetMembers();
}

CResourceDIB::~CResourceDIB()
{
    CResourceDIB::Destroy();
}

void CResourceDIB::Destroy()
{
    if (m_dib != NULL) {
        delete m_dib;
        m_dib = NULL;
    }
}

void CResourceDIB::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceDIB::GetSize() const
{
    if (m_dib == NULL) {
        return 0;
    }
    return sizeof(graphics::CDIB);
}

void* CResourceDIB::GetData() const
{
    return m_dib;
}

unsigned int CResourceDIB::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                       unsigned int mimeType, components::CIdToObjectRouter* router,
                                       unsigned char copyName)
{
    m_handle = handle;
    m_dib = new graphics::CDIB();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_dib->Load(*stream, mimeType) ? CreateResult_Loaded : CreateResult_Failed;
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
