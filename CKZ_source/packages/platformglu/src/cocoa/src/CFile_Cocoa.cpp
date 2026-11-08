// CFile_Cocoa.cpp - ICFile on top of a C stdio FILE.
#include "CFile_Cocoa.h"

CFile_Cocoa::CFile_Cocoa()
{
    m_classId = ClassId;
    m_file = NULL;
}

CFile_Cocoa::~CFile_Cocoa()
{
    if (m_file != NULL) {
        fclose(m_file);
    }
    m_file = NULL;
}

unsigned char CFile_Cocoa::IsOpen()
{
    return m_file != NULL;
}

unsigned char CFile_Cocoa::Seek(int offset, eFileSeekMode mode)
{
    if (m_file == NULL) {
        return 0;
    }
    // eFileSeekMode uses the stdio SEEK_SET / SEEK_CUR / SEEK_END values
    return fseek(m_file, offset, mode) == 0;
}

unsigned int CFile_Cocoa::Tell()
{
    if (m_file == NULL) {
        return 0;
    }
    return ftell(m_file);
}

unsigned int CFile_Cocoa::Read(unsigned char* buffer, unsigned int size)
{
    if (m_file == NULL) {
        return 0;
    }
    return fread(buffer, 1, size, m_file);
}

unsigned int CFile_Cocoa::Write(const unsigned char* data, unsigned int size)
{
    if (m_file == NULL) {
        return 0;
    }
    return fwrite(data, 1, size, m_file);
}
