// ICLicenseMgr.h - license / activation service (singleton interface). On iOS every query reports
// "no license information" (CLicenseMgr_Cocoa); the App Store handles licensing.
#ifndef ICLICENSEMGR_H
#define ICLICENSEMGR_H

#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace components {

class ICLicenseMgr : public CSingleton {
public:
    /** Singleton id in CApp::m_singletons. */
    enum { SingletonId = 0x539dccad };

    ICLicenseMgr() : CSingleton(SingletonId) {}

    /** Forget the cached instance in CApp. */
    inline virtual ~ICLicenseMgr()
    {
        if (CApplet::m_pApp != NULL) {
            CApplet::m_pApp->m_cached38 = NULL;
        }
    }

    /** Platform factory (CLicenseMgr_Cocoa). */
    static ICLicenseMgr* CreateInstance();

    /** The application's license manager: cached in CApp, created on first use. */
    static inline ICLicenseMgr* GetInstance()
    {
        framework::CApp* app = CApplet::m_pApp;
        ICLicenseMgr* instance;
        if (app != NULL) {
            instance = (ICLicenseMgr*)app->m_cached38;
            if (instance == NULL) {
                ICLicenseMgr* found = NULL;
                app->m_singletons->Find(SingletonId, (void**)&found);
                instance = found;
                if (instance == NULL) {
                    instance = CreateInstance();
                }
                app->m_cached38 = instance;
            }
        } else {
            instance = NULL;
        }
        return instance;
    }

    virtual void ReadLicenseInfo() = 0;
    virtual int GetLicenseType() = 0;
    virtual int GetRemainingTimeSeconds() = 0;
    virtual int GetRemainingUsageCount() = 0;
    virtual unsigned char RequestActivation() = 0;
    virtual unsigned char IsActivating() = 0;
};

} } } }

#endif
