// CCore_Cocoa_mm.mm - UIKit / Foundation part of the iOS platform core (see CCore_Cocoa.h).
#include <wchar.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#import <SystemConfiguration/SystemConfiguration.h>

#include "CCore_Cocoa.h"
#include "CStrChar.h"
#include "CStrWChar.h"
#import "AppDelegate.h"

// the sysctl buffers here come straight from the C library
#undef malloc
#undef free

using namespace com::glu::platform::components;
using namespace com::glu::platform::framework;

// ---- constants ----
static const unsigned int SystemEvent_DeviceOrientationChanged = 0x747ab255;   // queued by SetDeviceOrientation
static const unsigned int SystemEvent_GraphicsSurfaceChanged = 0x97b4dec3;     // the window surface must be rebuilt
static const unsigned int SystemEvent_LayoutChanged = 0x24314258;              // screen layout follows the orientation

NSString* UserIDAsNSString();

unsigned char CCore_Cocoa::DeviceCanMultitask() const
{
    UIDevice* device = [UIDevice currentDevice];
    bool canMultitask = false;
    if ([device respondsToSelector:@selector(isMultitaskingSupported)]) {
        canMultitask = [device isMultitaskingSupported] == YES;
    }
    return canMultitask;
}

unsigned char CCore_Cocoa::LaunchURL(const wchar_t* url, unsigned char exitApplication)
{
    NSString* text = [[NSString alloc] initWithBytes:url
                                              length:wcslen(url) * sizeof(wchar_t)
                                            encoding:NSUTF32LittleEndianStringEncoding];
    NSURL* address = [NSURL URLWithString:text];
    [[UIApplication sharedApplication] openURL:address];
    [text release];
    return 1;
}

CStrWChar CCore_Cocoa::GetDeviceIdentifier() const
{
    return CStrWChar([UserIDAsNSString() cStringUsingEncoding:NSASCIIStringEncoding]);
}

/**
 * The persistent user id: stored in the user defaults; first set from the en0 MAC address,
 * or from a fresh UUID when that cannot be read.
 */
NSString* UserIDAsNSString()
{
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* key = [[NSString alloc] initWithCString:"com.glu.userid" encoding:NSASCIIStringEncoding];
    NSString* userId = [defaults stringForKey:key];

    if (userId == nil) {
        int mib[6];
        size_t length;
        char* buffer;

        mib[0] = CTL_NET;
        mib[1] = AF_ROUTE;
        mib[2] = 0;
        mib[3] = AF_LINK;
        mib[4] = NET_RT_IFLIST;
        mib[5] = if_nametoindex("en0");
        if (mib[5] != 0) {
            if (sysctl(mib, 6, NULL, &length, NULL, 0) >= 0) {
                buffer = (char*)malloc(length);
                if (buffer != NULL) {
                    if (sysctl(mib, 6, buffer, &length, NULL, 0) >= 0) {
                        struct if_msghdr* message = (struct if_msghdr*)buffer;
                        struct sockaddr_dl* address = (struct sockaddr_dl*)(message + 1);
                        unsigned char* mac = (unsigned char*)LLADDR(address);
                        NSString* macText = [[NSString alloc] initWithFormat:@"%02X%02X%02X%02X%02X%02X",
                                             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]];
                        [defaults setObject:[macText lowercaseString] forKey:key];
                        [macText release];
                        userId = [defaults stringForKey:key];
                    }
                    free(buffer);
                }
            }
        }

        if (userId == nil) {
            CFUUIDRef uuid = CFUUIDCreate(NULL);
            CFStringRef uuidText = CFUUIDCreateString(NULL, uuid);
            [defaults setObject:(NSString*)uuidText forKey:key];
            CFRelease(uuidText);
            CFRelease(uuid);
            userId = [defaults stringForKey:key];
        }
    }

    [key release];
    return userId;
}

