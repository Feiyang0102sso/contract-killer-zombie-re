// CFileInputStream.cpp - input stream over an ICFile.
#include "CFileInputStream.h"
#include "ICFile.h"
#include "ICFileMgr.h"

namespace com { namespace glu { namespace platform { namespace components {

CFileInputStream::CFileInputStream()
{
    m_file = NULL;
    m_ownsFile = 0;
    m_markPosition = 0;
}

CFileInputStream::~CFileInputStream()
{
    Close();
}

void CFileInputStream::Close()
{
    CInputStream::Close();
    if (m_ownsFile) {
        if (m_file != NULL) {
            delete m_file;
            m_file = NULL;
        }
    } else {
        m_file = NULL;
    }
    m_markPosition = 0;
}

unsigned char CFileInputStream::Open(const wchar_t* path)
{
    Close();
    m_error = 1;
    if (path == NULL) {
        return 0;
    }
    m_length = ICFileMgr::GetInstance()->Size(path);
    if (m_length != 0) {
        m_ownsFile = 1;
        m_file = ICFileMgr::GetInstance()->Open(path, ICFileMgr::AccessRead);
        m_error = (m_file == NULL);
    }
    Mark();
    return m_error == 0;
}

unsigned char CFileInputStream::Open(ICFile* file)
{
    unsigned char result = 0;
    Close();
    m_error = 1;
    if (file != NULL) {
        // size = end position, then go back to where the file was
        unsigned int position = file->Tell();
        unsigned char error = 1;
        if (file->Seek(0, ICFile::SeekEnd)) {
            m_length = file->Tell();
            error = !file->Seek(position, ICFile::SeekSet);
        }
        m_ownsFile = 0;
        m_file = file;
        m_error = error;
        Mark();
        if (!m_error) {
            result = 1;
        }
    }
    return result;
}

unsigned char CFileInputStream::IsOpenInternal()
{
    return m_file != NULL;
}

unsigned int CFileInputStream::AvailableInternal()
{
    unsigned int available = 0;
    if (m_file != NULL && !m_error) {
        available = m_length - m_file->Tell();
    }
    return available;
}

void CFileInputStream::SkipInternal(unsigned int size)
{
    if (size == 0) {
        return;
    }
    if (Available() >= size) {
        m_error = !m_file->Seek(size, ICFile::SeekCurrent);
    } else {
        m_error = 1;
    }
}

void CFileInputStream::ReadInternal(unsigned char* buffer, unsigned int size)
{
    if (buffer == NULL || size == 0) {
        return;
    }
    if (Available() < size) {
        m_error = 1;
    } else {
        unsigned int readSize = m_file->Read(buffer, size);
        m_error = (readSize != size);
        if (readSize == size) {
            return;
        }
    }
    memset(buffer, 0, size);
}

void CFileInputStream::ResetInternal()
{
    CInputStream::ResetInternal();
    if (m_file != NULL) {
        m_file->Seek(m_markPosition, ICFile::SeekSet);
    }
}

void CFileInputStream::MarkInternal()
{
    if (m_file != NULL) {
        m_markPosition = m_file->Tell();
    }
}

} } } }
