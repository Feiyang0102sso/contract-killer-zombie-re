// CFlurry_Cocoa_mm.mm - Flurry analytics on iOS: session start / stop events, jailbreak and crash
// detection, and custom events with the standard Glu parameter set.
#include "CFlurry.h"

#import <UIKit/UIKit.h>
#import "FlurryAnalytics.h"

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <wchar.h>

// the MAC address buffer below comes straight from the C library
#undef malloc
#undef free

// ---- constants ----
/** NSUserDefaults key of the crash flag: > 0 / -1 while a session runs, 0 after a clean stop. */
static const char* const kCrashFlagKey = "com.glu.flurry.crashflag";
/** Memory warnings counted at most up to this value. */
static const int kMaxMemoryWarnings = 1000;

// ---- state (merged into one block by the compiler, as in the original) ----
static unsigned char s_multitaskingSupported = 0;  // background / foreground notifications registered
static unsigned char s_jailbroken = 0;             // a jailbreak application was found
static long s_sessionStartTime = 0;                // seconds, start of the current session
static int s_memoryWarningCount = 0;               // memory warnings in this session
static id s_lifetimeHandler;                       // GLUApplicationLifetimeHandler, created once

static void SetCrashFlag(bool running);
static void TestCrashFlag();
static int GetCrashFlag();
static NSString* UserID();
static void LogEvent(NSString* eventName, NSString* info, int eventValue, int externalRef);

/** Watches the application life cycle and logs the matching Flurry events. */
@interface GLUApplicationLifetimeHandler : NSObject
{
}
- (void) enterBackground: (NSNotification*) notification;
- (void) enterForeground: (NSNotification*) notification;
- (void) terminate: (NSNotification*) notification;
- (void) receiveMemoryWarning: (NSNotification*) notification;
@end

@implementation GLUApplicationLifetimeHandler

- (id) init
{
    self = [super init];
    if (self != nil) {
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        [center addObserver:self selector:@selector(terminate:) name:UIApplicationWillTerminateNotification object:nil];
        if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]) {
            [center addObserver:self selector:@selector(enterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
            [center addObserver:self selector:@selector(enterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
            s_multitaskingSupported = 1;
        }
        [center addObserver:self selector:@selector(receiveMemoryWarning:) name:UIApplicationDidReceiveMemoryWarningNotification object:nil];
    }
    return self;
}

- (void) enterBackground: (NSNotification*) notification
{
    struct timeval now;
    gettimeofday(&now, NULL);
    CFlurry::LogCustomEvent("SessionStop", L"", now.tv_sec - s_sessionStartTime, 0);
    SetCrashFlag(false);
}

- (void) enterForeground: (NSNotification*) notification
{
    struct timeval now;
    gettimeofday(&now, NULL);
    s_sessionStartTime = now.tv_sec;
    CFlurry::LogCustomEvent("SessionStart", L"", 0, 0);
    // ?: kept: the original selects the text with a conditional move
    CFlurry::LogCustomEvent("Jailbroken", s_jailbroken ? L"Jailbroken" : L"Genuine", s_jailbroken, 0);
    TestCrashFlag();
    SetCrashFlag(true);
}

- (void) terminate: (NSNotification*) notification
{
    // With multitasking the session was already closed by enterBackground:
    if (!s_multitaskingSupported) {
        struct timeval now;
        gettimeofday(&now, NULL);
        CFlurry::LogCustomEvent("SessionStop", L"", now.tv_sec - s_sessionStartTime, 0);
    }
    CFlurry::LogCustomEvent("AppStop", L"", 0, 0);
    SetCrashFlag(false);
}

- (void) receiveMemoryWarning: (NSNotification*) notification
{
    if (s_memoryWarningCount < kMaxMemoryWarnings) {
        s_memoryWarningCount++;
    }
    // Refresh the stored flag with the new count while a session runs
    SetCrashFlag(GetCrashFlag() != 0);
}

@end

void CFlurry::LogCustomEvent(const char* eventName, const wchar_t* info, int eventValue, int externalRef)
{
    NSString* eventString = [[NSString alloc] initWithCString:eventName encoding:NSASCIIStringEncoding];
    NSString* infoString;
    if (info != NULL && info[0] != 0) {
        infoString = [[NSString alloc] initWithBytes:info length:wcslen(info) * sizeof(wchar_t)
                                            encoding:NSUTF32LittleEndianStringEncoding];
    } else {
        infoString = [[NSString alloc] init];
    }
    LogEvent(eventString, infoString, eventValue, externalRef);
    [eventString release];
    [infoString release];
}

/** Stores the crash flag: while running the memory warning count (or -1), 0 after a clean stop. */
static void SetCrashFlag(bool running)
{
    NSString* key = [[NSString alloc] initWithCString:kCrashFlagKey encoding:NSASCIIStringEncoding];
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    int value = 0;
    if (running) {
        value = s_memoryWarningCount;
        if (value <= 0) {
            value = -1;
        }
    }
    [defaults setInteger:value forKey:key];
    [key release];
}

/** Reports a crash of the previous session (flag still set at start-up). */
static void TestCrashFlag()
{
    int flag = GetCrashFlag();
    if (flag < 0) {
        CFlurry::LogCustomEvent("GENERAL_EVT_TYPE_CRASH", "Caught crashflag", 0, 0);
    } else if (flag > 0) {
        CFlurry::LogCustomEvent("GENERAL_EVT_TYPE_CRASH", "Caught crashflag and memory warning", flag, 0);
    }
}

static int GetCrashFlag()
{
    NSString* key = [[NSString alloc] initWithCString:kCrashFlagKey encoding:NSASCIIStringEncoding];
    int flag = [[NSUserDefaults standardUserDefaults] integerForKey:key];
    [key release];
    return flag;
}

/** Stored user id: created once from the MAC address of en0, or from a UUID when that fails. */
static NSString* UserID()
{
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* userIdKey = [[NSString alloc] initWithCString:"com.glu.userid" encoding:NSASCIIStringEncoding];
    NSString* userId = [defaults stringForKey:userIdKey];
    if (userId == nil) {
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
                        userId = [defaults stringForKey:userIdKey];
                    }
                    free(buffer);
                }
            }
        }
        if (userId == nil) {
            CFUUIDRef uuid = CFUUIDCreate(NULL);
            CFStringRef uuidString = CFUUIDCreateString(NULL, uuid);
            [defaults setObject:(NSString*)uuidString forKey:userIdKey];
            CFRelease(uuidString);
            CFRelease(uuid);
            userId = [defaults stringForKey:userIdKey];
        }
    }
    [userIdKey release];
    return userId;
}

