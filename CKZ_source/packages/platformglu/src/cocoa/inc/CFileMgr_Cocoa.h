// CFileMgr_Cocoa.h - iOS file system: application bundle / Documents / Caches paths, stdio files,
// POSIX directories and NSDirectoryEnumerator enumeration.
// C++ part in CFileMgr_Cocoa.cpp, Foundation part in CFileMgr_Cocoa_mm.mm.
#ifndef CFILEMGR_COCOA_H
#define CFILEMGR_COCOA_H

#include <wchar.h>
#include "ICFileMgr.h"

#ifdef __OBJC__
@class NSDirectoryEnumerator;
#else
typedef void NSDirectoryEnumerator;
#endif

class CFileMgr_Cocoa : public com::glu::platform::components::ICFileMgr {
public:
    /** Path buffers in characters (wide) or bytes (UTF-8). */
    enum { PathLength = 256 };

    CFileMgr_Cocoa()
    {
        ReadPathInfo();
        m_enumerator = NULL;
        m_enumFileName[0] = 0;
    }

    virtual ~CFileMgr_Cocoa();

    virtual const wchar_t* GetApplicationPath();
    virtual const wchar_t* GetApplicationDataPath();
    virtual const wchar_t* GetApplicationTempPath();
    virtual const wchar_t* GetApplicationCachePath();
    virtual const wchar_t* GetPathSeparator();
    /** Start enumerating `path` (empty: the data directory); `directories` selects directories or files. */
    virtual unsigned char EnumInit(const wchar_t* path, unsigned char directories);
    virtual unsigned char EnumNext(EnumDirectoryFileInfo* info);
    virtual com::glu::platform::components::ICFile* Open(const wchar_t* path, eFileAccessMode mode);
    virtual void Close(com::glu::platform::components::ICFile* file);
    virtual unsigned char Exists(const wchar_t* path);
    virtual unsigned int Size(const wchar_t* path);
    virtual unsigned char Rename(const wchar_t* from, const wchar_t* to);
    virtual unsigned char Delete(const wchar_t* path);
    virtual unsigned char ExistsDir(const wchar_t* path);
    virtual unsigned char MkDir(const wchar_t* path);
    virtual unsigned char RmDir(const wchar_t* path);
    virtual unsigned char DeleteFilesInDir(const wchar_t* path);
    virtual unsigned int GetFreeDiskSpace();

    /** Fill the application, data, temp and cache paths from the bundle and the search paths. */
    void ReadPathInfo();

    wchar_t m_applicationPath[PathLength];      // 0x0008 bundle resources
    wchar_t m_dataPath[PathLength];             // 0x0408 Documents
    wchar_t m_tempPath[PathLength];             // 0x0808 Caches
    wchar_t m_cachePath[PathLength];            // 0x0c08 Caches
    NSDirectoryEnumerator* m_enumerator;        // 0x1008 EnumInit() .. EnumNext()
    wchar_t m_enumFileName[PathLength];         // 0x100c name returned by EnumNext()
    unsigned char m_enumDirectories;            // 0x140c enumerate directories instead of files
    char m_utf8Path[PathLength];                // 0x140d UTF-8 conversion buffers
    char m_utf8Path2[PathLength];               // 0x150d
};

#endif
