// PROVISIONAL (created by b012x): third-party Flurry Analytics SDK (precompiled library, header not in
// the project). Only the class methods the Glu wrapper sends are declared; names match the original binary.
#import <Foundation/Foundation.h>

@interface FlurryAnalytics : NSObject

+ (void) startSession: (NSString*) apiKey;
+ (void) setUserID: (NSString*) userID;
+ (void) setSessionReportsOnCloseEnabled: (BOOL) sendSessionReportsOnClose;
+ (void) setSessionReportsOnPauseEnabled: (BOOL) setSessionReportsOnPauseEnabled;
+ (void) logEvent: (NSString*) eventName withParameters: (NSDictionary*) parameters;

@end
