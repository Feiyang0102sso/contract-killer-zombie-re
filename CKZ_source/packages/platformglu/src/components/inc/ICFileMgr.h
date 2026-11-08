// (first drafted by b005 as PROVISIONAL; completed by b002 with the implementation CFileMgr_Cocoa;
// class_owners.json lists packages/gserve_1_6_0/src/gServe_platform/CFileUtil_gServe.cpp)
// ICFileMgr.h - platform file system interface; one instance per application (singleton).
#ifndef ICFILEMGR_H
#define ICFILEMGR_H

#include <wchar.h>
#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace components {

class ICFile;

class ICFileMgr : public CSingleton {
public:
    enum { SingletonId = 0x70fa1bdf };

    enum eFileAccessMode {
        AccessRead = 0,     // "rb"
        AccessWrite = 1,    // "wb"
        AccessAppend = 2
    };

    struct EnumDirectoryFileInfo;

    ICFileMgr() : CSingleton(SingletonId) {}

    inline virtual ~ICFileMgr()
    {
        if (CApplet::m_pApp != NULL) {
            CApplet::m_pApp->m_fileMgr = NULL;
        }
    }

    /** Platform factory (CFileMgr_Cocoa). */
    static ICFileMgr* CreateInstance();

    /** The application's file manager: cached in CApp, created on first use. */
    static inline ICFileMgr* GetInstance()
    {
        framework::CApp* app = CApplet::m_pApp;
        ICFileMgr* instance;
        if (app != NULL) {
            instance = app->m_fileMgr;
            if (instance == NULL) {
                ICFileMgr* found = NULL;
                app->m_singletons->Find(SingletonId, (void**)&found);
                instance = found;
                if (instance == NULL) {
                    instance = CreateInstance();
                }
                app->m_fileMgr = instance;
            }
        } else {
            instance = NULL;
        }
        return instance;
    }

    virtual const wchar_t* GetApplicationPath() = 0;
    virtual const wchar_t* GetApplicationDataPath() = 0;
    virtual const wchar_t* GetApplicationTempPath() = 0;
    virtual const wchar_t* GetApplicationCachePath() = 0;
    virtual const wchar_t* GetPathSeparator() = 0;
    virtual unsigned char EnumInit(const wchar_t* path, unsigned char recursive) = 0;
    virtual unsigned char EnumNext(EnumDirectoryFileInfo* info) = 0;
    /** NULL when the file cannot be opened. */
    virtual ICFile* Open(const wchar_t* path, eFileAccessMode mode) = 0;
    virtual void Close(ICFile* file) = 0;
    virtual unsigned char Exists(const wchar_t* path) = 0;
    /** File size in bytes, 0 when missing. */
    virtual unsigned int Size(const wchar_t* path) = 0;
    virtual unsigned char Rename(const wchar_t* from, const wchar_t* to) = 0;
    virtual unsigned char Delete(const wchar_t* path) = 0;
    virtual unsigned char ExistsDir(const wchar_t* path) = 0;
    virtual unsigned char MkDir(const wchar_t* path) = 0;
    virtual unsigned char RmDir(const wchar_t* path) = 0;
    virtual unsigned char DeleteFilesInDir(const wchar_t* path) = 0;
    virtual unsigned int GetFreeDiskSpace() = 0;
};

/** One directory entry returned by EnumNext(). */
struct ICFileMgr::EnumDirectoryFileInfo {
    enum { Attribute_Directory = 2 };
    unsigned int m_size;            // 0x00 file size in bytes
    unsigned int m_attributes;      // 0x04 Attribute_Directory or 0
    const wchar_t* m_name;          // 0x08 path relative to the enumerated directory
};

} } } }

#endif
