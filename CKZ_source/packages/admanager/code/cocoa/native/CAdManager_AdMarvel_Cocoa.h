// CAdManager_AdMarvel_Cocoa.h - C++ side of the AdMarvel integration; owns the Objective-C delegate
// (CAdManager_AdMarvel_Cocoa_Delegate) that talks to the AdMarvel SDK.
#ifndef CADMANAGER_ADMARVEL_COCOA_H
#define CADMANAGER_ADMARVEL_COCOA_H

#include "ICAdManager.h"

#ifdef __OBJC__
@class CAdManager_AdMarvel_Cocoa_Delegate;
typedef CAdManager_AdMarvel_Cocoa_Delegate* AdMarvelDelegateRef;
#else
typedef void* AdMarvelDelegateRef;
#endif

class CAdManager_AdMarvel_Cocoa : public com::glu::platform::components::CSingleton {
public:
    /** Singleton key (the hashed name is not known; value taken from the original constructor). */
    static const unsigned int ClassId = 0x3266cd97;

    /** Finds the registered instance or creates it. */
    static inline CAdManager_AdMarvel_Cocoa* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CAdManager_AdMarvel_Cocoa* instance = (CAdManager_AdMarvel_Cocoa*)found;
        if (instance == NULL) {
            instance = new CAdManager_AdMarvel_Cocoa();
        }
        return instance;
    }

    CAdManager_AdMarvel_Cocoa();
    virtual ~CAdManager_AdMarvel_Cocoa();

    /** Creates the delegate (and with it the banner view) for the phone / tablet site id. */
    void Init(int phoneSiteId, int tabletSiteId);
    void Destroy();
    void ShowAd(ICAdManager::Position position, int x, int y, ICAdManager::Callback callback, void* context);
    unsigned char IsAvailable();
    unsigned char IsVisible();
    unsigned char IsFullScreen();
    void HideAd();
    void HandleUpdate(int deltaMilliseconds);
    void HandleRender();

private:
    int m_unused08;                    // 0x08 never touched by this package
    AdMarvelDelegateRef m_delegate;    // 0x0c set by Init(), not initialized by the constructor
};

#endif
