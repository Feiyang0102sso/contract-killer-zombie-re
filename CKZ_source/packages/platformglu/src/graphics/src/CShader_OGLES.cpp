// CShader_OGLES.cpp - shader of the OpenGL ES 1.x device (fixed function: the source is ignored).
#include "CShader_OGLES.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform { namespace graphics {

CShader_OGLES::CShader_OGLES()
{
    m_compiled = 0;
}

unsigned char CShader_OGLES::Save(components::COutputStream& stream, unsigned int type)
{
    return 0;
}

void CShader_OGLES::Destroy()
{
    m_compiled = 0;
    CShader::Destroy();
}

unsigned char CShader_OGLES::Compile()
{
    m_compiled = 1;
    m_needsCompile = 0;
    return 1;
}

unsigned char CShader_OGLES::Compile(components::CStrWChar& log)
{
    if (Compile()) {
        log = "Success. Currently, fixed function OpenGL ES ignores source.";
        return 1;
    }
    log = "Failure. Currently, fixed function OpenGL ES ignores source.";
    return 0;
}

ICGraphicsAbstract::Abstraction CShader_OGLES::GetAbstraction() const
{
    return ICGraphicsAbstract::Abstraction_Hardware;
}

} } } }
