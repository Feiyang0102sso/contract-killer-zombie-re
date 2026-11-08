// ICShader.h - shader interface.
// (first drafted by b011x as PROVISIONAL; slots completed by b037x from the vtables of CShader /
// CShader_OGLES. The factory lives in cocoa/src/CGraphicsAbstractionManager_Cocoa.cpp.)
#ifndef ICSHADER_H
#define ICSHADER_H

#include "ICGraphicsResource.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
class COutputStream;
class CStrWChar;
}

namespace graphics {

class ICShader : public ICGraphicsResource {
public:
    /** Resource type key of shaders. */
    enum { ResourceType = 0xd65e722e };

    /** Pipeline stage of a shader. */
    enum PipelineType {
        PipelineType_Undefined = 0
    };

    /** Platform factory. */
    static ICShader* CreateInstance();

    ICShader() : ICGraphicsResource(ResourceType) {}

    virtual ~ICShader() {}

    /** Load the shader source / binary of mime type `type` from `stream`. */
    virtual unsigned char Load(components::CInputStream& stream, unsigned int type) = 0;
    virtual unsigned char Save(components::COutputStream& stream, unsigned int type) = 0;
    virtual void Destroy() = 0;
    virtual void SetPipelineType(PipelineType type) = 0;
    virtual PipelineType GetPipelineType() const = 0;
    virtual unsigned char SetSource(components::CInputStream& stream, unsigned int type) = 0;
    virtual unsigned char Compile() = 0;
    virtual unsigned char Compile(components::CStrWChar& log) = 0;
    virtual void DiscardSource() = 0;
    virtual unsigned char SaveStateForFutureRecreation() = 0;
    virtual unsigned char Recreate() = 0;
};

} } } }

#endif
