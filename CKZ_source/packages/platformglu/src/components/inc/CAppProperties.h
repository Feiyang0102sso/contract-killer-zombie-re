// CAppProperties.h - application settings singleton (created by b000x, header-only).
// Wraps the CProperties loaded from the properties file and remembers which keys a later
// merge overrode. The destructor is the key function and is defined inline below, so the
// vtable and destructors are emitted (weak) in every file that includes this header.
#ifndef CAPPPROPERTIES_H
#define CAPPPROPERTIES_H

#include "CSingleton.h"
#include "CProperties.h"
#include "CStrChar.h"
#include "CVector.h"

namespace com { namespace glu { namespace platform { namespace components {

class CAppProperties : public CSingleton {
public:
    enum { SingletonId = 0x8c535a57 };

    // CDemoMgr::Read/GetDemoString and CUtility::RegisterUISoftkeys inline the full constructor.
    inline CAppProperties() : CSingleton(SingletonId), m_overriddenKeys(0, 0)
    {
    }

    virtual ~CAppProperties();

    CProperties m_properties;                       // 0x08
    core::CVector<CStrChar> m_overriddenKeys;       // 0x2c keys replaced by merged property files
};

inline CAppProperties::~CAppProperties()
{
    m_overriddenKeys.Clear();
}

} } } }

#endif
