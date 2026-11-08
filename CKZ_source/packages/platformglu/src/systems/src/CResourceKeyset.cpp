// CResourceKeyset.cpp - resource wrapping a components::CKeysetResource.
#include "CResourceKeyset.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceKeyset::CResourceKeyset()
{
    m_classId = ClassId;
    m_keyset = NULL;
    CResource::ResetMembers();
}

CResourceKeyset::~CResourceKeyset()
{
    CResourceKeyset::Destroy();
}

void CResourceKeyset::Destroy()
{
    if (m_keyset != NULL) {
        delete m_keyset;
        m_keyset = NULL;
    }
}

void CResourceKeyset::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceKeyset::GetSize() const
{
    if (m_keyset == NULL) {
        return 0;
    }
    return sizeof(components::CKeysetResource);
}

void* CResourceKeyset::GetData() const
{
    return m_keyset;
}

unsigned int CResourceKeyset::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                          unsigned int mimeType, components::CIdToObjectRouter* router,
                                          unsigned char copyName)
{
    m_handle = handle;
    m_keyset = new components::CKeysetResource();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_keyset->Load(*stream, mimeType, router) ? CreateResult_Loaded : CreateResult_Failed;
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
