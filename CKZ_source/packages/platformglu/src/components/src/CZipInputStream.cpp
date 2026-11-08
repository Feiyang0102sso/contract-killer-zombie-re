// CZipInputStream.cpp - inflating input stream on top of another stream.
#include "CZipInputStream.h"

namespace com { namespace glu { namespace platform { namespace components {

CZipInputStream::CZipInputStream()
{
    m_inflateActive = 0;
    m_inputSize = 0;
    m_inputPos = 0;
    m_outputSize = 0;
    m_outputPos = 0;
    m_compressed = NULL;
    m_compressedSize = 0;
    m_compressedRead = 0;
    m_uncompressedSize = 0;
    m_uncompressedRead = 0;
}

CZipInputStream::~CZipInputStream()
{
    Close();
}

void CZipInputStream::Close()
{
    CInputStream::Close();
    if (m_inflateActive) {
        inflateEnd(&m_stream);
        m_inflateActive = 0;
    }
    m_length = 0;
    m_error = 0;
    m_compressed = NULL;
    m_compressedSize = 0;
    m_compressedRead = 0;
    m_inputSize = 0;
    m_inputPos = 0;
    m_outputSize = 0;
    m_outputPos = 0;
    m_uncompressedSize = 0;
    m_uncompressedRead = 0;
}

unsigned char CZipInputStream::Open(CInputStream& source, unsigned int compressedSize, unsigned int uncompressedSize)
{
    Close();
    m_length = uncompressedSize;
    m_compressed = &source;
    m_compressedSize = compressedSize;
    m_compressedRead = 0;
    CInputStream::Mark(&source);
    m_uncompressedSize = uncompressedSize;
    m_uncompressedRead = 0;

    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;
    m_stream.avail_in = 0;
    m_stream.next_in = Z_NULL;
    m_inflateActive = (inflateInit(&m_stream) == Z_OK);
    m_error = !m_inflateActive;
    return m_inflateActive;
}

unsigned char CZipInputStream::IsOpenInternal()
{
    return m_inflateActive;
}

unsigned int CZipInputStream::AvailableInternal()
{
    if (m_error) {
        return 0;
    }
    return m_uncompressedSize - m_uncompressedRead;
}

void CZipInputStream::SkipInternal(unsigned int size)
{
    if (Available() < size) {
        m_error = 1;
        return;
    }

    while (!m_error && size != 0) {
        if (m_outputPos < m_outputSize) {
            unsigned int chunk = m_outputSize - m_outputPos;
            if (size < chunk) {
                chunk = size;
            }
            m_outputPos += chunk;
            size -= chunk;
            m_uncompressedRead += chunk;
        } else {
            ZipReadChunk();
            if (m_outputSize == 0) {
                m_error = 1;
            }
        }
    }
}

void CZipInputStream::ZipReadChunk()
{
    m_outputSize = 0;
    m_outputPos = 0;
    if (m_inflateActive) {
        // keep the unconsumed input at the start of the buffer
        unsigned int buffered = 0;
        if (m_inputPos < m_inputSize) {
            buffered = m_inputSize - m_inputPos;
            memmove(m_input, m_input + m_inputPos, buffered);
            m_inputSize = buffered;
            m_inputPos = 0;
        } else {
            m_inputSize = 0;
            m_inputPos = 0;
        }

        // top it up from the source
        if (buffered < InputBufferSize && m_compressedRead < m_compressedSize) {
            unsigned int toRead = m_compressedSize - m_compressedRead;
            if (InputBufferSize - buffered < toRead) {
                toRead = InputBufferSize - buffered;
            }
            m_compressed->Read(m_input + buffered, toRead);
            m_compressedRead += toRead;
            buffered = m_inputSize + toRead;
            m_inputSize = buffered;
            m_error = m_compressed->m_error;
        }
        if (m_error) {
            return;
        }

        m_stream.avail_in = buffered;
        m_stream.next_in = m_input;
        m_stream.avail_out = OutputBufferSize;
        m_stream.next_out = m_output;
        int result = inflate(&m_stream, Z_NO_FLUSH);
        if (result != Z_OK && result != Z_STREAM_END) {
            m_error = 1;
        } else {
            m_outputSize = OutputBufferSize - m_stream.avail_out;
            m_inputPos = m_inputSize - m_stream.avail_in;
            if (result == Z_OK) {
                return;
            }
        }

        // finished or failed: release the inflater
        if (m_inflateActive) {
            inflateEnd(&m_stream);
            m_inflateActive = 0;
        }
    } else {
        m_error = 1;
    }
}

void CZipInputStream::ReadInternal(unsigned char* buffer, unsigned int size)
{
    if (buffer == NULL || size == 0) {
        return;
    }

    if (Available() >= size) {
        unsigned int copied = 0;
        while (!m_error && size != 0) {
            if (m_outputPos < m_outputSize) {
                unsigned int chunk = m_outputSize - m_outputPos;
                if (size < chunk) {
                    chunk = size;
                }
                memcpy(buffer + copied, m_output + m_outputPos, chunk);
                size -= chunk;
                copied += chunk;
                m_outputPos += chunk;
                m_uncompressedRead += chunk;
            } else {
                ZipReadChunk();
            }
        }
    } else {
        m_error = 1;
    }
    if (m_error) {
        // note: clears the start of the buffer, not the part that was left unfilled
        memset(buffer, 0, size);
    }
}

void CZipInputStream::ResetInternal()
{
    CInputStream::ResetInternal();
    CInputStream::Reset(m_compressed);
    Open(*m_compressed, m_compressedSize, m_length);
}

} } } }
