// CAdManager_AdMarvel_Cocoa_Delegate.mm - AdMarvel banner / interstitial state machine and SDK delegate.
#import "CAdManager_AdMarvel_Cocoa_Delegate.h"

#import "AdManager_Reachability.h"
#import "AdMarvelRootController.h"

#include "CApplet.h"
#include "ICCore.h"
#include "ICAdManager.h"

// The original N_SOL records these four headers; the ObjC++ copy keeps the SjLj destructor cleanup.
#include "ICMediaPlayer.h"
#include "CColor.h"
#include "CStrChar.h"
#include "CRenderSurfaceBuffer.h"

#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>

// the MAC address buffer below comes straight from the C library
#undef malloc
#undef free

using namespace com::glu::platform::framework;

// ---- constants ----
static const int kPhoneBannerWidth = 320;
static const int kPhoneBannerHeight = 50;
static const int kTabletBannerWidth = 768;
static const int kTabletBannerHeight = 90;
static const int kAdRotationIntervalSlow = 30000;       // ms, cellular or no network
static const int kAdRotationIntervalWiFi = 15000;       // ms
static const int kInterstitialSanityTime = 3000;        // ms
static const int kWebBrowserSanityTime = 3000;          // ms
static const int kWebBrowserDelayTime = 1000;           // ms
static const int kRetryTime = 15000;                    // ms

@implementation CAdManager_AdMarvel_Cocoa_Delegate

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self destroyInterstitialAdView];
    [gluAdvertBannerView setDelegate:nil];
    [gluAdvertBannerView release];
    [gluAdvertBannerViewController release];
    [super dealloc];
}

- (id) init: (int) phoneSiteId : (int) tabletSiteId
{
    self = [super init];
    if (self != nil) {
        appDelegate = (AppDelegate*)[[UIApplication sharedApplication] delegate];
        window = [appDelegate window];

        // iPad (either orientation) gets the leaderboard sized banner
        CGRect screenBounds = [[UIScreen mainScreen] bounds];
        if ((screenBounds.size.width == 768 && screenBounds.size.height == 1024) ||
            (screenBounds.size.width == 1024 && screenBounds.size.height == 768)) {
            m_bannerWidth = kTabletBannerWidth;
            m_bannerHeight = kTabletBannerHeight;
            m_siteId = tabletSiteId;
        } else {
            m_bannerWidth = kPhoneBannerWidth;
            m_bannerHeight = kPhoneBannerHeight;
            m_siteId = phoneSiteId;
        }

        m_retryBannerAdTimer = ADMANAGER_TIMER_IDLE;
        m_retryInterstitialTimer = ADMANAGER_TIMER_IDLE;
        m_adState = AdState_None;
        m_prevAnchor = ICAdManager::Position_BottomCenter;
        m_x = -1;
        m_y = -1;
        m_bannerAdVisible = 0;
        m_focusOnInternalBrowser = 0;
        m_bannerAdHasAppeared = 0;
        m_webBrowserDelayTimer = 0;
        m_webBrowserSanityTimer = 0;
        gluAdvertBannerViewController = nil;
        gluAdvertBannerView = nil;

        if ([[[UIApplication sharedApplication] delegate] respondsToSelector:@selector(viewController)]) {
            // The application has its own view controller (see applicationUIViewController)
            gluAdvertBannerView = [[AdMarvelView createAdMarvelViewWithDelegate:self prefetchAd:YES] retain];
        } else {
            gluAdvertBannerViewController = [[AdMarvelRootController alloc] init];
            gluAdvertBannerView = [[AdMarvelView createAdMarvelViewWithDelegate:self prefetchAd:YES] retain];
            [gluAdvertBannerViewController setView:gluAdvertBannerView];
        }
        [gluAdvertBannerView setAutoresizingMask:UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin];

        if (gluAdvertBannerViewController != nil) {
            [window addSubview:[gluAdvertBannerViewController view]];
        } else {
            [[[[[UIApplication sharedApplication] keyWindow] subviews] objectAtIndex:0] addSubview:gluAdvertBannerView];
        }
        [gluAdvertBannerView setHidden:YES];

        [self createInterstitialAdView];
        [self setAdState:AdState_Hidden];
        [self fetchBannerAd];
        m_adRotationTimeInterval = kAdRotationIntervalSlow;
        m_adRotationTimer = m_adRotationTimeInterval;
        [self fetchInterstitialAd];
        m_adInterstitialSanityTimer = kInterstitialSanityTime;
        m_interstitialDisplayedSuccessfully = 0;
        m_interstitialFinished = 0;
        [self showBannerAd];
        [self hideBannerAd];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(checkInternetReachability:)
                                                     name:AdManager_kNetworkReachabilityChangedNotification object:nil];
        internetReachability = [[AdManager_Reachability reachabilityForInternetConnection] retain];
        [internetReachability startNotifier];
        [self checkInternetReachability:nil];
    }
    return self;
}