CStrWChar CCore_Cocoa::GetPlatformIdentifier() const
{
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char* machine = (char*)malloc(size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    CStrWChar identifier(machine);
    free(machine);
    return identifier;
}

CStrWChar CCore_Cocoa::GetPlatformVersion() const
{
    return CStrWChar([[[UIDevice currentDevice] systemVersion] cStringUsingEncoding:NSASCIIStringEncoding]);
}

CStrWChar CCore_Cocoa::GetPlatformName() const
{
    return CStrWChar("iphone");
}

unsigned char CCore_Cocoa::CanDetectLocale() const
{
    return 1;
}

unsigned char CCore_Cocoa::DetectLocale(CStrChar& locale) const
{
    locale = "";

    const char* identifier = [[[NSLocale currentLocale] objectForKey:NSLocaleIdentifier]
                              cStringUsingEncoding:NSASCIIStringEncoding];
    if (identifier == NULL || identifier[0] == 0) {
        return 0;
    }
    locale = identifier;
    return 1;
}

unsigned char CCore_Cocoa::CanSetDeviceOrientation(eDeviceOrientation orientation) const
{
    return m_canSetDeviceOrientation[DeviceOrientationIndex(orientation)];
}

unsigned char CCore_Cocoa::SetDeviceOrientation(eDeviceOrientation orientation)
{
    if (!CanSetDeviceOrientation(orientation)) {
        return 0;
    }
    int index = DeviceOrientationIndex(orientation);
    if (!m_canSetDeviceOrientation[index]) {
        return 0;
    }

    m_deviceOrientationSet = 1;
    m_deviceOrientation = (eDeviceOrientation)index;
    CApplet::m_pThis->QueueSystemEvent(SystemEvent_DeviceOrientationChanged, 0, 0);
    CApplet::m_pThis->QueueSystemEvent(SystemEvent_GraphicsSurfaceChanged, 0, 0);
    CApplet::m_pThis->QueueSystemEvent(SystemEvent_LayoutChanged, 0, 0);
    return 1;
}

unsigned char CCore_Cocoa::SetBacklightEnabled(unsigned char enabled)
{
    [[UIApplication sharedApplication] setIdleTimerDisabled:enabled != 0];
    return 1;
}

ICCore::eAutoRotationOrientation CCore_Cocoa::GetDisplayOrientationInTermsOfAutoRotation() const
{
    id delegate = [[UIApplication sharedApplication] delegate];
    if ([delegate respondsToSelector:@selector(viewController)]) {
        UIViewController* controller = [delegate viewController];
        if (controller != nil) {
            switch ([controller interfaceOrientation]) {
            case UIInterfaceOrientationPortrait:
                return AutoRotation_Portrait;
            case UIInterfaceOrientationPortraitUpsideDown:
                return AutoRotation_PortraitUpsideDown;
            case UIInterfaceOrientationLandscapeRight:
                return AutoRotation_LandscapeRight;
            case UIInterfaceOrientationLandscapeLeft:
                return AutoRotation_LandscapeLeft;
            }
        }
    }
    return AutoRotation_None;
}

void CCore_Cocoa::AttachView() const
{
    id delegate = [[UIApplication sharedApplication] delegate];
    if ([delegate respondsToSelector:@selector(viewController)]) {
        UIViewController* controller = [delegate viewController];
        if (controller != nil) {
            UIWindow* window = [[UIApplication sharedApplication] keyWindow];
            UIView* view = [[[window subviews] objectAtIndex:0] retain];
            [view removeFromSuperview];
            [controller setView:view];
            [window insertSubview:[controller view] atIndex:0];
            [view release];
        }
    }
}

void CCore_Cocoa::DetachView() const
{
    id delegate = [[UIApplication sharedApplication] delegate];
    if ([delegate respondsToSelector:@selector(viewController)]) {
        UIViewController* controller = [delegate viewController];
        if (controller != nil) {
            UIWindow* window = [[UIApplication sharedApplication] keyWindow];
            UIView* view = [[[window subviews] objectAtIndex:0] retain];
            [view removeFromSuperview];
            [controller setView:nil];
            [window insertSubview:view atIndex:0];
            [view release];
        }
    }
}

static NSString* CCore_Cocoa_AllocAndInitWithCStrWChar(const CStrWChar& text);

void CCore_Cocoa::SystemMessageBox(const CStrWChar& title, const CStrWChar& message) const
{
    NSString* titleText = CCore_Cocoa_AllocAndInitWithCStrWChar(title);
    NSString* messageText = CCore_Cocoa_AllocAndInitWithCStrWChar(message);
    UIAlertView* alert = [[UIAlertView alloc] initWithTitle:titleText
                                                    message:messageText
                                                   delegate:nil
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [titleText release];
    [messageText release];
    [[[[[UIApplication sharedApplication] keyWindow] subviews] objectAtIndex:0] addSubview:alert];
    [alert show];
    [alert release];
}

/** Retained NSString with the contents of `text` (UTF-32 wide characters). */
static NSString* CCore_Cocoa_AllocAndInitWithCStrWChar(const CStrWChar& text)
{
    int length = text.GetLength();
    NSString* string = [NSString alloc];
    if (length > 0) {
        return [string initWithBytes:text.GetWCharPtr()
                              length:text.GetLength() * sizeof(wchar_t)
                            encoding:NSUTF32LittleEndianStringEncoding];
    }
    return [string init];
}

CStrWChar CCore_Cocoa::GetDeviceModel() const
{
    return CStrWChar([[[UIDevice currentDevice] model] UTF8String]);
}

unsigned char CCore_Cocoa::IsNetworkAvailable() const
{
    SCNetworkReachabilityFlags flags;
    bool reachable = false;
    SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithName(NULL, "www.apple.com");
    if (SCNetworkReachabilityGetFlags(reachability, &flags)) {
        reachable = false;
        if (flags & kSCNetworkReachabilityFlagsReachable) {
            reachable = true;
        }
    }
    CFRelease(reachability);
    return reachable;
}

unsigned char CCore_Cocoa::CanDetectTimezone() const
{
    return 1;
}

CStrWChar CCore_Cocoa::GetTimezone() const
{
    return CStrWChar([[[NSTimeZone systemTimeZone] name] UTF8String]);
}
