// CFileInputStream.h - input stream reading a platform file.
#ifndef CFILEINPUTSTREAM_H
#define CFILEINPUTSTREAM_H

#include <wchar.h>
#include "CInputStream.h"

namespace com { namespace glu { namespace platform { namespace components {

class ICFile;

class CFileInputStream : public CInputStream {
public:
    CFileInputStream();
    virtual ~CFileInputStream();

    virtual void Close();
    /** Open `path` for reading (the stream owns the file). */
    unsigned char Open(const wchar_t* path);
    /** Read from an already open file (not owned, not closed by the stream). */
    unsigned char Open(ICFile* file);

    virtual unsigned char IsOpenInternal();
    virtual unsigned int AvailableInternal();
    virtual void SkipInternal(unsigned int size);
    virtual void ReadInternal(unsigned char* buffer, unsigned int size);
    virtual void ResetInternal();
    virtual void MarkInternal();

    ICFile* m_file;                 // 0x1c
    unsigned char m_ownsFile;       // 0x20 opened by path: delete the file on Close()
    unsigned int m_markPosition;    // 0x24 file position saved by MarkInternal()
};

} } } }

#endif