- (void) destroySoundEngine
{
}

- (void) restoreSoundEngine
{
}

- (void) setAdBounds
{
    ICCore* core = CApplet::m_pCore;
    ICCore::eDeviceOrientation orientation = core->GetDeviceOrientation();

    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    unsigned int screenWidth = screenBounds.size.width;
    unsigned int screenHeight = screenBounds.size.height;

    bool autoRotation = false;
    bool landscape = false;
    if (core->AutoRotationIsActive()) {
        autoRotation = true;
        if (core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_LandscapeRight) ||
            core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_LandscapeLeft)) {
            landscape = true;
        }
    } else if (orientation == ICCore::DeviceOrientation_LandscapeRight ||
               orientation == ICCore::DeviceOrientation_LandscapeLeft) {
        landscape = true;
    }

    // Lay out in landscape terms when the screen is reported in portrait
    if (landscape && screenHeight > screenWidth) {
        unsigned int swap = screenWidth;
        screenWidth = screenHeight;
        screenHeight = swap;
    }

    // No explicit position: derive it from the anchor
    int x = m_x;
    int y = m_y;
    if (x == -1 && y == -1) {
        switch (m_prevAnchor) {
        case ICAdManager::Position_TopLeft:
            x = 0;
            y = 0;
            break;
        case ICAdManager::Position_TopCenter:
            y = 0;
            x = (screenWidth - m_bannerWidth) / 2;
            break;
        case ICAdManager::Position_TopRight:
            y = 0;
            x = screenWidth - m_bannerWidth;
            break;
        case ICAdManager::Position_MiddleLeft:
            x = 0;
            y = (screenHeight - m_bannerHeight) / 2;
            break;
        case ICAdManager::Position_Center:
            y = (screenHeight - m_bannerHeight) / 2;
            x = (screenWidth - m_bannerWidth) / 2;
            break;
        case ICAdManager::Position_MiddleRight:
            x = screenWidth - m_bannerWidth;
            y = (screenHeight - m_bannerHeight) / 2;
            break;
        case ICAdManager::Position_BottomLeft:
            x = 0;
            y = screenHeight - m_bannerHeight;
            break;
        case ICAdManager::Position_BottomCenter:
            y = screenHeight - m_bannerHeight;
            x = (screenWidth - m_bannerWidth) / 2;
            break;
        case ICAdManager::Position_BottomRight:
            y = screenHeight - m_bannerHeight;
            x = screenWidth - m_bannerWidth;
            break;
        case ICAdManager::Position_FullScreen:
            x = 0;
            y = 0;
            break;
        }
    }

    // Without auto-rotation the banner is rotated by hand to follow the device
    CGAffineTransform transform = CGAffineTransformMakeRotation(0);
    bool rotatedSideways = false;
    if (!autoRotation) {
        switch (orientation) {
        case ICCore::DeviceOrientation_Portrait:
            break;
        case ICCore::DeviceOrientation_LandscapeRight: {
            transform = CGAffineTransformMakeRotation(3 * M_PI_2);
            int rotatedY = screenHeight - x - m_bannerWidth;
            x = y;
            y = rotatedY;
            rotatedSideways = true;
            break;
        }
        case ICCore::DeviceOrientation_PortraitUpsideDown:
            transform = CGAffineTransformMakeRotation(M_PI);
            x = screenWidth - x - m_bannerWidth;
            y = screenHeight - y - m_bannerHeight;
            break;
        case ICCore::DeviceOrientation_LandscapeLeft: {
            transform = CGAffineTransformMakeRotation(M_PI_2);
            int rotatedX = screenWidth - y - m_bannerHeight;
            y = x;
            x = rotatedX;
            rotatedSideways = true;
            break;
        }
        default:
            rotatedSideways = true;
            break;
        }
    }
    if (rotatedSideways) {
        // The banner is turned by 90 degrees: width and height swap
        [gluAdvertBannerView setFrame:CGRectMake(x, y, m_bannerHeight, m_bannerWidth)];
    } else {
        [gluAdvertBannerView setFrame:CGRectMake(x, y, m_bannerWidth, m_bannerHeight)];
    }
    [gluAdvertBannerView setTransform:transform];
    m_prevOrientation = orientation;
}

