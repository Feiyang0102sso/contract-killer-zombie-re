// CShaderProgram.cpp - platform independent part of a shader program.
#include "CShaderProgram.h"
#include "ICShader.h"
#include "CInputStream.h"
#include "CIdToObjectRouter.h"
#include "CSharedPtr.h"
#include "CStrChar.h"
#include "CTypedVariableTable.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// Keys of the entries of a serialized program (Load); each is followed by its value.
enum ProgramKey {
    ProgramKey_ShaderId             = 0xf47e84d8,   // uint32 resource id of a shader object
    ProgramKey_ShaderName           = 0x65762d03,   // zero terminated name of a shader object
    ProgramKey_ShaderSourceId       = 0x1f483106,   // uint32 resource id of a shader source stream
    ProgramKey_ShaderSourceName     = 0xa5d5881b,   // zero terminated name of a shader source stream
    ProgramKey_Flag5                = 0xa5f1bea9,   // ParamType_Flag5
    ProgramKey_AttributeId          = 0x6a70e687,   // ParamType_AttributeId
    ProgramKey_AttributeName        = 0xa22514c3,   // ParamType_AttributeName
    ProgramKey_AttributeName2       = 0x0bfe8507    // ParamType_AttributeName
};

// Encodings of an attribute name (ParamType_AttributeNameType).
enum NameType {
    NameType_WideUtf8       = 0xf686aadc,   // const wchar_t*
    NameType_WideString     = 0x00215e62,   // const wchar_t*
    NameType_CharString     = 0x00025e62,   // const char*
    NameType_CStrWChar      = 0x43735eb4,   // const CStrWChar*
    NameType_CStrChar       = 0x64365e6e    // const CStrChar*
};

// Shader source mime types ParseCommon skips (binary shaders).
enum {
    SourceType_Binary1 = 0xcc4fcbf6,
    SourceType_Binary2 = 0x2c435e5c
};

// Sizes of the scratch tables.
enum {
    MaxLoadedParams = 50,       // InitParam entries Load can build
    MaxLoadedStreams = 10,      // shader sources / shader objects Load can resolve
    MaxInitStreams = 25,        // entries of each Initialize scratch table
    MaxDeclarations = 30,       // parameter declarations ParseCommon can find
    NameBufferSize = 256
};

/** The InitParam key a serialized program key turns into (0 for unknown keys). */
static inline unsigned int GetParamTypeOfKey(unsigned int key)
{
    switch (key) {
    case ProgramKey_ShaderId:
    case ProgramKey_ShaderName:
        return CShaderProgram::ParamType_Shader;
    case ProgramKey_ShaderSourceId:
    case ProgramKey_ShaderSourceName:
        return CShaderProgram::ParamType_ShaderSource;
    case ProgramKey_Flag5:
        return CShaderProgram::ParamType_Flag5;
    case ProgramKey_AttributeId:
        return CShaderProgram::ParamType_AttributeId;
    case ProgramKey_AttributeName:
    case ProgramKey_AttributeName2:
        return CShaderProgram::ParamType_AttributeName;
    }
    return CShaderProgram::ParamType_End;
}

CShaderProgram::CShaderProgram()
{
    m_attributeCount = 0;
    m_attributes = NULL;
    m_count40 = 0;
    m_buffer44 = NULL;
    m_flags = 0;
    m_parameterTable = NULL;
    m_parameterCount = 0;
    m_parameterTypes = NULL;
    m_parameterArraySizes = NULL;
    m_parameterNames = NULL;
    m_parameterEntries = NULL;
    m_shaderCount = 0;
    m_sourceShaderCount = 0;
    m_shaders = NULL;
    m_buffer30 = NULL;
    m_flag34 = 0;
    m_finalized = 0;
}

CShaderProgram::~CShaderProgram()
{
    CShaderProgram::Destroy();
}

void CShaderProgram::Destroy()
{
    if (m_buffer44 != NULL) {
        delete[] m_buffer44;
        m_buffer44 = NULL;
    }
    m_count40 = 0;
    if (m_attributes != NULL) {
        delete[] m_attributes;
        m_attributes = NULL;
    }
    m_attributeCount = 0;
    if (m_parameterNames != NULL) {
        delete[] m_parameterNames;
        m_parameterNames = NULL;
    }
    if (m_parameterArraySizes != NULL) {
        delete[] m_parameterArraySizes;
        m_parameterArraySizes = NULL;
    }
    if (m_parameterTypes != NULL) {
        delete[] m_parameterTypes;
        m_parameterTypes = NULL;
    }
    if (m_parameterEntries != NULL) {
        delete[] m_parameterEntries;
        m_parameterEntries = NULL;
    }
    if (m_buffer30 != NULL) {
        delete[] m_buffer30;
        m_buffer30 = NULL;
    }
    if (m_shaders != NULL) {
        for (unsigned int index = 0; index < m_shaderCount; index++) {
            delete m_shaders[index];
        }
        delete[] m_shaders;
        m_shaders = NULL;
    }
    m_shaderCount = 0;
    m_sourceShaderCount = 0;
    m_finalized = 0;
    m_flag34 = 0;
    m_parameterTable = NULL;
    m_flags = 0;
}

