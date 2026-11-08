// CFile_Cocoa.h - ICFile on top of a C stdio FILE (opened by CFileMgr_Cocoa::Open).
#ifndef CFILE_COCOA_H
#define CFILE_COCOA_H

#include <stdio.h>
#include "ICFile.h"

class CFile_Cocoa : public com::glu::platform::components::ICFile {
public:
    /** Runtime class id of the platform file. */
    static const unsigned int ClassId = 0x02d70fa5;

    CFile_Cocoa();
    /** Closes the file when still open. */
    virtual ~CFile_Cocoa();

    virtual unsigned char IsOpen();
    virtual unsigned char Seek(int offset, eFileSeekMode mode);
    virtual unsigned int Tell();
    virtual unsigned int Read(unsigned char* buffer, unsigned int size);
    virtual unsigned int Write(const unsigned char* data, unsigned int size);

    FILE* m_file;       // 0x08 NULL while closed
};

#endif