/**
 * Dead-stripped in the shipped binary; the name and body are speculation (b117-inl).
 * The original kept UserID() out of line although StartSession is its only surviving
 * caller, so the original object had a second external caller that the linker removed.
 */
NSString* CFlurry_GetUserID()
{
    return UserID();
}

void CFlurry::StartSession(const char* apiKey)
{
    static const char* jailbreak_apps[] = {
        "/Applications/Cydia.app",
        "/Applications/limera1n.app",
        "/Applications/greenpois0n.app",
        "/Applications/blackra1n.app",
        "/Applications/blacksn0w.app",
        "/Applications/redsn0w.app",
        "/Applications/IAPFree.app"
    };

    if (s_lifetimeHandler != nil) {
        return;
    }

    // The original exits on "index == 7" (cmp #7; beq), i.e. a != test (b117-inl).
    for (unsigned int index = 0; index != sizeof(jailbreak_apps) / sizeof(jailbreak_apps[0]); index++) {
        if ([[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithUTF8String:jailbreak_apps[index]]]) {
            s_jailbroken = 1;
            break;
        }
    }

    [FlurryAnalytics setUserID:UserID()];
    NSString* apiKeyString = [[NSString alloc] initWithCString:apiKey encoding:NSASCIIStringEncoding];
    [FlurryAnalytics setSessionReportsOnCloseEnabled:NO];
    [FlurryAnalytics setSessionReportsOnPauseEnabled:NO];
    [FlurryAnalytics startSession:apiKeyString];

    struct timeval now;
    gettimeofday(&now, NULL);
    s_sessionStartTime = now.tv_sec;
    s_lifetimeHandler = [[GLUApplicationLifetimeHandler alloc] init];

    // Launch counter, stored under the API key
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    int launchCount = [defaults integerForKey:apiKeyString] + 1;
    CFlurry::LogCustomEvent("AppStart", L"", launchCount, 0);
    CFlurry::LogCustomEvent("SessionStart", L"", 0, 0);
    // ?: kept: the original selects the text with a conditional move
    CFlurry::LogCustomEvent("Jailbroken", s_jailbroken ? L"Jailbroken" : L"Genuine", s_jailbroken, 0);
    [defaults setInteger:launchCount forKey:apiKeyString];
    [apiKeyString release];

    TestCrashFlag();
    SetCrashFlag(true);
}

/** Sends one Flurry event with the standard parameter dictionary. */
static void LogEvent(NSString* eventName, NSString* info, int eventValue, int externalRef)
{
    NSString* eventValueString = [[NSString alloc] initWithFormat:@"%d", eventValue];
    NSString* externalRefString = [[NSString alloc] initWithFormat:@"%d", externalRef];
    struct timeval now;
    gettimeofday(&now, NULL);
    unsigned int seconds = now.tv_sec;
    NSString* eventTimeString = [[NSString alloc] initWithFormat:@"%qu", (unsigned long long)seconds * 1000];
    NSString* language = [[NSLocale preferredLanguages] objectAtIndex:0];
    NSString* localeIdentifier = [[NSLocale currentLocale] objectForKey:NSLocaleIdentifier];
    NSString* country = [[NSLocale componentsFromLocaleIdentifier:localeIdentifier] objectForKey:NSLocaleCountryCode];
    NSString* osVersion = [[UIDevice currentDevice] systemVersion];
    NSDictionary* parameters = [[NSDictionary alloc] initWithObjectsAndKeys:
                                info, @"info",
                                eventValueString, @"eventValue",
                                externalRefString, @"externalRef",
                                eventTimeString, @"Event time",
                                language, @"Language",
                                country, @"Country",
                                osVersion, @"OS version",
                                nil];
    [FlurryAnalytics logEvent:eventName withParameters:parameters];
    [eventValueString release];
    [externalRefString release];
    [eventTimeString release];
    [parameters release];
}

void CFlurry::LogCustomEvent(const char* eventName, const char* info, int eventValue, int externalRef)
{
    NSString* eventString = [[NSString alloc] initWithCString:eventName encoding:NSASCIIStringEncoding];
    NSString* infoString;
    if (info != NULL && info[0] != 0) {
        infoString = [[NSString alloc] initWithUTF8String:info];
    } else {
        infoString = [[NSString alloc] init];
    }
    LogEvent(eventString, infoString, eventValue, externalRef);
    [eventString release];
    [infoString release];
}

void CFlurry::SetSessionReportsOnCloseEnabled(bool enabled)
{
    [FlurryAnalytics setSessionReportsOnCloseEnabled:enabled];
}

void CFlurry::SetSessionReportsOnPauseEnabled(bool enabled)
{
    [FlurryAnalytics setSessionReportsOnPauseEnabled:enabled];
}
