// CFileUtil.cpp - file helpers.
#include "CFileUtil.h"
#include "CStrWChar.h"
#include "ICFile.h"
#include "ICFileMgr.h"

namespace com { namespace glu { namespace platform { namespace components {

/** Name of the scratch file SafeWriteFile() writes before renaming. */
const wchar_t kwcsTempFileName[] = L"temp.dat";

void CFileUtil::GetFilePath(CStrWChar& outPath, const wchar_t* fileName)
{
    outPath = "";
    if (fileName == NULL) {
        return;
    }
    const wchar_t* separator = ICFileMgr::GetInstance()->GetPathSeparator();
    const wchar_t* lastSeparator = wcsrchr(fileName, *separator);
    if (lastSeparator == NULL) {
        return;
    }
    outPath = fileName;
    outPath = outPath.GetSubString(0, (lastSeparator - fileName) - 1);
}

void CFileUtil::GetApplicationPathForFile(CStrWChar& outPath, const wchar_t* fileName)
{
    outPath = ICFileMgr::GetInstance()->GetApplicationPath();
    if (outPath.m_length > 0) {
        outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
    }
    outPath.Concatenate(fileName);
}

void CFileUtil::GetApplicationDataPathForFile(CStrWChar& outPath, const wchar_t* fileName)
{
    outPath = ICFileMgr::GetInstance()->GetApplicationDataPath();
    if (outPath.m_length > 0) {
        outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
    }
    outPath.Concatenate(fileName);
}

void CFileUtil::GetApplicationCachePathForFile(CStrWChar& outPath, const wchar_t* fileName)
{
    outPath = ICFileMgr::GetInstance()->GetApplicationCachePath();
    if (outPath.m_length > 0) {
        outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
    }
    outPath.Concatenate(fileName);
}

unsigned char CFileUtil::ReadFile(const wchar_t* path, unsigned char* buffer, unsigned int size)
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

unsigned char CFileUtil::WriteFile(const wchar_t* path, const unsigned char* data, unsigned int size)
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

unsigned char CFileUtil::SafeWriteFile(const wchar_t* path, const unsigned char* data, unsigned int size)
{
    unsigned char result;
    if (data != NULL && size != 0) {
        // write next to the target first, then swap it in
        CStrWChar tempPath;
        GetFilePath(tempPath, path);
        tempPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
        tempPath.Concatenate(kwcsTempFileName);
        result = 0;
        ICFile* file = ICFileMgr::GetInstance()->Open(tempPath.m_string, ICFileMgr::AccessWrite);
        if (file != NULL) {
            unsigned int writtenSize = file->Write(data, size);
            result = (writtenSize == size);
            ICFileMgr::GetInstance()->Close(file);
            if (writtenSize == size) {
                ICFileMgr::GetInstance()->Delete(path);
                result = ICFileMgr::GetInstance()->Rename(tempPath.m_string, path);
            }
        }
        ICFileMgr::GetInstance()->Delete(tempPath.m_string);
    } else {
        // nothing to write: the file is removed
        ICFileMgr::GetInstance()->Delete(path);
        result = 1;
    }
    return result;
}

unsigned char CFileUtil::WriteApplicationDataFile(const wchar_t* fileName, const unsigned char* data, unsigned int size)
{
    CStrWChar path;
    GetApplicationDataPathForFile(path, fileName);
    return WriteFile(path.m_string, data, size);
}

} } } }
