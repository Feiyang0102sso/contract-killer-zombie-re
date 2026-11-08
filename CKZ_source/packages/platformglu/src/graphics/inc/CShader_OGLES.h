// CShader_OGLES.h - shader of the OpenGL ES 1.x device. Fixed function OpenGL ES has no shaders:
// compiling only records that the shader was "compiled", the source is ignored.
// (first drafted by b002 as PROVISIONAL; completed by b040x, the owner of CShader_OGLES.cpp)
#ifndef CSHADER_OGLES_H
#define CSHADER_OGLES_H

#include "CShader.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CShader_OGLES : public CShader {
public:
    CShader_OGLES();

    virtual ~CShader_OGLES()
    {
    }

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const;
    virtual unsigned char Save(components::COutputStream& stream, unsigned int type);
    virtual void Destroy();
    virtual unsigned char Compile();
    virtual unsigned char Compile(components::CStrWChar& log);

    unsigned int m_compiled;        // 0x14 set by Compile (no GL object exists)
};

} } } }

#endif
