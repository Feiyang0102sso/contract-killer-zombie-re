// b107 STUB (not original code): empty Flurry Analytics SDK for the first link / play test. See README.md.
// No session is started and no event is recorded.
#import "FlurryAnalytics.h"

@implementation FlurryAnalytics

+ (void) startSession: (NSString*) apiKey
{
}

+ (void) setUserID: (NSString*) userID
{
}

+ (void) setSessionReportsOnCloseEnabled: (BOOL) sendSessionReportsOnClose
{
}

+ (void) setSessionReportsOnPauseEnabled: (BOOL) setSessionReportsOnPauseEnabled
{
}

+ (void) logEvent: (NSString*) eventName withParameters: (NSDictionary*) parameters
{
}

@end
