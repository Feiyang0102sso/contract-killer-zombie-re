// AdManager_Reachability.m - Apple "Reachability" sample (v2.2) trimmed and renamed for the ad manager.
//
// Origin: Apple sample code "Reachability" version 2.2 (Reachability.m), copied into the Glu
// ad manager package with every public name prefixed by AdManager_ so that it can live next
// to other copies of the sample in the same application.
//
// Differences from the sample as shipped by Apple:
//   - no reachabilityWithHostName: / reachabilityForLocalWiFi (only the internet check is used)
//   - no localWiFiRef flag, so currentReachabilityStatus always uses networkStatusForFlags:
//   - no connectionRequired and no PrintReachabilityFlags debug helper
//
// NOTE (reconstruction): the original file keeps Apple's long license header; the assertion
// macros below embed __LINE__ into the binary, so the line numbers of the NSCAssert /
// NSAssert calls must stay where they are (39, 40 and 151).
//
// Behaviour summary:
//   startNotifier    - registers ReachabilityCallback on the current run loop
//   stopNotifier     - unregisters it
//   ReachabilityCallback posts AdManager_kNetworkReachabilityChangedNotification with the
//   AdManager_Reachability object, CAdManager_AdMarvel_Cocoa_Delegate listens for it and
//   adapts the banner rotation interval to the network type.
//
// Flag decoding (networkStatusForFlags:):
//   not Reachable                         -> NotReachable
//   Reachable, no ConnectionRequired      -> ReachableViaWiFi
//   Reachable, on demand/traffic, no user intervention -> ReachableViaWiFi
//   IsWWAN                                -> ReachableViaWWAN
//
#import "AdManager_Reachability.h"

#import <CoreFoundation/CoreFoundation.h>

@implementation AdManager_Reachability

/** SCNetworkReachability callback: forwards the change as an NSNotification. */
static void ReachabilityCallback(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* info)
{
#pragma unused (target, flags)
    NSCAssert(info != NULL, @"info was NULL in ReachabilityCallback");
    NSCAssert([(NSObject*) info isKindOfClass: [AdManager_Reachability class]], @"info was wrong class in ReachabilityCallback");

    // We're on the main RunLoop, so an NSAutoreleasePool is not necessary, but is added defensively
    // in case someone uses the Reachability object in a different thread.
    NSAutoreleasePool* myPool = [[NSAutoreleasePool alloc] init];

    AdManager_Reachability* noteObject = (AdManager_Reachability*) info;
    // Post a notification to notify the client that the network reachability changed.
    [[NSNotificationCenter defaultCenter] postNotificationName: AdManager_kNetworkReachabilityChangedNotification object: noteObject];

    [myPool release];
}

- (BOOL) startNotifier
{
    BOOL retVal = NO;
    SCNetworkReachabilityContext context = {0, self, NULL, NULL, NULL};
    if (SCNetworkReachabilitySetCallback(reachabilityRef, ReachabilityCallback, &context)) {
        if (SCNetworkReachabilityScheduleWithRunLoop(reachabilityRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode)) {
            retVal = YES;
        }
    }
    return retVal;
}

- (void) stopNotifier
{
    if (reachabilityRef != NULL) {
        SCNetworkReachabilityUnscheduleFromRunLoop(reachabilityRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }
}

- (void) dealloc
{
    [self stopNotifier];
    if (reachabilityRef != NULL) {
        CFRelease(reachabilityRef);
    }
    [super dealloc];
}

+ (AdManager_Reachability*) reachabilityWithAddress: (const struct sockaddr_in*) hostAddress
{
    SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)hostAddress);
    AdManager_Reachability* retVal = NULL;
    if (reachability != NULL) {
        retVal = [[[self alloc] init] autorelease];
        if (retVal != NULL) {
            retVal->reachabilityRef = reachability;
        }
    }
    return retVal;
}

+ (AdManager_Reachability*) reachabilityForInternetConnection
{
    struct sockaddr_in zeroAddress;
    bzero(&zeroAddress, sizeof(zeroAddress));
    zeroAddress.sin_len = sizeof(zeroAddress);
    zeroAddress.sin_family = AF_INET;
    return [self reachabilityWithAddress: &zeroAddress];
}

#pragma mark Network Flag Handling

- (AdManager_NetworkStatus) networkStatusForFlags: (SCNetworkReachabilityFlags) flags
{
    if ((flags & kSCNetworkReachabilityFlagsReachable) == 0) {
        // if target host is not reachable
        return AdManager_NotReachable;
    }

    AdManager_NetworkStatus retVal = AdManager_NotReachable;

    if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0) {
        // if target host is reachable and no connection is required
        //  then we'll assume (for now) that your on Wi-Fi
        retVal = AdManager_ReachableViaWiFi;
    }

    if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand) != 0) ||
         (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0)) {
        // ... and the connection is on-demand (or on-traffic) if the
        //     calling application is using the CFSocketStream or higher APIs
        if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0) {
            // ... and no [user] intervention is needed
            retVal = AdManager_ReachableViaWiFi;
        }
    }

    if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == kSCNetworkReachabilityFlagsIsWWAN) {
        // ... but WWAN connections are OK if the calling application
        //     is using the CFNetwork (CFSocketStream?) APIs.
        retVal = AdManager_ReachableViaWWAN;
    }
    return retVal;
}

/**
 * Current reachability of the monitored address.
 *
 * Reads the flags synchronously with SCNetworkReachabilityGetFlags (may block
 * briefly while the system resolves the route) and decodes them with
 * networkStatusForFlags:. Returns AdManager_NotReachable when the flags cannot
 * be read.
 *
 * Asserts (debug builds) that the reachability reference exists.
 */

- (AdManager_NetworkStatus) currentReachabilityStatus
{
    NSAssert(reachabilityRef != NULL, @"currentReachabilityStatus called with NULL reachabilityRef");
    AdManager_NetworkStatus retVal = AdManager_NotReachable;
    SCNetworkReachabilityFlags flags;
    if (SCNetworkReachabilityGetFlags(reachabilityRef, &flags)) {
        retVal = [self networkStatusForFlags: flags];
    }
    return retVal;
}

@end
