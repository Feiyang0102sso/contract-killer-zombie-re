// ICShaderProgram.h - linked shader program interface.
// (first drafted by b011x as PROVISIONAL; slots completed by b037x from the vtables of
// CShaderProgram / CShaderProgram_OGLES. The factory lives in cocoa/src/CGraphicsAbstractionManager_Cocoa.cpp.)
#ifndef ICSHADERPROGRAM_H
#define ICSHADERPROGRAM_H

#include "ICGraphicsResource.h"
#include "CTypedVariableTable.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
class CIdToObjectRouter;
}

namespace graphics {

class ICShader;  // added by b040x

class ICShaderProgram : public ICGraphicsResource {
public:
    /** Resource type key of shader programs. */
    enum { ResourceType = 0xca2ce35f };

    /** The values of the program parameters (uniforms). */
    class ParameterTable : public components::CTypedVariableTable {
    public:
        /** Runtime class id. */
        static const unsigned int ClassId = 0x95b6fd22;

        ParameterTable() : components::CTypedVariableTable(ClassId) {}

        inline virtual ~ParameterTable() {}
    };

    /** One (key, value) initialization setting; a list ends with key 0. */
    struct InitParam {
        unsigned int m_key;     // 0x00
        unsigned int m_value;   // 0x04
    };

    /** A vertex attribute the program reads (id, layout and name). */  // added by b040x
    struct Attribute {  // added by b040x
        unsigned int m_id;              // 0x00 CVertex::Attribute::Id  // added by b040x
        unsigned int m_type;            // 0x04 ComponentType  // added by b040x
        unsigned int m_count;           // 0x08 components  // added by b040x
        components::CStrWChar m_name;   // 0x0c  // added by b040x

        Attribute()  // added by b040x
        {  // added by b040x
            m_id = 0xffffffff;  // added by b040x
        }  // added by b040x
    };  // added by b040x

    /** Description of a program (GetDescription). */
    struct Description {
        unsigned int m_type;    // 0x00
        unsigned int m_shaderCount;             // 0x04  // added by b040x
        ICShader** m_shaders;                   // 0x08  // added by b040x
        unsigned char m_finalized;              // 0x0c FinalizeInitialization ran  // added by b040x
        unsigned int m_attributeCount;          // 0x10  // added by b040x
        Attribute* m_attributes;                // 0x14  // added by b040x
        unsigned int m_parameterCount;          // 0x18  // added by b040x
        unsigned int* m_parameterTypes;         // 0x1c CTypedVariableTable::Entry::Type  // added by b040x
        int* m_parameterArraySizes;             // 0x20  // added by b040x
        components::CStrWChar* m_parameterNames;    // 0x24  // added by b040x
    };

    /** Platform factory. */
    static ICShaderProgram* CreateInstance();

    ICShaderProgram() : ICGraphicsResource(ResourceType) {}

    virtual ~ICShaderProgram() {}

    /** Load the program description of mime type `type`; shaders resolve through `router`. */
    virtual unsigned char Load(components::CInputStream& stream, unsigned int type,
                               components::CIdToObjectRouter* router) = 0;
    virtual unsigned char Initialize(const InitParam* params) = 0;
    virtual void SetParameterTable(ParameterTable& table) = 0;
    virtual ParameterTable* GetParameterTable() = 0;
    virtual unsigned char GetDescription(Description& description) = 0;
    virtual void Destroy() = 0;
    // changed by b040x: returns void (CShaderProgram_OGLES's override ends with a call of the void ParseCommon)
    virtual void FinalizeInitialization(components::CInputStream** streams, unsigned int* types) = 0;
    virtual unsigned char SaveStateForFutureRecreation() = 0;
    virtual unsigned char Recreate() = 0;
};

} } } }

#endif
