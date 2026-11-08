// CResourceShaderProgram.cpp - resource wrapping a ICShaderProgram.
#include "CResourceShaderProgram.h"
#include "ICShaderProgram.h"
#include "ICGraphics.h"
#include "ICGraphicsAbstractionManager.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceShaderProgram::CResourceShaderProgram()
{
    m_classId = ClassId;
    m_program = NULL;
    CResource::ResetMembers();
}

CResourceShaderProgram::~CResourceShaderProgram()
{
    CResourceShaderProgram::Destroy();
}

void CResourceShaderProgram::Destroy()
{
    if (m_program != NULL) {
        delete m_program;
        m_program = NULL;
    }
}

void CResourceShaderProgram::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceShaderProgram::GetSize() const
{
    if (m_program == NULL) {
        return 0;
    }
    return 8;
}

void* CResourceShaderProgram::GetData() const
{
    return m_program;
}

unsigned int CResourceShaderProgram::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                                 unsigned int mimeType, components::CIdToObjectRouter* router,
                                                 unsigned char copyName)
{
    m_handle = handle;
    unsigned int result = CreateResult_Failed;
    unsigned char success = 0;
    unsigned char known = 1;
    graphics::ICGraphicsAbstract::Abstraction abstraction = graphics::ICGraphicsAbstract::Abstraction_2;
    switch (mimeType) {
    case 0xb52802e0:
        abstraction = graphics::ICGraphicsAbstract::Abstraction_2;
        break;
    case 0xb52802fb:
        abstraction = graphics::ICGraphicsAbstract::Abstraction_1;
        break;
    case 0xf1854bbc:
        // generic program: use the abstraction of the running renderer
        switch ((int)graphics::ICGraphics::GetInstance()->GetAbstraction()) {
        case 2:
            abstraction = graphics::ICGraphicsAbstract::Abstraction_2;
            break;
        case 1:
            abstraction = graphics::ICGraphicsAbstract::Abstraction_1;
            break;
        default:
            known = 0;
            break;
        }
        break;
    default:
        known = 0;
        break;
    }
    if (known) {
        graphics::ICGraphicsAbstractionManager::SetAbstractionLayer(
            *graphics::ICGraphicsAbstractionManager::GetInstance(), abstraction);
        m_program = graphics::ICShaderProgram::CreateInstance();
        success = 1;
        result = CreateResult_Failed;
        if (m_program != NULL) {
            result = CreateResult_Created;
            if (stream != NULL) {
                // ?: kept on purpose: the original selects the result instead of branching
                result = m_program->Load(*stream, mimeType, router) ? CreateResult_Loaded : CreateResult_Failed;
                success = result != CreateResult_Failed;
            }
        }
    }
    if (success) {
        SetName(name, copyName);
        m_isCreated = 1;
    } else {
        // unknown program type or load failure
        Destroy();
        result = CreateResult_Failed;
    }
    return result;
}

} } } }
