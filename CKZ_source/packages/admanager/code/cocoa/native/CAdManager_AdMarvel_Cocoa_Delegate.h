// CAdManager_AdMarvel_Cocoa_Delegate.h - Objective-C side of the AdMarvel integration: owns the banner
// and interstitial AdMarvelViews, acts as their AdMarvelDelegate and runs the ad state machine.
#import <UIKit/UIKit.h>

#import "AdMarvelView.h"

@class AdManager_Reachability;
@class AppDelegate;
@class AdMarvelRootController;

/** Ad state (m_adState); -1 before the first setAdState:. */
enum {
    AdState_None = -1,
    AdState_Hidden = 1,
    AdState_Banner = 2,
    AdState_Interstitial = 3
};

/** Value of the retry timers while no retry is pending. */
#define ADMANAGER_TIMER_IDLE 0xfacade

@interface CAdManager_AdMarvel_Cocoa_Delegate : NSObject <AdMarvelDelegate>
{
    UIWindow* window;                                   // 0x04 window of the application delegate
    AppDelegate* appDelegate;                           // 0x08 [[UIApplication sharedApplication] delegate] (b142: ivar type encoding @"AppDelegate")
    AdMarvelRootController* gluAdvertBannerViewController;  // 0x0c own host controller (nil when the app has one) (b142: type encoding @"AdMarvelRootController")
    AdMarvelView* adMarvelInterstitialView;             // 0x10
    AdMarvelView* gluAdvertBannerView;                  // 0x14
    unsigned char m_fetchingBannerAd;                   // 0x18 siteId asked for a banner request
    unsigned char m_fetchingInterstitialAd;             // 0x19 siteId asked for an interstitial request
    int m_adState;                                      // 0x1c AdState_*
    int m_prevAnchor;                                   // 0x20 ICAdManager::Position of the banner
    int m_x;                                            // 0x24 explicit banner position, -1 = use anchor
    int m_y;                                            // 0x28
    int m_siteId;                                       // 0x2c AdMarvel site id (phone or tablet)
    int m_bannerWidth;                                  // 0x30
    int m_bannerHeight;                                 // 0x34
    int m_prevOrientation;                              // 0x38 device orientation of the last layout
    int m_webBrowserDelayTimer;                         // 0x3c ms until the sound engine comes back
    int m_retryBannerAdTimer;                           // 0x40 ms until a failed banner is retried
    int m_retryInterstitialTimer;                       // 0x44 ms until a failed interstitial is retried
    int m_adRotationTimeInterval;                       // 0x48 ms between banner refreshes
    int m_adRotationTimer;                              // 0x4c
    int m_adInterstitialSanityTimer;                    // 0x50 ms before giving up on an interstitial
    int m_webBrowserSanityTimer;                        // 0x54
    unsigned char m_interstitialDisplayedSuccessfully;  // 0x58
    unsigned char m_interstitialFinished;               // 0x59
    unsigned char m_bannerAdVisible;                    // 0x5a
    unsigned char m_bannerAdReady;                      // 0x5b
    unsigned char m_focusOnInternalBrowser;             // 0x5c the SDK's web browser is in front
    unsigned char m_bannerAdHasAppeared;                // 0x5d
    AdManager_Reachability* internetReachability;       // 0x60
}

/** Creates the views for the given phone / tablet AdMarvel site ids and starts fetching. */
- (id) init: (int) phoneSiteId : (int) tabletSiteId;

- (void) destroySoundEngine;
- (void) restoreSoundEngine;
/** Lays the banner out for the current anchor, screen size and device orientation. */
- (void) setAdBounds;

- (int) getAdState;
/** Current state; with checkBannerReady a banner that has no ad yet reports AdState_Hidden. */
- (int) getAdState: (unsigned char) checkBannerReady;
- (void) setAdState: (int) state;
- (void) setAdState: (int) state : (int) anchor;
- (void) setAdState: (int) state : (int) anchor : (int) x : (int) y;
/** Advances the timers by deltaMilliseconds. */
- (void) tickAds: (int) deltaMilliseconds;

- (unsigned char) adWebBrowserIsOpen;
- (unsigned char) isInterstitialFinished;
- (unsigned char) didInterstitialDisplaySuccessfully;
- (unsigned char) areAdsInFullscreenMode;
- (int) getAdBannerHeight;

- (void) hideBannerAd;
- (void) showBannerAd;
- (void) fetchBannerAd;
- (void) fetchInterstitialAd;
- (void) hideInterstitialAd;
- (void) showInterstitialAd;
- (void) destroyInterstitialAdView;
- (void) createInterstitialAdView;
- (BOOL) areAdsInFullscreenModeAdMarvel;

/** AdManager_kNetworkReachabilityChangedNotification handler. */
- (void) checkInternetReachability: (NSNotification*) notification;

@end
