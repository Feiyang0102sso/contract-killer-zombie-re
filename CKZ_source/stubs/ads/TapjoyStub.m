// b107 STUB (not original code): empty Tapjoy SDK for the first link / play test. See README.md.
// No connection is made, no notification is posted, no view is created.
#import "TapjoyConnect.h"

@implementation TJCCallsWrapper

/** No offer-wall controller exists; TapjoyManager falls back to the app's own controller. */
+ (TJCCallsWrapper*)sharedTJCCallsWrapper
{
    return nil;
}

- (void)hideView
{
}

@end

@implementation TapjoyConnect

+ (TapjoyConnect*)requestTapjoyConnect:(NSString*)appID secretKey:(NSString*)secretKey
{
    return nil;
}

+ (void)setTransitionEffect:(int)transitionEffect
{
}

+ (void)initVideoAdWithDelegate:(id<TJCVideoAdDelegate>)delegate
{
}

+ (void)setVideoCacheCount:(int)count
{
}

+ (void)getDisplayAdWithDelegate:(id<TJCAdDelegate>)delegate
{
}

+ (TJCAdView*)getDisplayAdView
{
    return nil;
}

+ (BOOL)isDisplayAdLoaded
{
    return NO;
}

+ (void)refreshDisplayAd
{
}

+ (void)showOffersWithViewController:(UIViewController*)viewController
{
}

+ (void)updateViewsWithOrientation:(UIInterfaceOrientation)orientation
{
}

+ (void)getTapPoints
{
}

+ (void)spendTapPoints:(int)points
{
}

+ (void)getFeaturedApp
{
}

@end
