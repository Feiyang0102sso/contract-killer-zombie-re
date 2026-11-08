// CShaderProgram_OGLES.h - shader program of the OpenGL ES 1.x device: the parameters are emulated
// with fixed function state, each one is mapped to a ParameterSemantic by its name.
// (first drafted by b002 as PROVISIONAL; completed by b040x, the owner of CShaderProgram_OGLES.cpp)
#ifndef CSHADERPROGRAM_OGLES_H
#define CSHADERPROGRAM_OGLES_H

#include "CShaderProgram.h"
#include "ICGraphicsAbstractionManager.h"

namespace com { namespace glu { namespace platform { namespace graphics {

/** Sync handler installed in the parameter table: re-resolves the parameters after a table rebuild. */
unsigned char CShaderProgram_OGLES_ParameterTableSyncCallback(void* context, components::CTypedVariableTable* table);

class CShaderProgram_OGLES : public CShaderProgram {
public:
    /** Fixed function meaning of a parameter, by name. */
    enum ParameterSemantic {
        ParameterSemantic_None = 0,
        ParameterSemantic_AlphaTest = 1,        // "alphaTest"
        ParameterSemantic_AlphaRefVal = 2,      // "alphaRefVal"
        ParameterSemantic_ConstColor = 3,       // "constColor"
        ParameterSemantic_Mvp = 4,              // "mvp"
        ParameterSemantic_TexEnv0 = 5,          // "texEnv0"
        ParameterSemantic_TexEnv1 = 6           // "texEnv1"
    };

    CShaderProgram_OGLES()
    {
        m_parameterSemantics = NULL;
    }

    virtual ~CShaderProgram_OGLES();

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const
    {
        return ICGraphicsAbstract::Abstraction_Hardware;
    }

    virtual void SetParameterTable(ParameterTable& table);
    virtual void Destroy();
    virtual void FinalizeInitialization(components::CInputStream** streams, unsigned int* types);

    /** Nothing to bring up to date for OpenGL ES programs. */
    virtual unsigned char UpdateContext()
    {
        return 1;
    }

    unsigned int* m_parameterSemantics;     // 0x48 ParameterSemantic of each parameter
};

} } } }

#endif
