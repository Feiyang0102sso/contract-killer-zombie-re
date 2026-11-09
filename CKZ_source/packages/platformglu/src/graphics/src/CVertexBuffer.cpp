// CVertexBuffer.cpp - vertex buffer in system memory.
#include "CVertexBuffer.h"

namespace com { namespace glu { namespace platform { namespace graphics {

CVertexBuffer::~CVertexBuffer()
{
    CVertexBuffer::Destroy();
}

void CVertexBuffer::Destroy()
{
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    m_size = 0;
    m_stride = 0;
    m_length = 0;
    m_decl.m_attributes.Clear();
}

unsigned char CVertexBuffer::Initialize(const CVertex::Attribute::Decl* decl, int count, int stride)
{
    m_decl.Set(decl, 1);
    if (stride == 0) {
        // the stride is the end of the last attribute, padded to 4 bytes
        CVertex::Attribute::Decl& last = m_decl.m_attributes[m_decl.m_attributes.m_size - 1].m_decl;
        unsigned int type = last.m_type;
        int offset = last.m_offset;
        int componentCount = last.m_count;
        int attributeSize = componentCount * GetComponentSize(type);
        int padding = (-componentCount * GetComponentSize(type)) & 3;
        stride = offset + attributeSize + padding;
    }
    m_size = stride * count;
    m_stride = stride;
    m_length = count;
    m_data = new unsigned char[(unsigned int)m_size];
    return 1;
}

void CVertexBuffer::Lock(unsigned char** data, int& size, int offset, int count, LockType lockType)
{
    if (m_data != NULL) {
        int available = m_size - offset;
        if (available > count) {
            available = count;
        }
        size = available;
        if (available > 0) {
            *data = m_data + offset;
            return;
        }
    }
    *data = NULL;
    size = 0;
}

void CVertexBuffer::Lock(Itr& itr, int first, int count, LockType lockType)
{
    if (m_data != NULL) {
        int available;
        if (count == 0x7fffffff) {
            available = m_size - m_stride * first;
        } else {
            int wanted = m_stride * count;
            available = m_size - m_stride * first;
            available = wanted < available ? wanted : available;
        }
        if (available > 0) {
            unsigned char* begin = m_data + m_stride * first;
            itr.m_begin = begin;
            itr.m_current = begin;
            itr.m_size = available;
            itr.m_stride = m_stride;
            itr.m_decl = &m_decl;
            return;
        }
    }
    itr.m_begin = NULL;
    itr.m_current = NULL;
    itr.m_size = 0;
}

ICBuffer::Type CVertexBuffer::GetType()
{
    return Type_Vertex;
}

CVertex::Decl* CVertexBuffer::GetDecl()
{
    return &m_decl;
}

int CVertexBuffer::GetStride()
{
    return m_stride;
}

int CVertexBuffer::GetLength()
{
    return m_length;
}

int CVertexBuffer::GetSize()
{
    return m_size;
}

ICGraphicsAbstract::Abstraction CVertexBuffer::GetAbstraction() const
{
    return ICGraphicsAbstract::Abstraction_Hardware;
}

} } } }
