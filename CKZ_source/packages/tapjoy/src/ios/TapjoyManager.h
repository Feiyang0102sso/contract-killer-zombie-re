// TapjoyManager.h - Glu's Objective-C front end to the Tapjoy SDK (created by b033x):
// connect, offer wall, tap points (server and locally cached), featured app, display ad banner, video ads.
#ifndef TAPJOYMANAGER_H
#define TAPJOYMANAGER_H

#import "TapjoyConnect.h"

@class AppDelegate;

/** Video ad notification: (1 = began / 0 = closed, context). */
typedef void (*TapjoyVideoAdCallback)(unsigned char began, void* context);

@interface TapjoyManager : NSObject <TJCAdDelegate, TJCVideoAdDelegate> {
    UIWindow* window;                           // 0x04 application window
    AppDelegate* appDelegate;                   // 0x08 UIApplication delegate (b142: ivar type encoding @"AppDelegate")
    NSString* m_tapjoyId;                       // 0x0c app id (autoreleased string, released in dealloc)
    NSString* m_tapjoyAdsBannerSize;            // 0x10 TJC_AD_BANNERSIZE_*
    TapjoyConnect* m_tapjoy;                    // 0x14 retained
    unsigned int m_previousConsumed;            // 0x18 points of the last consumeTapjoyPoints:
    unsigned char m_tapjoyConnected;            // 0x1c
    unsigned char m_tapjoyOpen;                 // 0x1d offer wall shown
    unsigned char m_gotUpdatedPoints;           // 0x1e no tap points request in flight
    unsigned char m_tapjoyFirstLaunch;          // 0x1f no saved local points yet
    unsigned int m_serverTapjoyPoints;          // 0x20
    unsigned int m_localLastPointTotal;         // 0x24 last server total seen
    unsigned int m_localRemainingPointTotal;    // 0x28 points not yet consumed by the game
    TJCAdView* m_adView;                        // 0x2c retained display ad banner (b142: type encoding @"TJCAdView")
    TapjoyVideoAdCallback m_videoAdCallback;    // 0x30
    void* m_videoCallbackPtr;                   // 0x34 callback context
    unsigned char m_showBanner;                 // 0x38
    int m_adState;                              // 0x3c AdState_*
    int m_timeToRefreshAds;                     // 0x40 ms until the shown banner is refreshed
    int m_timeToRetryAds;                       // 0x44 ms until a failed banner is retried
}

- (id)init:(const char*)appId :(const char*)secretKey :(bool)enableVideoAds;
- (NSString*)getTapjoyID;
- (void)tick:(int)deltaTime;
- (unsigned char)canDisplayInterface;
- (unsigned char)interfaceIsOpen;
- (void)displayInterface;
- (void)updateVideoOrientation;
- (void)updateBounds:(UIView*)view;
- (void)removeInterface;
- (void)onResume;
- (void)tjcConnectSuccess:(NSNotification*)notification;
- (void)tjcConnectFail:(NSNotification*)notification;
- (void)getUpdatePoints:(NSNotification*)notification;
- (void)getSpentPoints:(NSNotification*)notification;
- (void)queryFeaturedApp;
- (BOOL)isIntValue:(NSString*)string;
- (NSString*)replaceHTMLCodesInString:(NSString*)string;
- (void)getFeaturedApp:(NSNotification*)notification;
- (void)tjcWebViewOpen;
- (void)tjcWebViewClosed:(NSNotification*)notification;
- (unsigned int)getRemainingTapjoyPoints;
- (unsigned int)getLastTapjoyPointTotal;
- (unsigned char)consumeTapjoyPoints:(unsigned int)points;
- (void)saveLocalPoints;
- (unsigned char)loadLocalPoints;
- (unsigned int)getServerTapjoyPoints;
- (unsigned char)consumeServerTapjoyPoints:(unsigned int)points;
- (unsigned int)convertPointsToGameCurrency:(unsigned int)points;
- (unsigned int)getPreviousConsumedThisSession;
- (unsigned int)getExchangeRate;
- (void)showAd:(int)alignment :(int)x :(int)y;
- (void)hideAd;
- (bool)isVisible;
- (void)registerVideoCallback:(TapjoyVideoAdCallback)callback :(void*)context;
- (void)setVideoCacheCount:(unsigned int)count;
- (void)debugGivePoints:(unsigned int)points;

@end

#endif
