// CShaderProgram.h - platform independent part of a shader program: collects the shaders, the
// vertex attributes and the parameter (uniform) declarations from an InitParam list.
// (first drafted by b002 as PROVISIONAL; completed by b040x, the owner of CShaderProgram.cpp)
#ifndef CSHADERPROGRAM_H
#define CSHADERPROGRAM_H

#include "ICShaderProgram.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICShader;

class CShaderProgram : public ICShaderProgram {
public:
    /** InitParam keys understood by Initialize. */
    enum ParamType {
        ParamType_End               = 0,
        ParamType_Shader            = 1,    // ICShader*
        ParamType_ShaderType        = 2,    // mime type of the matching ParamType_Shader
        ParamType_ShaderSource      = 3,    // CInputStream* with shader source
        ParamType_ShaderSourceType  = 4,    // mime type of the matching ParamType_ShaderSource
        ParamType_Flag5             = 5,    // boolean kept in m_flag34 (meaning unknown)
        ParamType_AttributeId       = 6,    // CVertex::Attribute::Id of the next attribute
        ParamType_AttributeName     = 7,    // name of the next attribute (encoded per ParamType_AttributeNameType)
        ParamType_AttributeNameType = 8     // encoding of the matching ParamType_AttributeName
    };

    /** Attributes reserved by Initialize. */
    enum { MaxAttributes = 8 };

    CShaderProgram();
    virtual ~CShaderProgram();

    virtual unsigned char Load(components::CInputStream& stream, unsigned int type,
                               components::CIdToObjectRouter* router);
    virtual unsigned char Initialize(const InitParam* params);
    virtual void SetParameterTable(ParameterTable& table);
    virtual ParameterTable* GetParameterTable();
    virtual unsigned char GetDescription(Description& description);
    virtual void Destroy();
    virtual unsigned char SaveStateForFutureRecreation();
    virtual unsigned char Recreate();
    /** Bring the platform program up to date with the current context. */
    // b118: in the original this class ends at Recreate; the new UpdateContext virtual slot is introduced by CShaderProgram_OGLES.

    /**
     * Collect the parameter declarations ("name type count" separated by blanks or ':') from the
     * text of the first m_sourceShaderCount shader sources.
     */
    void ParseCommon(components::CInputStream** streams, unsigned int* types);

    unsigned int m_flags;                       // 0x08 bit 31: initialized
    ParameterTable* m_parameterTable;           // 0x0c
    unsigned int m_parameterCount;              // 0x10
    unsigned int* m_parameterTypes;             // 0x14 CTypedVariableTable::Entry::Type, 0 terminated
    int* m_parameterArraySizes;                 // 0x18
    components::CStrWChar* m_parameterNames;    // 0x1c
    int* m_parameterEntries;                    // 0x20 table entry of each parameter (0x7fffffff: none)
    unsigned int m_shaderCount;                 // 0x24
    unsigned int m_sourceShaderCount;           // 0x28 shaders given as source (the first slots of m_shaders)
    ICShader** m_shaders;                       // 0x2c
    unsigned char* m_buffer30;                  // 0x30 only freed here (not set by this module)
    unsigned char m_flag34;                     // 0x34 ParamType_Flag5
    unsigned char m_finalized;                  // 0x35 FinalizeInitialization ran
    unsigned int m_attributeCount;              // 0x38
    Attribute* m_attributes;                    // 0x3c MaxAttributes entries
    unsigned int m_count40;                     // 0x40 only cleared here
    unsigned char* m_buffer44;                  // 0x44 only freed here
};

} } } }

#endif