unsigned char CShaderProgram::Load(components::CInputStream& stream, unsigned int type,
                                   components::CIdToObjectRouter* router)
{
    // (declared in reverse order of their stack slots)
    InitParam params[MaxLoadedParams];
    char name[NameBufferSize];
    components::CInputStream streams[MaxLoadedStreams];
    unsigned int streamTypes[MaxLoadedStreams];
    unsigned int objectTypes[MaxLoadedStreams];
    memset(params, 0, sizeof(params));

    unsigned char result = 0;
    unsigned char loaded = 1;
    int objectCount = 0;
    int streamCount = 0;
    int paramCount = 0;
    while (stream.Available() != 0) {
        InitParam& param = params[paramCount];
        paramCount++;
        unsigned int key = stream.ReadUInt32();
        switch (key) {
        case ProgramKey_ShaderId: {
            core::CSharedPtr<void*> object;
            loaded = router->KeyToPointer(stream.ReadUInt32(), object, objectTypes[objectCount]);
            if (loaded) {
                param.m_key = GetParamTypeOfKey(key);
                param.m_value = (unsigned int)*object;
                objectCount++;
            }
            break;
        }
        case ProgramKey_ShaderName: {
            char* cursor = name;
            unsigned char character;
            do {
                character = stream.ReadUInt8();
                *cursor++ = character;
            } while (character != 0);
            core::CSharedPtr<void*> object;
            loaded = router->NameToPointer(name, object, objectTypes[objectCount]);
            if (loaded) {
                param.m_key = GetParamTypeOfKey(key);
                param.m_value = (unsigned int)*object;
                objectCount++;
            }
            break;
        }
        case ProgramKey_ShaderSourceId:
            loaded = router->KeyToInputStream(stream.ReadUInt32(), streams[streamCount], streamTypes[streamCount]);
            if (loaded) {
                param.m_key = GetParamTypeOfKey(key);
                param.m_value = (unsigned int)&streams[streamCount];
                streamCount++;
            }
            break;
        case ProgramKey_ShaderSourceName: {
            char* cursor = name;
            unsigned char character;
            do {
                character = stream.ReadUInt8();
                *cursor++ = character;
            } while (character != 0);
            loaded = router->NameToInputStream(name, streams[streamCount], streamTypes[streamCount]);
            if (loaded) {
                param.m_key = GetParamTypeOfKey(key);
                param.m_value = (unsigned int)&streams[streamCount];
                streamCount++;
            }
            break;
        }
        default:
            param.m_key = GetParamTypeOfKey(key);
            param.m_value = stream.ReadUInt32();
            break;
        }
    }

    // the mime types follow the objects / streams they belong to
    for (int index = 0; index < objectCount; index++) {
        params[paramCount].m_key = ParamType_ShaderType;
        params[paramCount].m_value = objectTypes[index];
        paramCount++;
    }
    for (int index = 0; index < streamCount; index++) {
        params[paramCount].m_key = ParamType_ShaderSourceType;
        params[paramCount].m_value = streamTypes[index];
        paramCount++;
    }

    // a program is built either from shader objects or from shader sources
    if (loaded && (streamCount == 0 || objectCount == 0)) {
        result = Initialize(params);
    }
    return result;
}

