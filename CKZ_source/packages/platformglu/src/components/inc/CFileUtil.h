// CFileUtil.h - whole-file helpers and application path building on top of ICFileMgr.
#ifndef CFILEUTIL_H
#define CFILEUTIL_H

#include <wchar.h>

namespace com { namespace glu { namespace platform { namespace components {

class CStrWChar;

class CFileUtil {
public:
    /** Directory part of `fileName` (without the trailing separator); empty when there is none. */
    static void GetFilePath(CStrWChar& outPath, const wchar_t* fileName);
    static void GetApplicationPathForFile(CStrWChar& outPath, const wchar_t* fileName);
    static void GetApplicationDataPathForFile(CStrWChar& outPath, const wchar_t* fileName);
    static void GetApplicationCachePathForFile(CStrWChar& outPath, const wchar_t* fileName);

    /** Read exactly `size` bytes; an empty request succeeds. */
    static unsigned char ReadFile(const wchar_t* path, unsigned char* buffer, unsigned int size);
    /** Write exactly `size` bytes. */
    static unsigned char WriteFile(const wchar_t* path, const unsigned char* data, unsigned int size);
    /** Write through a temp file then rename; empty data deletes the file. */
    static unsigned char SafeWriteFile(const wchar_t* path, const unsigned char* data, unsigned int size);
    static unsigned char WriteApplicationDataFile(const wchar_t* fileName, const unsigned char* data, unsigned int size);
};

} } } }

#endif
