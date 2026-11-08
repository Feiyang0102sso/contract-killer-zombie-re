// PROVISIONAL (created by b033x): stand-in declarations of the Tapjoy iOS SDK.
// The SDK itself (TapjoyConnect.m, Components/TJC*/...) is third-party code compiled into zone A and
// is not rebuilt here; only the classes, selectors and notification names TapjoyManager.mm uses are
// declared, with the names/strings the original binary uses.
#ifndef TAPJOYCONNECT_H
#define TAPJOYCONNECT_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

// Notification names (posted by the SDK)
#define TJC_CONNECT_SUCCESS                         @"TJC_Connect_Success"
#define TJC_CONNECT_FAILED                          @"TJC_Connect_Failed"
#define TJC_VIEW_CLOSED_NOTIFICATION                @"TJC_SHOW_BOX_NOTIFICATION"
#define TJC_FEATURED_APP_RESPONSE_NOTIFICATION      @"TJC_FEATURED_APP_RESPONSE_NOTIFICATION"
#define TJC_TAP_POINTS_RESPONSE_NOTIFICATION        @"TJC_TAP_POINTS_RESPONSE_NOTIFICATION"
#define TJC_SPEND_TAP_POINTS_RESPONSE_NOTIFICATION  @"TJC_SPEND_TAP_POINTS_RESPONSE_NOTIFICATION"

// Display ad sizes (TJCAdDelegate adContentSize)
#define TJC_AD_BANNERSIZE_320X50                    @"320x50"
#define TJC_AD_BANNERSIZE_768X90                    @"768x90"

/** Display ad banner view (SDK class; b142: the original type encoding of TapjoyManager's m_adView ivar is @"TJCAdView"). */
@interface TJCAdView : UIView
@end

/** Featured app record delivered with TJC_FEATURED_APP_RESPONSE_NOTIFICATION. */
@interface TJCFeaturedAppModel : NSObject
- (NSString*)cost;
- (NSString*)name;
- (void)setName:(NSString*)name;
- (NSString*)description;
- (void)setDescription:(NSString*)description;
- (int)amount;
- (NSString*)iconURL;
- (NSString*)redirectURL;
- (int)maxTimesToDisplayThisApp;
@end

/** View controller the SDK presents its offer wall with. */
@interface TJCCallsWrapper : UIViewController
+ (TJCCallsWrapper*)sharedTJCCallsWrapper;
- (void)hideView;
@end

@protocol TJCAdDelegate <NSObject>
@required
- (void)didReceiveAd:(UIView*)adView;
- (void)didFailWithMessage:(NSString*)message;
- (NSString*)adContentSize;
- (BOOL)shouldRefreshAd;
@end

@protocol TJCVideoAdDelegate <NSObject>
@optional
- (void)videoAdBegan;
- (void)videoAdClosed;
@end

@interface TapjoyConnect : NSObject
+ (TapjoyConnect*)requestTapjoyConnect:(NSString*)appID secretKey:(NSString*)secretKey;
+ (void)setTransitionEffect:(int)transitionEffect;
+ (void)initVideoAdWithDelegate:(id<TJCVideoAdDelegate>)delegate;
+ (void)setVideoCacheCount:(int)count;
+ (void)getDisplayAdWithDelegate:(id<TJCAdDelegate>)delegate;
+ (TJCAdView*)getDisplayAdView;
+ (BOOL)isDisplayAdLoaded;
+ (void)refreshDisplayAd;
+ (void)showOffersWithViewController:(UIViewController*)viewController;
+ (void)updateViewsWithOrientation:(UIInterfaceOrientation)orientation;
+ (void)getTapPoints;
+ (void)spendTapPoints:(int)points;
+ (void)getFeaturedApp;
@end

#endif