- (int) getAdState
{
    return [self getAdState:0];
}

- (int) getAdState: (unsigned char) checkBannerReady
{
    // A banner without a loaded ad is not on screen yet
    if (checkBannerReady && m_adState == AdState_Banner) {
        if (!m_bannerAdReady) {
            return AdState_Hidden;
        }
    }
    return m_adState;
}

- (void) setAdState: (int) state
{
    [self setAdState:state :m_prevAnchor];
}

- (void) setAdState: (int) state : (int) anchor
{
    // The original ignores `anchor` and keeps the previous one
    [self setAdState:state :m_prevAnchor :-1 :-1];
}

- (void) setAdState: (int) state : (int) anchor : (int) x : (int) y
{
    m_adState = state;
    m_prevAnchor = anchor;
    m_x = x;
    m_y = y;
    switch (state) {
    case AdState_Hidden:
        if (m_bannerAdVisible) {
            [self hideBannerAd];
        }
        m_bannerAdVisible = 0;
        break;
    case AdState_Banner:
        if (m_bannerAdReady) {
            m_bannerAdVisible = 1;
            m_bannerAdHasAppeared = 1;
            [self showBannerAd];
        }
        break;
    case AdState_Interstitial:
        [self showInterstitialAd];
        break;
    }
}

- (void) tickAds: (int) deltaMilliseconds
{
    if (m_adState == AdState_Hidden) {
        return;
    }

    [self setAdBounds];

    if (m_webBrowserDelayTimer > 0) {
        m_webBrowserDelayTimer -= deltaMilliseconds;
        if (m_webBrowserDelayTimer <= 0) {
            [self restoreSoundEngine];
        }
    }

    if (m_adState == AdState_Banner) {
        // Rotate the banner periodically
        if (m_bannerAdHasAppeared) {
            m_adRotationTimer -= deltaMilliseconds;
            if (m_adRotationTimer <= 0 && m_bannerAdReady) {
                [self fetchBannerAd];
                m_adRotationTimer = m_adRotationTimeInterval;
            }
        }
    } else if (m_adState == AdState_Interstitial) {
        // Give up when the interstitial never shows
        if (!m_interstitialDisplayedSuccessfully) {
            m_adInterstitialSanityTimer -= deltaMilliseconds;
            if (m_adInterstitialSanityTimer <= 0) {
                [self setAdState:AdState_Hidden];
                m_interstitialFinished = 1;
            }
        }
    }

    if (m_retryBannerAdTimer != ADMANAGER_TIMER_IDLE) {
        m_retryBannerAdTimer -= deltaMilliseconds;
        if (m_retryBannerAdTimer <= 0) {
            m_retryBannerAdTimer = ADMANAGER_TIMER_IDLE;
            [self fetchBannerAd];
        }
    }

    if (m_retryInterstitialTimer != ADMANAGER_TIMER_IDLE) {
        m_retryInterstitialTimer -= deltaMilliseconds;
        if (m_retryInterstitialTimer <= 0) {
            m_retryInterstitialTimer = ADMANAGER_TIMER_IDLE;
            [self fetchInterstitialAd];
        }
    }
}

- (unsigned char) adWebBrowserIsOpen
{
    if (m_adState == AdState_Hidden) {
        return 0;
    }
    if (m_focusOnInternalBrowser) {
        return 1;
    }
    return m_webBrowserDelayTimer > 0;
}

