// PROVISIONAL (created by b012x): third-party AdMarvel SDK (precompiled library, header not in the project).
// Only the classes / selectors the Glu ad manager sends are declared; names match the original binary.
#import <UIKit/UIKit.h>

@protocol AdMarvelDelegate;

/** AdMarvel banner / interstitial view (UIView subclass inside the SDK). */
@interface AdMarvelView : UIView

/** Creates a banner view; prefetchAd starts loading the first ad immediately. */
+ (AdMarvelView*) createAdMarvelViewWithDelegate: (id<AdMarvelDelegate>) delegate prefetchAd: (BOOL) prefetchAd;
/** Creates a view used for interstitial ads. */
+ (AdMarvelView*) createAdMarvelViewWithDelegate: (id<AdMarvelDelegate>) delegate;

- (void) setDelegate: (id<AdMarvelDelegate>) delegate;
/** Requests a new banner ad; the delegate gets getAdSucceeded / getAdFailed. */
- (void) getAdWithNotification;
- (BOOL) isInFullScreenMode;

- (BOOL) isInterstitialReady;
- (void) getInterstitialAd;
- (void) displayInterstitial;

@end

/** Callbacks the SDK sends to its delegate (the subset implemented by the Glu delegate). */
// b142: methods, order and required/optional completed from the original protocol_t metadata (the original SDK header was a newer version)
@class CLLocation;

@protocol AdMarvelDelegate <NSObject>
- (NSString*) partnerId;
- (NSString*) siteId;
- (UIViewController*) applicationUIViewController;
@optional
- (NSDictionary*) targetingParameters;
- (CGRect) adMarvelViewFrame;
- (BOOL) testingEnabled;
- (BOOL) animationsEnabled;
- (UIColor*) backgroundColor;
- (UIColor*) textAdFontColor;
- (UIColor*) textAdBackgroundColor;
- (UIColor*) textAdBorderColor;
- (UIColor*) fullScreenToolBarColor;
- (UIBarStyle) fullScreenToolBarStyle;
- (BOOL) locationServicesEnabled;
- (CLLocation*) locationObject;
- (void) fullScreenWebViewActivated;
- (void) fullScreenWebViewClosed;
- (void) getAdSucceeded;
- (void) getAdFailed;
- (void) handleAdMarvelSDKClick: (NSString*) urlString;
- (void) adDidExpand;
- (void) adDidCollapse;
- (void) getInterstitialAdSucceeded;
- (void) getInterstitialAdFailed;
- (void) interstitialActivated;
- (void) interstitialClosed;
- (void) disableRotations;
- (void) enableRotations;
- (void) adMarvelViewWasClicked;
@end
