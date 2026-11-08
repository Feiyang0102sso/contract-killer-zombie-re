// CAppInfo.h - application identity: name, variant, bundle id, build tag and version (singleton).
#ifndef CAPPINFO_H
#define CAPPINFO_H

#include "CSingleton.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform { namespace components {

class CAppInfo : public CSingleton {
public:
    enum { SingletonId = 0x7674f803 };

    /** The application's instance, created on first use. */
    static CAppInfo* GetInstance();

    CAppInfo();

    virtual inline ~CAppInfo();

    int GetVersionMajor() const;
    int GetVersionMinor() const;
    int GetVersionMicro() const;

    /** Fill bundle id and version from the platform manifest (Info.plist). */
    void CacheManifestInfo();

    CStrWChar m_name;               // 0x08 APP_GLU_NAME_STR
    CStrWChar m_variant;            // 0x18 APP_GLU_VARIANT_STR
    CStrWChar m_bundleIdentifier;   // 0x28 from the manifest
    CStrWChar m_buildTag;           // 0x38 from the "$Name: ... $" build tag
    int m_versionMajor;             // 0x48
    int m_versionMinor;             // 0x4c
    int m_versionMicro;             // 0x50
};

inline CAppInfo::~CAppInfo()
{
}

} } } }

#endif