- (unsigned char) isInterstitialFinished
{
    if (m_adState == AdState_Hidden) {
        return 1;
    }
    return m_interstitialFinished;
}

- (unsigned char) didInterstitialDisplaySuccessfully
{
    if (m_adState == AdState_Hidden) {
        return 1;
    }
    return m_interstitialDisplayedSuccessfully;
}

- (unsigned char) areAdsInFullscreenMode
{
    if (m_adState == AdState_Hidden) {
        return 0;
    }
    return [self areAdsInFullscreenModeAdMarvel];
}

- (int) getAdBannerHeight
{
    return m_bannerHeight;
}

- (void) hideBannerAd
{
    if (gluAdvertBannerView != nil) {
        [gluAdvertBannerView setHidden:YES];
    }
}

- (void) showBannerAd
{
    if (gluAdvertBannerView != nil) {
        [gluAdvertBannerView setHidden:NO];
    }
}

- (void) fetchBannerAd
{
    m_fetchingBannerAd = 1;
    [gluAdvertBannerView getAdWithNotification];
}

- (void) fetchInterstitialAd
{
    m_fetchingInterstitialAd = 1;
    if ([adMarvelInterstitialView isInterstitialReady]) {
        [adMarvelInterstitialView displayInterstitial];
    } else {
        [adMarvelInterstitialView getInterstitialAd];
    }
}

- (void) hideInterstitialAd
{
}

- (void) showInterstitialAd
{
    [adMarvelInterstitialView displayInterstitial];
}

- (void) destroyInterstitialAdView
{
    if (adMarvelInterstitialView != nil) {
        [adMarvelInterstitialView release];
        adMarvelInterstitialView = nil;
    }
}

- (void) createInterstitialAdView
{
    adMarvelInterstitialView = [[AdMarvelView createAdMarvelViewWithDelegate:self] retain];
}

- (BOOL) areAdsInFullscreenModeAdMarvel
{
    return [gluAdvertBannerView isInFullScreenMode];
}

#pragma mark AdMarvelDelegate

- (NSString*) partnerId
{
    return @"a1434e57627654cf";
}

- (NSString*) siteId
{
    // The SDK asks once per request; interstitials use a fixed site
    if (m_fetchingInterstitialAd) {
        m_fetchingInterstitialAd = 0;
        return @"3102";
    }
    m_fetchingBannerAd = 0;
    return [NSString stringWithFormat:@"%d", m_siteId];
}

- (UIViewController*) applicationUIViewController
{
    id applicationDelegate = [[UIApplication sharedApplication] delegate];
    if ([applicationDelegate respondsToSelector:@selector(viewController)]) {
        UIViewController* viewController = [applicationDelegate viewController];
        if (viewController != nil) {
            return viewController;
        }
    }
    return gluAdvertBannerViewController;
}

