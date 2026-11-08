// CResourceStrWChar.cpp - resource wrapping a components::CStrWChar.
#include "CResourceStrWChar.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceStrWChar::CResourceStrWChar()
{
    m_classId = ClassId;
    m_string = NULL;
    CResource::ResetMembers();
}

CResourceStrWChar::~CResourceStrWChar()
{
    CResourceStrWChar::Destroy();
}

void CResourceStrWChar::Destroy()
{
    if (m_string != NULL) {
        delete m_string;
        m_string = NULL;
    }
}

void CResourceStrWChar::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceStrWChar::GetSize() const
{
    if (m_string == NULL) {
        return 0;
    }
    return m_string->m_length * sizeof(wchar_t) + sizeof(unsigned int);
}

void* CResourceStrWChar::GetData() const
{
    return m_string;
}

unsigned int CResourceStrWChar::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                            unsigned int mimeType, components::CIdToObjectRouter* router,
                                            unsigned char copyName)
{
    m_handle = handle;
    m_string = new components::CStrWChar();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_string->Load(*stream, mimeType) ? CreateResult_Loaded : CreateResult_Failed;
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
