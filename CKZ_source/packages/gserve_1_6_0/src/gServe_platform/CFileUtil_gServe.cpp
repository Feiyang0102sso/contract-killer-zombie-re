// CFileUtil_gServe.cpp - application data files and the MD5-named download cache.
#include "CFileUtil_gServe.h"

#include <stdlib.h>
#include "ICFile.h"
#include "ICFileMgr.h"
#include "CMD5_gServe.h"
#include "CCRC32_gServe.h"

using namespace com::glu::platform;
using namespace com::glu::platform::components;

void CFileUtil_gServe::GetApplicationDataPathForFile(CStrWChar& outPath, const wchar_t* fileName, const wchar_t* subFolder)
{
    outPath = ICFileMgr::GetInstance()->GetApplicationDataPath();
    if (outPath.m_length > 0) {
        outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
    }
    if (subFolder != NULL) {
        outPath.Concatenate(subFolder);
        outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
    }
    outPath.Concatenate(fileName);
}

unsigned char CFileUtil_gServe::ReadFile(const wchar_t* path, unsigned char* buffer, unsigned int size)
{
    unsigned char result = 1;
    if (buffer != NULL && size != 0) {
        result = 0;
        ICFile* file = ICFileMgr::GetInstance()->Open(path, ICFileMgr::AccessRead);
        if (file != NULL) {
            unsigned int readSize = file->Read(buffer, size);
            result = (readSize == size);
            ICFileMgr::GetInstance()->Close(file);
        }
    }
    return result;
}

unsigned char CFileUtil_gServe::WriteFile(const wchar_t* path, const unsigned char* data, unsigned int size)
{
    unsigned char result = 0;
    if (data != NULL && size != 0) {
        result = 0;
        ICFile* file = ICFileMgr::GetInstance()->Open(path, ICFileMgr::AccessWrite);
        if (file != NULL) {
            unsigned int writtenSize = file->Write(data, size);
            result = (writtenSize == size);
            ICFileMgr::GetInstance()->Close(file);
        }
    }
    return result;
}

unsigned char CFileUtil_gServe::ReadApplicationDataFile(const wchar_t* fileName, unsigned char* buffer, unsigned int size,
                                                        const wchar_t* subFolder)
{
    CStrWChar path;
    GetApplicationDataPathForFile(path, fileName, subFolder);
    return ReadFile(path.m_string, buffer, size);
}

unsigned char CFileUtil_gServe::WriteApplicationDataFile(const wchar_t* fileName, const unsigned char* data,
                                                         unsigned int size, const wchar_t* subFolder)
{
    CStrWChar path;
    GetApplicationDataPathForFile(path, fileName, subFolder);
    return WriteFile(path.m_string, data, size);
}

void CFileUtil_gServe::GenerateCacheFilePath(const wchar_t* subFolder, const wchar_t* fileName, CStrWChar& outPath)
{
    GetApplicationCachePath(outPath);

    // the cached file is named by the MD5 of its (narrowed) name
    CStrWChar hashedName("");
    if (fileName != NULL) {
        char* narrowName = (char*)malloc(wcslen(fileName) + 1);
        wcstombs(narrowName, fileName, wcslen(fileName) + 1);
        char* digest = CMD5_gServe::GetInstance()->GenerateMD5(narrowName);
        hashedName = digest;
        free(digest);
        free(narrowName);
    }

    if (subFolder != NULL) {
        outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
        outPath.Concatenate(subFolder);
    }
    outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
    if (hashedName.m_length > 0) {
        outPath.Concatenate(hashedName.m_string);
    }
}

int CFileUtil_gServe::GetModificationDateFromCache(const wchar_t* subFolder, const wchar_t* fileName)
{
    CStrWChar path;
    GenerateCacheFilePath(subFolder, fileName, path);
    return GetModificationDate(path.m_string);
}

unsigned int CFileUtil_gServe::GetCRCFromFileInCache(const wchar_t* subFolder, const wchar_t* fileName)
{
    unsigned int crc = 0;
    unsigned int size = GetFilesizeFromCache(subFolder, fileName);
    if (size != 0) {
        unsigned char* data = (unsigned char*)malloc(size);
        if (data != NULL) {
            if (ReadFileFromCache(subFolder, fileName, data, size) == 1) {
                crc = CCRC32_gServe::GetInstance()->GenerateCRC32(data, size);
            }
            free(data);
        }
    }
    return crc;
}

unsigned int CFileUtil_gServe::GetFilesizeFromCache(const wchar_t* subFolder, const wchar_t* fileName)
{
    CStrWChar path;
    GenerateCacheFilePath(subFolder, fileName, path);
    return ICFileMgr::GetInstance()->Size(path.m_string);
}

unsigned char CFileUtil_gServe::ReadFileFromCache(const wchar_t* subFolder, const wchar_t* fileName,
                                                  unsigned char* buffer, unsigned int size)
{
    unsigned char result;
    CStrWChar path;
    GenerateCacheFilePath(subFolder, fileName, path);
    unsigned int fileSize = ICFileMgr::GetInstance()->Size(path.m_string);
    if (size != 0xffffffff && fileSize > size) {
        // original quirk: a file too big for the buffer is reported as success without reading it
        result = 1;
    } else {
        // the open result is not checked in the original
        ICFile* file = ICFileMgr::GetInstance()->Open(path.m_string, ICFileMgr::AccessRead);
        unsigned int readSize = file->Read(buffer, size);
        result = (readSize == fileSize);
        ICFileMgr::GetInstance()->Close(file);
    }
    return result;
}

unsigned char CFileUtil_gServe::IsFileInCache(const wchar_t* subFolder, const wchar_t* fileName)
{
    unsigned char exists;
    {
        CStrWChar path;
        GenerateCacheFilePath(subFolder, fileName, path);
        exists = ICFileMgr::GetInstance()->Exists(path.m_string);
    }
    return exists != 0;
}

unsigned char CFileUtil_gServe::SaveFileToCache(const wchar_t* subFolder, const wchar_t* fileName,
                                                unsigned char* data, unsigned int size)
{
    unsigned int writtenSize;   // original bug: left uninitialized when the file cannot be opened
    {
        CStrWChar path;
        GenerateCacheFilePath(subFolder, fileName, path);
        ICFile* file = ICFileMgr::GetInstance()->Open(path.m_string, ICFileMgr::AccessWrite);
        if (file != NULL) {
            writtenSize = file->Write(data, size);
        }
        ICFileMgr::GetInstance()->Close(file);
    }
    return writtenSize == size;
}

unsigned int CFileUtil_gServe::GetFilesize(const wchar_t* subFolder, const wchar_t* fileName)
{
    CStrWChar path;
    GetApplicationDataPathForFile(path, fileName, subFolder);
    return ICFileMgr::GetInstance()->Size(path.m_string);
}
