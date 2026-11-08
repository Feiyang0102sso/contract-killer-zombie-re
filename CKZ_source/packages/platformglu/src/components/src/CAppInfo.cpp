// CAppInfo.cpp - application identity.
#include "CAppInfo.h"
#include "CStrChar.h"
#include "AppGluStrings.h"

namespace com { namespace glu { namespace platform { namespace components {

CAppInfo* CAppInfo::GetInstance()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
    CAppInfo* instance = (CAppInfo*)found;
    if (instance == NULL) {
        instance = new CAppInfo();
    }
    return instance;
}

CAppInfo::CAppInfo() : CSingleton(SingletonId)
{
    m_versionMajor = 0;
    m_versionMinor = 0;
    m_versionMicro = 0;
    CacheManifestInfo();
    m_name = APP_GLU_NAME_STR;
    m_variant = APP_GLU_VARIANT_STR;

    // the build tag looks like "$Name: TAG $": keep the trimmed TAG
    CStrChar buildTag(APP_GLU_BUILDTAG_STR);
    char* colon = strchr(buildTag.m_string, ':');
    if (colon != NULL) {
        char* dollar = strrchr(colon + 1, '$');
        if (dollar != NULL) {
            *dollar = 0;
            m_buildTag = CStrWChar(colon + 1).Trim();
        }
    }
}

int CAppInfo::GetVersionMajor() const
{
    return m_versionMajor;
}

int CAppInfo::GetVersionMinor() const
{
    return m_versionMinor;
}

int CAppInfo::GetVersionMicro() const
{
    return m_versionMicro;
}

} } } }
