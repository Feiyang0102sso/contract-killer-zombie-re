// AdManager_Reachability.h - network reachability monitor (Apple "Reachability" sample, renamed for the ad manager).
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>

/** Kind of network the internet is reachable through. */
typedef enum {
    AdManager_NotReachable = 0,
    AdManager_ReachableViaWiFi,
    AdManager_ReachableViaWWAN
} AdManager_NetworkStatus;

/** Posted (object = the AdManager_Reachability) whenever the reachability flags change. */
#define AdManager_kNetworkReachabilityChangedNotification @"AdManager_kNetworkReachabilityChangedNotification"

@interface AdManager_Reachability : NSObject
{
    SCNetworkReachabilityRef reachabilityRef;
}

/** Starts listening for reachability notifications on the current run loop. */
- (BOOL) startNotifier;
- (void) stopNotifier;

/** Checks the reachability of a particular IP address. */
+ (AdManager_Reachability*) reachabilityWithAddress: (const struct sockaddr_in*) hostAddress;

/** Checks whether the default route is available. */
+ (AdManager_Reachability*) reachabilityForInternetConnection;

- (AdManager_NetworkStatus) networkStatusForFlags: (SCNetworkReachabilityFlags) flags;
- (AdManager_NetworkStatus) currentReachabilityStatus;

@end