- (NSDictionary*) targetingParameters
{
    NSMutableDictionary* parameters = [[[NSMutableDictionary alloc] init] autorelease];
    [parameters setObject:@"1.0" forKey:@"APP_VERSION"];
    [parameters setObject:@"BEE MAZE FUN GAME" forKey:@"KEYWORDS"];

    // Device id: UDID when available, otherwise a stored id (MAC address or a fresh UUID)
    NSString* deviceId = nil;
    if ([[UIDevice currentDevice] respondsToSelector:@selector(uniqueIdentifier)]) {
        deviceId = [[UIDevice currentDevice] uniqueIdentifier];
    }
    if (deviceId == nil) {
        NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
        NSString* userIdKey = [[NSString alloc] initWithCString:"com.glu.userid" encoding:NSASCIIStringEncoding];
        deviceId = [defaults stringForKey:userIdKey];
        if (deviceId == nil) {
            // MAC address of en0
            int mib[6];
            size_t length;
            mib[0] = CTL_NET;
            mib[1] = AF_ROUTE;
            mib[2] = 0;
            mib[3] = AF_LINK;
            mib[4] = NET_RT_IFLIST;
            mib[5] = if_nametoindex("en0");
            if (mib[5] != 0) {
                if (sysctl(mib, 6, NULL, &length, NULL, 0) >= 0) {
                    char* buffer = (char*)malloc(length);
                    if (buffer != NULL) {
                        if (sysctl(mib, 6, buffer, &length, NULL, 0) >= 0) {
                            struct if_msghdr* interfaceMessage = (struct if_msghdr*)buffer;
                            struct sockaddr_dl* linkAddress = (struct sockaddr_dl*)(interfaceMessage + 1);
                            unsigned char* macAddress = (unsigned char*)LLADDR(linkAddress);
                            NSString* macString = [[NSString alloc] initWithFormat:@"%02X%02X%02X%02X%02X%02X",
                                                   macAddress[0], macAddress[1], macAddress[2],
                                                   macAddress[3], macAddress[4], macAddress[5]];
                            [defaults setObject:[macString lowercaseString] forKey:userIdKey];
                            [macString release];
                            deviceId = [defaults stringForKey:userIdKey];
                        }
                        free(buffer);
                    }
                }
            }
            if (deviceId == nil) {
                CFUUIDRef uuid = CFUUIDCreate(NULL);
                CFStringRef uuidString = CFUUIDCreateString(NULL, uuid);
                [defaults setObject:(NSString*)uuidString forKey:userIdKey];
                CFRelease(uuidString);
                CFRelease(uuid);
                deviceId = [defaults stringForKey:userIdKey];
            }
        }
        [userIdKey release];
    }
    if (deviceId != nil) {
        [parameters setObject:[deviceId lowercaseString] forKey:[NSString stringWithUTF8String:"IMEI_ID"]];
    }
    return parameters;
}

- (void) fullScreenWebViewActivated
{
    [self hideBannerAd];
    m_interstitialDisplayedSuccessfully = 1;
    m_focusOnInternalBrowser = 1;
    m_webBrowserSanityTimer = kWebBrowserSanityTime;
    [self destroySoundEngine];
}

- (void) fullScreenWebViewClosed
{
    if (m_adState == AdState_Banner) {
        [self showBannerAd];
    }
    m_interstitialFinished = 1;
    if (m_focusOnInternalBrowser) {
        m_webBrowserDelayTimer = kWebBrowserDelayTime;
    }
    m_focusOnInternalBrowser = 0;
}

- (void) getAdSucceeded
{
    if (m_adState == AdState_Banner) {
        m_bannerAdVisible = 1;
        m_bannerAdHasAppeared = 1;
        [self showBannerAd];
    }
    m_bannerAdReady = 1;
}

- (void) getAdFailed
{
    m_retryBannerAdTimer = kRetryTime;
}

- (void) handleAdMarvelSDKClick: (NSString*) urlString
{
}

- (void) adDidExpand
{
    m_focusOnInternalBrowser = 1;
}

- (void) adDidCollapse
{
    m_focusOnInternalBrowser = 0;
}

- (void) getInterstitialAdSucceeded
{
    if (m_adState == AdState_Interstitial) {
        [adMarvelInterstitialView displayInterstitial];
    }
}

- (void) getInterstitialAdFailed
{
    m_retryInterstitialTimer = kRetryTime;
}

- (void) interstitialActivated
{
}

- (void) interstitialClosed
{
    int state = [self getAdState];
    m_adInterstitialSanityTimer = kInterstitialSanityTime;
    m_interstitialDisplayedSuccessfully = 0;
    m_interstitialFinished = 0;
    [self fetchInterstitialAd];
    // Back to hidden once the interstitial is gone
    if (state == AdState_Interstitial) {
        state = AdState_Hidden;
    }
    [self setAdState:state];
}

- (CGRect) adMarvelViewFrame
{
    return CGRectMake(0, 0, m_bannerWidth, m_bannerHeight);
}

- (void) checkInternetReachability: (NSNotification*) notification
{
    // Refresh banners less often without Wi-Fi
    switch ([internetReachability currentReachabilityStatus]) {
    case AdManager_NotReachable:
        m_adRotationTimeInterval = kAdRotationIntervalSlow;
        break;
    case AdManager_ReachableViaWiFi:
        m_adRotationTimeInterval = kAdRotationIntervalWiFi;
        break;
    case AdManager_ReachableViaWWAN:
        m_adRotationTimeInterval = kAdRotationIntervalSlow;
        break;
    }
}

@end
