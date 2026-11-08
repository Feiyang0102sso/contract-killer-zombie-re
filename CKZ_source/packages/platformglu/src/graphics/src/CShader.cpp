// CShader.cpp - platform independent part of a shader.
#include "CShader.h"
#include "CInputStream.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// Shader mime type keys accepted by Load (the same keys CResourceFactory maps to CResourceShader).
static const unsigned int ShaderType_2c43455a = 0x2c43455a;
static const unsigned int ShaderType_2c43455c = 0x2c43455c;
static const unsigned int ShaderType_2c435e5a = 0x2c435e5a;   // the only one loaded as pipeline type 3
static const unsigned int ShaderType_2c435e5c = 0x2c435e5c;
static const unsigned int ShaderType_cc4fcbf0 = 0xcc4fcbf0;
static const unsigned int ShaderType_cc4fcbf6 = 0xcc4fcbf6;
static const unsigned int ShaderType_2d8e7a27 = 0x2d8e7a27;

CShader::CShader()
{
    m_pipelineType = PipelineType_Undefined;
    m_source = NULL;
    m_sourceLength = 0;
    m_needsCompile = 1;
}

CShader::~CShader()
{
    CShader::Destroy();
}

void CShader::Destroy()
{
    if (m_source != NULL) {
        free(m_source);
        m_source = NULL;
    }
    m_sourceLength = 0;
    m_pipelineType = PipelineType_Undefined;
    m_needsCompile = 1;
}

unsigned char CShader::Load(components::CInputStream& stream, unsigned int type)
{
    unsigned char result = 0;
    if (stream.Available() != 0) {
        switch (type) {
        case ShaderType_2c43455a:
        case ShaderType_2c43455c:
        case ShaderType_2c435e5a:
        case ShaderType_2c435e5c:
        case ShaderType_cc4fcbf0:
        case ShaderType_cc4fcbf6:
        case ShaderType_2d8e7a27:
            if (type == ShaderType_2c435e5a) {
                m_pipelineType = (PipelineType)3;
            } else {
                m_pipelineType = (PipelineType)2;
            }
            result = SetSource(stream, SourceType_Binary);
            break;
        }
    }
    return result;
}

void CShader::SetPipelineType(PipelineType type)
{
    m_pipelineType = type;
}

ICShader::PipelineType CShader::GetPipelineType() const
{
    return m_pipelineType;
}

unsigned char CShader::SetSource(components::CInputStream& stream, unsigned int type)
{
    if (m_source != NULL) {
        free(m_source);
        m_source = NULL;
    }
    m_sourceLength = 0;

    unsigned char result = 0;
    if (stream.Available() != 0) {
        result = 1;
        m_sourceLength = (unsigned short)stream.Available();
        m_source = (char*)malloc(m_sourceLength + 1);
        stream.Read((unsigned char*)m_source, m_sourceLength);
        if (stream.m_error) {
            if (m_source != NULL) {
                free(m_source);
                m_source = NULL;
            }
            result = 0;
            m_sourceLength = 0;
        } else {
            m_source[m_sourceLength] = 0;
            m_needsCompile = result;
        }
    }
    return result;
}

void CShader::DiscardSource()
{
}

unsigned char CShader::SaveStateForFutureRecreation()
{
    return 1;
}

unsigned char CShader::Recreate()
{
    return 1;
}

} } } }
