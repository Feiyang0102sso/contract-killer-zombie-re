// CResourceShader.cpp - resource wrapping a ICShader.
#include "CResourceShader.h"
#include "ICShader.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceShader::CResourceShader()
{
    m_classId = ClassId;
    m_shader = NULL;
    CResource::ResetMembers();
}

CResourceShader::~CResourceShader()
{
    CResourceShader::Destroy();
}

void CResourceShader::Destroy()
{
    if (m_shader != NULL) {
        delete m_shader;
        m_shader = NULL;
    }
}

void CResourceShader::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceShader::GetSize() const
{
    if (m_shader == NULL) {
        return 0;
    }
    return 8;
}

void* CResourceShader::GetData() const
{
    return m_shader;
}

unsigned int CResourceShader::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                          unsigned int mimeType, components::CIdToObjectRouter* router,
                                          unsigned char copyName)
{
    m_handle = handle;
    unsigned int result = CreateResult_Failed;
    unsigned char success = 0;
    if (mimeType == 0xd65e722e || mimeType == 0x2c435e5a || mimeType == 0x2c435e5c) {
        m_shader = graphics::ICShader::CreateInstance();
        success = 1;
        result = CreateResult_Failed;
        if (m_shader != NULL) {
            result = CreateResult_Created;
            if (stream != NULL) {
                // ?: kept on purpose: the original selects the result instead of branching
                result = m_shader->Load(*stream, mimeType) ? CreateResult_Loaded : CreateResult_Failed;
                success = result != CreateResult_Failed;
            }
        }
    }
    if (success) {
        SetName(name, copyName);
        m_isCreated = 1;
    } else {
        // unknown shader type or load failure
        Destroy();
        result = CreateResult_Failed;
    }
    return result;
}

} } } }
