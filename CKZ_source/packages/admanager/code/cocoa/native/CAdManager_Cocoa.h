// CAdManager_Cocoa.h - iOS ICAdManager: forwards every call to the AdMarvel singleton.
#ifndef CADMANAGER_COCOA_H
#define CADMANAGER_COCOA_H

#include "ICAdManager.h"

class CAdManager_AdMarvel_Cocoa;

class CAdManager_Cocoa : public ICAdManager {
public:
    /** m_adMarvel is only set by Init() (the original constructor leaves it untouched). */
    inline CAdManager_Cocoa()
    {
    }

    virtual ~CAdManager_Cocoa();

    virtual void Init(int phoneSiteId, int tabletSiteId);
    virtual void Destroy();
    virtual void ShowAd(Type type, Position position, int x, int y, Callback callback, void* context);
    virtual unsigned char IsAvailable(Type type);
    virtual unsigned char IsVisible();
    virtual unsigned char IsFullScreen();
    virtual void HideAd();
    virtual void HandleUpdate(int deltaMilliseconds);
    virtual void HandleRender();

private:
    CAdManager_AdMarvel_Cocoa* m_adMarvel;  // 0x08
};

#endif
