// CFileUtil_gServe.h - gserve file helpers: whole-file read/write in the application data folder and a
// download cache (files named by the MD5 of their name inside the OS cache folder).
// Implemented in CFileUtil_gServe.cpp and, for the parts that need Foundation, CFileUtil_gServe.mm.
#ifndef CFILEUTIL_GSERVE_H
#define CFILEUTIL_GSERVE_H

#include <wchar.h>
#include "CStrWChar.h"

class CFileUtil_gServe {
public:
    /** outPath = data folder [+ separator + subFolder] + separator + fileName. */
    static void GetApplicationDataPathForFile(com::glu::platform::components::CStrWChar& outPath,
                                              const wchar_t* fileName, const wchar_t* subFolder);
    /** Read exactly `size` bytes; 1 on success (also when there is nothing to read). */
    static unsigned char ReadFile(const wchar_t* path, unsigned char* buffer, unsigned int size);
    /** Write `size` bytes; 1 when all were written. */
    static unsigned char WriteFile(const wchar_t* path, const unsigned char* data, unsigned int size);
    static unsigned char ReadApplicationDataFile(const wchar_t* fileName, unsigned char* buffer, unsigned int size,
                                                 const wchar_t* subFolder);
    static unsigned char WriteApplicationDataFile(const wchar_t* fileName, const unsigned char* data,
                                                  unsigned int size, const wchar_t* subFolder);
    /** outPath = cache folder [+ separator + subFolder] + separator + MD5(fileName). */
    static void GenerateCacheFilePath(const wchar_t* subFolder, const wchar_t* fileName,
                                      com::glu::platform::components::CStrWChar& outPath);
    /** Modification time (seconds since 1970) of a cached file, 0 when missing. */
    static int GetModificationDateFromCache(const wchar_t* subFolder, const wchar_t* fileName);
    /** CRC-32 of a cached file, 0 when it cannot be read. */
    static unsigned int GetCRCFromFileInCache(const wchar_t* subFolder, const wchar_t* fileName);
    static unsigned int GetFilesizeFromCache(const wchar_t* subFolder, const wchar_t* fileName);
    /** Read a cached file into `buffer` (`size` = 0xffffffff: no size check); 1 when read completely. */
    static unsigned char ReadFileFromCache(const wchar_t* subFolder, const wchar_t* fileName,
                                           unsigned char* buffer, unsigned int size);
    static unsigned char IsFileInCache(const wchar_t* subFolder, const wchar_t* fileName);
    static unsigned char SaveFileToCache(const wchar_t* subFolder, const wchar_t* fileName,
                                         unsigned char* data, unsigned int size);
    /** Size of a file in the application data folder. */
    static unsigned int GetFilesize(const wchar_t* subFolder, const wchar_t* fileName);

    // CFileUtil_gServe.mm
    /** Modification time (seconds since 1970) of `path`, 0 when missing. */
    static int GetModificationDate(const wchar_t* path);
    /** The OS cache folder (looked up once). */
    static void GetApplicationCachePath(com::glu::platform::components::CStrWChar& outPath);
};

#endif
