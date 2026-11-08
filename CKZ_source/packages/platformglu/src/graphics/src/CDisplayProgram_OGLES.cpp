// CDisplayProgram_OGLES.cpp - OpenGL ES flavour of the display program.
#include "CDisplayProgram_OGLES.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// default instruction memory and shared buffer sizes
static const unsigned int OGLES_NodeChunkLength = 0x2800;       // words per instruction block
static const unsigned int OGLES_IndexBufferLength = 0x6bd0;     // indices per shared index buffer
static const unsigned int OGLES_VertexBufferLength = 0x47e0;    // vertices per shared vertex buffer
static const unsigned int OGLES_IndexStreamLength = 0x228;      // indices per batch
static const unsigned int OGLES_VertexStreamLength = 0x170;     // vertices per batch

CDisplayProgram_OGLES::~CDisplayProgram_OGLES()
{
    CDisplayProgram_OGLES::Destroy();
}

void CDisplayProgram_OGLES::Destroy()
{
    CDisplayProgram::Destroy();
    m_deviceVersion = 0;
}

unsigned char CDisplayProgram_OGLES::GetDescription(Description& description)
{
    description.m_type = 1;
    return 1;
}

ICGraphicsAbstract::Abstraction CDisplayProgram_OGLES::GetAbstraction() const
{
    return ICGraphicsAbstract::Abstraction_1;
}

void CDisplayProgram_OGLES::ConfigDefaultAllocationInfo()
{
    m_nodeChunkLength = OGLES_NodeChunkLength;
    m_indexBufferLength = OGLES_IndexBufferLength;
    m_vertexBufferLength = OGLES_VertexBufferLength;
    m_indexStreamLength = OGLES_IndexStreamLength;
    m_vertexStreamLength = OGLES_VertexStreamLength;
}

} } } }