unsigned char CShaderProgram::Initialize(const InitParam* params)
{
    Destroy();

    m_attributes = new Attribute[MaxAttributes];
    for (int index = 0; index < MaxAttributes; index++) {
        m_attributes[index].m_id = 0x7fffffff;
        m_attributes[index].m_type = 0;
        m_attributes[index].m_count = 0;
    }

    // count the entries of each kind; they have to come in pairs
    unsigned int attributeIdCount = 0;
    unsigned int attributeNameCount = 0;
    unsigned int attributeNameTypeCount = 0;
    unsigned int sourceTypeCount = 0;
    unsigned int sourceCount = 0;
    unsigned int shaderTypeCount = 0;
    unsigned int shaderCount = 0;
    for (const InitParam* param = params; param->m_key != ParamType_End; param++) {
        switch (param->m_key) {
        case ParamType_Shader:
            shaderCount++;
            break;
        case ParamType_ShaderType:
            shaderTypeCount++;
            break;
        case ParamType_ShaderSource:
            sourceCount++;
            break;
        case ParamType_ShaderSourceType:
            sourceTypeCount++;
            break;
        case ParamType_AttributeId:
            attributeIdCount++;
            break;
        case ParamType_AttributeName:
            attributeNameCount++;
            break;
        case ParamType_AttributeNameType:
            attributeNameTypeCount++;
            break;
        }
    }
    bool failed = (shaderCount != shaderTypeCount) | (sourceCount != sourceTypeCount) |
                  (attributeNameCount != attributeNameTypeCount) | (attributeNameCount > attributeIdCount);
    if (failed) {
        Destroy();
        return 0;
    }

    m_sourceShaderCount = sourceCount;
    m_shaderCount += sourceCount + shaderCount;
    m_shaders = new ICShader*[m_shaderCount];

    // the shader objects follow the shader sources in m_shaders / shaderTypes
    unsigned int nameTypes[MaxInitStreams];
    components::CInputStream* sources[MaxInitStreams];
    unsigned int shaderTypes[MaxInitStreams];
    unsigned int shaderIndex = m_sourceShaderCount;
    unsigned int shaderTypeIndex = m_sourceShaderCount;
    unsigned int sourceIndex = 0;
    unsigned int sourceTypeIndex = 0;
    unsigned int attributeIndex = 0;
    unsigned int nameTypeIndex = 0;
    for (const InitParam* param = params; param->m_key != ParamType_End && !failed; param++) {
        switch (param->m_key) {
        case ParamType_Shader:
            m_shaders[shaderIndex] = (ICShader*)param->m_value;
            shaderIndex++;
            break;
        case ParamType_ShaderType:
            shaderTypes[shaderTypeIndex] = param->m_value;
            shaderTypeIndex++;
            break;
        case ParamType_ShaderSource:
            sources[sourceIndex] = (components::CInputStream*)param->m_value;
            sourceIndex++;
            break;
        case ParamType_ShaderSourceType:
            shaderTypes[sourceTypeIndex] = param->m_value;
            sourceTypeIndex++;
            break;
        case ParamType_Flag5:
            m_flag34 = *(const unsigned char*)&param->m_value != 0;    // the original reads one byte (ldrb)
            break;
        case ParamType_AttributeId:
            m_attributes[attributeIndex].m_id = param->m_value;
            attributeIndex++;
            break;
        case ParamType_AttributeNameType:
            nameTypes[nameTypeIndex] = param->m_value;
            nameTypeIndex++;
            break;
        }
    }
    if (failed) {
        Destroy();
        return 0;
    }
    m_attributeCount = attributeIndex;

    // name the attributes
    unsigned int nameIndex = 0;
    for (const InitParam* param = params; param->m_key != ParamType_End && !failed; param++) {
        if (param->m_key == ParamType_AttributeName) {
            if (nameIndex >= m_attributeCount) {
                Destroy();
                return 0;
            }
            switch (nameTypes[nameIndex]) {
            case NameType_WideUtf8:
            case NameType_WideString:
                m_attributes[nameIndex].m_name = (const wchar_t*)param->m_value;
                break;
            case NameType_CharString:
                m_attributes[nameIndex].m_name = (const char*)param->m_value;
                break;
            case NameType_CStrWChar:
                m_attributes[nameIndex].m_name = ((const components::CStrWChar*)param->m_value)->m_string;
                break;
            case NameType_CStrChar:
                m_attributes[nameIndex].m_name = ((const components::CStrChar*)param->m_value)->m_string;
                break;
            default:
                failed = true;
                break;
            }
            nameIndex++;
        }
    }
    if (failed) {
        Destroy();
        return 0;
    }

    FinalizeInitialization(sources, shaderTypes);
    m_flags |= 0x80000000;
    return 1;
}

void CShaderProgram::SetParameterTable(ParameterTable& table)
{
    m_parameterTable = &table;
}

ICShaderProgram::ParameterTable* CShaderProgram::GetParameterTable()
{
    return m_parameterTable;
}

