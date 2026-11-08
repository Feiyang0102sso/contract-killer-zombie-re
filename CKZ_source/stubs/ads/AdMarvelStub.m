// b107 STUB (not original code): empty AdMarvel SDK for the first link / play test. See README.md.
// Every method returns nil / NO; no ad is ever requested or shown.
#import "AdMarvelView.h"

@implementation AdMarvelView

/** Banner factory: no ad view is created. */
+ (AdMarvelView*) createAdMarvelViewWithDelegate: (id<AdMarvelDelegate>) delegate prefetchAd: (BOOL) prefetchAd
{
    return nil;
}

/** Interstitial factory: no ad view is created. */
+ (AdMarvelView*) createAdMarvelViewWithDelegate: (id<AdMarvelDelegate>) delegate
{
    return nil;
}

- (void) setDelegate: (id<AdMarvelDelegate>) delegate
{
}

- (void) getAdWithNotification
{
}

- (BOOL) isInFullScreenMode
{
    return NO;
}

- (BOOL) isInterstitialReady
{
    return NO;
}

- (void) getInterstitialAd
{
}

- (void) displayInterstitial
{
}

@end
