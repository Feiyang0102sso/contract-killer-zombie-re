// CNGSFileMgr.h - gserve file system helpers that need Foundation (CNGSFileMgr.mm).
#ifndef CNGSFILEMGR_H
#define CNGSFILEMGR_H

#include <wchar.h>

class CNGSFileMgr {
public:
    /** Create the directory `path` with its missing parents; 1 when it exists afterwards. */
    static unsigned char MkDir(const wchar_t* path);
};

#endif
