// CDisplayProgram_OGLES.h - display program executed by the OpenGL ES device.
// (first drafted by b002 as PROVISIONAL; completed by b037x, the owner)
#ifndef CDISPLAYPROGRAM_OGLES_H
#define CDISPLAYPROGRAM_OGLES_H

#include "CDisplayProgram.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CDisplayProgram_OGLES : public CDisplayProgram {
public:
    CDisplayProgram_OGLES() {}

    virtual ~CDisplayProgram_OGLES();

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const;
    virtual void Destroy();
    virtual unsigned char GetDescription(Description& description);
    virtual void ConfigDefaultAllocationInfo();

    unsigned int m_deviceVersion;   // 0xb4 GL version of the device that recorded the program
};

} } } }

#endif
