// ICAdManager.h - platform independent advertising interface (one singleton per application).
// The iOS implementation is CAdManager_Cocoa (AdMarvel banners and interstitials).
#ifndef ICADMANAGER_H
#define ICADMANAGER_H

#include "CSingleton.h"

class ICAdManager : public com::glu::platform::components::CSingleton {
public:
    /** Singleton key: CStringToKey("ICAdManager"). */
    static const unsigned int ClassId = 0x0b788c55;

    /** Ad provider / kind; only the AdMarvel banner (0) is implemented on iOS. */
    enum Type {
        Type_AdMarvel = 0
    };

    /** Screen anchor of the banner; FullScreen shows an interstitial instead. */
    enum Position {
        Position_TopLeft = 0,
        Position_TopCenter = 1,
        Position_TopRight = 2,
        Position_MiddleLeft = 3,
        Position_Center = 4,
        Position_MiddleRight = 5,
        Position_BottomLeft = 6,
        Position_BottomCenter = 7,
        Position_BottomRight = 8,
        Position_FullScreen = 9
    };

    /** Called by the ad manager with an event id and the caller's context; returns handled. */
    typedef unsigned char (*Callback)(int event, void* context);

    /** Finds the registered instance or creates the platform implementation. */
    static ICAdManager* GetInstance();

    inline ICAdManager()
        : CSingleton(ClassId)
    {
    }

    inline virtual ~ICAdManager()
    {
    }

    /** Starts the ad SDK; the two ids are the phone and tablet site ids. */
    virtual void Init(int phoneSiteId, int tabletSiteId) = 0;
    virtual void Destroy() = 0;
    /** Shows an ad of `type` at `position`; x / y override the anchor when not -1. */
    virtual void ShowAd(Type type, Position position, int x, int y, Callback callback, void* context) = 0;
    virtual unsigned char IsAvailable(Type type) = 0;
    virtual unsigned char IsVisible() = 0;
    /** True while an ad covers the screen (internal browser or full screen ad). */
    virtual unsigned char IsFullScreen() = 0;
    virtual void HideAd() = 0;
    virtual void HandleUpdate(int deltaMilliseconds) = 0;
    virtual void HandleRender() = 0;
};

#endif
