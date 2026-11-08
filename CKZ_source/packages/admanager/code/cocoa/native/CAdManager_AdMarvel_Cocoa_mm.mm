// CAdManager_AdMarvel_Cocoa_mm.mm - CAdManager_AdMarvel_Cocoa methods that message the Objective-C delegate.
#include "CAdManager_AdMarvel_Cocoa.h"

#import "CAdManager_AdMarvel_Cocoa_Delegate.h"

void CAdManager_AdMarvel_Cocoa::Init(int phoneSiteId, int tabletSiteId)
{
    m_delegate = [[CAdManager_AdMarvel_Cocoa_Delegate alloc] init:phoneSiteId :tabletSiteId];
}

void CAdManager_AdMarvel_Cocoa::Destroy()
{
    if (m_delegate != nil) {
        // The original sends dealloc directly instead of release
        [m_delegate dealloc];
        m_delegate = nil;
    }
}

void CAdManager_AdMarvel_Cocoa::ShowAd(ICAdManager::Position position, int x, int y,
                                       ICAdManager::Callback callback, void* context)
{
    if (m_delegate != nil) {
        // ?: kept: the original selects the state with a conditional move
        int state = (position == ICAdManager::Position_FullScreen) ? AdState_Interstitial : AdState_Banner;
        [m_delegate setAdState:state :position :x :y];
    }
}

unsigned char CAdManager_AdMarvel_Cocoa::IsAvailable()
{
    return 1;
}

unsigned char CAdManager_AdMarvel_Cocoa::IsVisible()
{
    unsigned char visible = 0;
    if (m_delegate != nil) {
        visible = [m_delegate getAdState:1] != AdState_Hidden;
    }
    return visible;
}

unsigned char CAdManager_AdMarvel_Cocoa::IsFullScreen()
{
    if (m_delegate != nil) {
        return [m_delegate adWebBrowserIsOpen] || [m_delegate areAdsInFullscreenMode];
    }
    return 0;
}

void CAdManager_AdMarvel_Cocoa::HideAd()
{
    if (m_delegate != nil) {
        [m_delegate setAdState:AdState_Hidden];
    }
}

void CAdManager_AdMarvel_Cocoa::HandleUpdate(int deltaMilliseconds)
{
    if (m_delegate != nil) {
        [m_delegate tickAds:deltaMilliseconds];
    }
}
