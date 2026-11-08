// CShader.h - platform independent part of a shader: keeps the (text) source until it is compiled.
// (created by b039x, the owner; CShader_OGLES derives from it)
#ifndef CSHADER_H
#define CSHADER_H

#include "ICShader.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CShader : public ICShader {
public:
    /** Mime type key passed to SetSource by Load (raw binary). */
    enum { SourceType_Binary = 0xf4e02223 };

    CShader();
    virtual ~CShader();

    virtual unsigned char Load(components::CInputStream& stream, unsigned int type);
    virtual void Destroy();
    virtual void SetPipelineType(PipelineType type);
    virtual PipelineType GetPipelineType() const;
    /** Replace the source by the remaining bytes of `stream` (zero terminated copy). */
    virtual unsigned char SetSource(components::CInputStream& stream, unsigned int type);
    virtual void DiscardSource();
    virtual unsigned char SaveStateForFutureRecreation();
    virtual unsigned char Recreate();

    PipelineType m_pipelineType;    // 0x08
    char* m_source;                 // 0x0c np_malloc'ed, zero terminated
    unsigned short m_sourceLength;  // 0x10 without the terminator
    unsigned char m_needsCompile;   // 0x12 source changed since the last Compile
};

} } } }

#endif
