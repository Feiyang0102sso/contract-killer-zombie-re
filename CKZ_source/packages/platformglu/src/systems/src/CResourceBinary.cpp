// CResourceBinary.cpp - resource wrapping raw binary data.
#include "CResourceBinary.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceBinary::CResourceBinary()
{
    m_classId = ClassId;
    m_binary = NULL;
    CResource::ResetMembers();
}

CResourceBinary::~CResourceBinary()
{
    CResourceBinary::Destroy();
}

void CResourceBinary::Destroy()
{
    if (m_binary != NULL) {
        delete m_binary;
        m_binary = NULL;
    }
}

void CResourceBinary::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceBinary::GetSize() const
{
    if (m_binary == NULL) {
        return 0;
    }
    return m_binary->m_size + sizeof(components::CBinary);
}

void* CResourceBinary::GetData() const
{
    return m_binary;
}

unsigned int CResourceBinary::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                             unsigned int mimeType, components::CIdToObjectRouter* router,
                                             unsigned char copyName)
{
    m_handle = handle;
    m_binary = new components::CBinary();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_binary->Load(*stream, mimeType) ? CreateResult_Loaded : CreateResult_Failed;
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
