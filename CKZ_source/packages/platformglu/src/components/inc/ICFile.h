// ICFile.h - platform file handle interface (created by b005, header-only).
#ifndef ICFILE_H
#define ICFILE_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class ICFile : public core::CClass {
public:
    enum eFileSeekMode {
        SeekSet = 0,        // from the start
        SeekCurrent = 1,    // from the current position
        SeekEnd = 2         // from the end
    };

    virtual ~ICFile() {}

    virtual unsigned char IsOpen() = 0;
    /** Move the position; false on failure. */
    virtual unsigned char Seek(int offset, eFileSeekMode mode) = 0;
    virtual unsigned int Tell() = 0;
    /** Returns the bytes actually read. */
    virtual unsigned int Read(unsigned char* buffer, unsigned int size) = 0;
    /** Returns the bytes actually written. */
    virtual unsigned int Write(const unsigned char* data, unsigned int size) = 0;
};

} } } }

#endif