unsigned char CShaderProgram::GetDescription(Description& description)
{
    description.m_type = GetAbstraction();
    description.m_shaderCount = m_shaderCount;
    description.m_shaders = m_shaders;
    description.m_finalized = m_finalized;
    description.m_attributeCount = m_attributeCount;
    description.m_attributes = m_attributes;
    description.m_parameterCount = m_parameterCount;
    description.m_parameterArraySizes = m_parameterArraySizes;
    description.m_parameterTypes = m_parameterTypes;
    description.m_parameterNames = m_parameterNames;
    return 1;
}

unsigned char CShaderProgram::SaveStateForFutureRecreation()
{
    return 1;
}

unsigned char CShaderProgram::Recreate()
{
    return 1;
}

void CShaderProgram::ParseCommon(components::CInputStream** streams, unsigned int* types)
{
    unsigned int totalSize = 0;
    for (unsigned int index = 0; index < m_sourceShaderCount; index++) {
        totalSize += streams[index]->Available();
    }
    unsigned char* text = (unsigned char*)malloc(totalSize);

    // declarations: "<name> <type> <count>", tokens separated by blanks or ':'
    // (declared in reverse order of their stack slots)
    unsigned char* nameStarts[MaxDeclarations];
    unsigned char* typeStarts[MaxDeclarations];
    unsigned char* countStarts[MaxDeclarations];
    int nameLengths[MaxDeclarations];
    int typeLengths[MaxDeclarations];
    int countLengths[MaxDeclarations];
    char token[128];
    wchar_t wideToken[32];
    unsigned int nameCount = 0;
    unsigned int typeCount = 0;
    unsigned int countCount = 0;
    for (unsigned int index = 0; index < m_sourceShaderCount; index++) {
        if (types[index] == SourceType_Binary1 || types[index] == SourceType_Binary2) {
            continue;
        }
        components::CInputStream* source = streams[index];
        source->Read(text, source->Available());

        unsigned char expectName = 1;
        unsigned char inName = 0;
        unsigned char expectType = 0;
        unsigned char inType = 0;
        unsigned char expectCount = 0;
        unsigned char inCount = 0;
        int length = 0;
        for (unsigned int position = 0; position < totalSize; position++) {
            unsigned char* cursor = &text[position];
            unsigned char character = *cursor;
            if (character < ' ' || character > '~') {
                break;
            }
            if (character == ' ') {
                if (inName) {
                    nameLengths[nameCount - 1] = length;
                    inName = 0;
                    expectType = 1;
                } else if (inType) {
                    typeLengths[typeCount - 1] = length;
                    inType = 0;
                    expectCount = 1;
                } else if (inCount) {
                    countLengths[countCount - 1] = length;
                    inCount = 0;
                    expectName = 1;
                }
            } else if (character == ':') {
                if (inName) {
                    nameLengths[nameCount - 1] = length;
                    inName = 0;
                    expectType = 1;
                } else if (inType) {
                    typeLengths[typeCount - 1] = length;
                    inType = 0;
                    expectCount = 1;
                }
            } else {
                if (expectName) {
                    expectName = 0;
                    inName = 1;
                    length = 0;
                    nameStarts[nameCount] = cursor;
                    nameCount++;
                } else if (expectType) {
                    expectType = 0;
                    inType = 1;
                    length = 0;
                    typeStarts[typeCount] = cursor;
                    typeCount++;
                } else if (expectCount) {
                    expectCount = 0;
                    inCount = 1;
                    length = 0;
                    countStarts[countCount] = cursor;
                    countCount++;
                }
                length++;
            }
        }
        if (inCount) {
            countLengths[countCount - 1] = length;
        }
    }

    m_parameterCount = nameCount;
    m_parameterTypes = new unsigned int[m_parameterCount + 1];
    m_parameterTypes[m_parameterCount] = 0;
    m_parameterArraySizes = new int[m_parameterCount];
    m_parameterNames = new components::CStrWChar[m_parameterCount];
    m_parameterEntries = new int[m_parameterCount];
    for (unsigned int index = 0; index < m_parameterCount; index++) {
        memcpy(token, typeStarts[index], typeLengths[index]);
        token[typeLengths[index]] = 0;
        mbstowcs(wideToken, token, strlen(token) + 1);
        m_parameterTypes[index] = components::CTypedVariableTable::Entry::GetType(wideToken);

        memcpy(token, countStarts[index], countLengths[index]);
        token[countLengths[index]] = 0;
        m_parameterArraySizes[index] = atoi(token);

        memcpy(token, nameStarts[index], nameLengths[index]);
        token[nameLengths[index]] = 0;
        m_parameterNames[index] = token;

        m_parameterEntries[index] = 0x7fffffff;
    }
    free(text);
}

} } } }
