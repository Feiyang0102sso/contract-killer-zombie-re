// CIndexBuffer.cpp - index buffer in system memory.
#include "CIndexBuffer.h"

namespace com { namespace glu { namespace platform { namespace graphics {

CIndexBuffer::~CIndexBuffer()
{
    CIndexBuffer::Destroy();
}

void CIndexBuffer::Destroy()
{
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    m_stride = 0;
    m_size = 0;
    m_length = 0;
    m_indexType = 0;
}

unsigned char CIndexBuffer::Initialize(unsigned int indexType, int count, int stride)
{
    m_indexType = indexType;
    if (stride == 0) {
        stride = GetComponentSize(indexType);
    }
    m_size = stride * count;
    m_stride = stride;
    m_length = count;
    m_data = new unsigned char[(unsigned int)m_size];
    return 1;
}

void CIndexBuffer::Lock(unsigned char** data, int& size, int offset, int count, LockType lockType)
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

void CIndexBuffer::Lock(Itr& itr, int first, int count, LockType lockType)
{
    if (m_data != NULL) {
        int available;
        if (count == 0x7fffffff) {
            available = m_size - m_stride * first;
        } else {
            // the wanted size first (as CVertexBuffer::Lock): keeps the loads inside both branches (fx1)
            int wanted = m_stride * count;
            available = m_size - m_stride * first;
            if (wanted < available) {
                available = wanted;
            }
        }
        if (available > 0) {
            unsigned char* begin = m_data + m_stride * first;
            itr.m_begin = begin;
            itr.m_current = begin;
            itr.m_size = available;
            itr.m_stride = m_stride;
            itr.m_indexType = m_indexType;
            return;
        }
    }
    itr.m_begin = NULL;
    itr.m_current = NULL;
    itr.m_size = 0;
}

ICBuffer::Type CIndexBuffer::GetType()
{
    return Type_Index;
}

unsigned int CIndexBuffer::GetIndexType()
{
    return m_indexType;
}

int CIndexBuffer::GetStride()
{
    return m_stride;
}

int CIndexBuffer::GetLength()
{
    return m_length;
}

int CIndexBuffer::GetSize()
{
    return m_size;
}

ICGraphicsAbstract::Abstraction CIndexBuffer::GetAbstraction() const
{
    return ICGraphicsAbstract::Abstraction_Hardware;
}

} } } }
