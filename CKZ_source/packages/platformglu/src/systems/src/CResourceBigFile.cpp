// CResourceBigFile.cpp - resource wrapping a CBigFile (version 2 archive).
#include "CResourceBigFile.h"
#include "ICResourceManager.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceBigFile::CResourceBigFile()
{
    m_classId = ClassId;
    m_bigFile = NULL;
    CResource::ResetMembers();
}

CResourceBigFile::~CResourceBigFile()
{
    CResourceBigFile::Destroy();
}

void CResourceBigFile::Destroy()
{
    if (m_manager != NULL) {
        m_manager->RemoveResource(*this);
    }
    if (m_bigFile != NULL) {
        delete m_bigFile;
        m_bigFile = NULL;
    }
}

void CResourceBigFile::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceBigFile::GetSize() const
{
    return 0;
}

void* CResourceBigFile::GetData() const
{
    return m_bigFile;
}

unsigned int CResourceBigFile::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                           unsigned int mimeType, components::CIdToObjectRouter* router,
                                           unsigned char copyName)
{
    m_handle = handle;
    m_bigFile = new components::CBigFile_v2();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_bigFile->Load(*stream, mimeType) ? CreateResult_Loaded : CreateResult_Failed;
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
