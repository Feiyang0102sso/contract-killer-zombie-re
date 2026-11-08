// AppDelegate_mm.mm - UIApplication delegate of the Glu application (see AppDelegate.h).
#include <math.h>
#import "AppDelegate.h"
#import "AppView.h"
#include "CApplet.h"
#include "ICCore.h"
#include "CPushNotification_Cocoa.h"
#include "CLocalNotification_Cocoa.h"

using namespace com::glu::platform::framework;

// ---- constants ----
static const unsigned int SystemEvent_Suspend = 0x54246de3;            // resign active
static const unsigned int SystemEvent_Resume = 0x75424363;             // become active
static const unsigned int SystemEvent_LowMemory = 0x695fb416;          // memory warning
static const unsigned int SystemEvent_EnterBackground = 0x1e96b3a4;
static const unsigned int SystemEvent_EnterForeground = 0x0096b3eb;
static const unsigned int SystemEvent_Accelerometer = 0xad174cef;      // x, y, z as packed 21-bit values

@implementation AppDelegate

@synthesize window;
@synthesize viewController;

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    if (viewController == nil) {
        viewController = [[AppViewController alloc] init];
    }

    // the GL view goes behind everything of the nib's root view
    UIView* rootView = [[window subviews] objectAtIndex:0];
    [rootView setOpaque:YES];
    [rootView setClearsContextBeforeDrawing:NO];
    AppGLView* glView = [[AppGLView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    [rootView addSubview:glView];
    [rootView sendSubviewToBack:glView];

    m_pApplet = new CApplet();
    if (m_pApplet != NULL && m_pApplet->Initialize()) {
        m_pApplet->Start();
        m_active = true;
    }

    if (m_active) {
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(DidRotate:)
                                                     name:@"UIDeviceOrientationDidChangeNotification"
                                                   object:nil];
        if (m_pApplet != NULL) {
            m_pApplet->SetAccelerometerFrequency(m_pApplet->m_accelerometerFrequency);
        }

        // launched from a remote notification
        id remoteNotification = [launchOptions objectForKey:UIApplicationLaunchOptionsRemoteNotificationKey];
        if (remoteNotification != nil) {
            CPushNotification_Cocoa* pushNotification = CPushNotification_Cocoa::GetInstance();
            if (pushNotification != NULL) {
                pushNotification->NotificationCallback(1, (const wchar_t*)[[remoteNotification description]
                                                       cStringUsingEncoding:NSUTF32LittleEndianStringEncoding]);
            }
        }

        // launched from a local notification (iOS 4 and later)
        if ([[[[[UIDevice currentDevice] systemVersion] componentsSeparatedByString:@"."] objectAtIndex:0] intValue] >= 4) {
            id localNotification = [launchOptions objectForKey:UIApplicationLaunchOptionsLocalNotificationKey];
            if (localNotification != nil) {
                CLocalNotification_Cocoa::NotificationCallback(1, localNotification);
            }
        }

        [self SetTimer];
    } else {
        UIWindow* errorWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
        UIAlertView* alert = [[UIAlertView alloc] initWithTitle:@"Error"
                                                        message:@"Ouch: Looks like you've encountered an error (#1). Please restart your device or contact iphone-support@glu.com with your error number."
                                                       delegate:self
                                              cancelButtonTitle:nil
                                              otherButtonTitles:nil];
        [errorWindow addSubview:alert];
        [errorWindow makeKeyAndVisible];
        [alert show];
    }

    CApplet::m_pCore->SetBacklightEnabled(1);
    return NO;
}

- (void)applicationWillResignActive:(UIApplication*)application
{
    if (m_active) {
        m_active = false;
        if (m_pApplet != NULL) {
            [[UIApplication sharedApplication] setApplicationIconBadgeNumber:0];
            [[NSNotificationCenter defaultCenter] removeObserver:self
                                                            name:@"UIDeviceOrientationDidChangeNotification"
                                                          object:nil];
            [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
            m_pApplet->QueueSystemEvent(SystemEvent_Suspend, 0, 0);
        }
    }
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
    if (!m_active) {
        m_active = true;
        if (m_pApplet != NULL) {
            [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(DidRotate:)
                                                         name:@"UIDeviceOrientationDidChangeNotification"
                                                       object:nil];
            m_pApplet->QueueSystemEvent(SystemEvent_Resume, 0, 0);
        }
    }
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application
{
    if (m_pApplet != NULL) {
        m_pApplet->QueueSystemEvent(SystemEvent_LowMemory, 0, 0);
    }
}

- (void)applicationWillTerminate:(UIApplication*)application
{
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:0];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:@"UIDeviceOrientationDidChangeNotification"
                                                  object:nil];
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    [self CancelTimer];

    if (m_pApplet != NULL) {
        m_pApplet->Stop();
        m_pApplet->Destroy();
        delete m_pApplet;
        m_pApplet = NULL;
    }
    [window release];
    [viewController release];
}

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
    // three 21-bit fixed point values packed into the two event parameters
    unsigned int x = [self ConvertAccelToFixed:acceleration.x];
    unsigned int y = [self ConvertAccelToFixed:acceleration.y];
    unsigned int z = [self ConvertAccelToFixed:acceleration.z];
    m_pApplet->QueueSystemEvent(SystemEvent_Accelerometer,
                                (x & 0x1fffff) | (y << 21),
                                ((y & 0x1fffff) >> 11) | ((z & 0x1fffff) << 10));
}

- (unsigned int)ConvertAccelToFixed:(float)value
{
    double magnitude = fabs((double)value);
    unsigned int whole = (unsigned int)floor(magnitude);
    unsigned int fraction = (unsigned int)((magnitude - floor(magnitude)) * 65535.0);
    unsigned int fixed = ((whole << 16) & 0xf0000) | (fraction & 0xffff);
    if (value < 0) {
        fixed = -fixed;
    }
    return fixed;
}

- (void)SetTimer
{
    m_timer = [NSTimer scheduledTimerWithTimeInterval:CApplet::m_pThis->m_tickRate / 1000.0f
                                               target:self
                                             selector:@selector(HandleTimer)
                                             userInfo:nil
                                              repeats:NO];
}

- (void)CancelTimer
{
    if (m_timer != nil) {
        [m_timer invalidate];
    }
    m_timer = nil;
}

- (void)HandleTimer
{
    m_pApplet->HandleTimer();
    [self SetTimer];
}

- (void)SetNeedsDisplay
{
    [[[window subviews] objectAtIndex:0] setNeedsDisplay];
}

- (void)MakeKeyAndVisible
{
    [window makeKeyAndVisible];
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    [self CancelTimer];
    if (m_pApplet != NULL) {
        m_pApplet->QueueSystemEvent(SystemEvent_EnterBackground, 0, 0);
        // deliver it now: no more timer ticks in the background
        m_pApplet->HandleTimer();
    }
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
    if (m_pApplet != NULL) {
        m_pApplet->QueueSystemEvent(SystemEvent_EnterForeground, 0, 0);
    }
    [self SetTimer];
}

- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken
{
    CPushNotification_Cocoa* pushNotification = CPushNotification_Cocoa::GetInstance();
    if (pushNotification != NULL) {
        // "<0123abcd 4567ef01 ...>" -> "0123abcd4567ef01..."
        NSString* token = [[[[deviceToken description]
                             stringByReplacingOccurrencesOfString:@"<" withString:@""]
                            stringByReplacingOccurrencesOfString:@">" withString:@""]
                           stringByReplacingOccurrencesOfString:@" " withString:@""];
        pushNotification->RegisterWithAppleCallback(1, [token UTF8String]);
    }
}

- (void)application:(UIApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error
{
    CPushNotification_Cocoa* pushNotification = CPushNotification_Cocoa::GetInstance();
    if (pushNotification != NULL) {
        pushNotification->RegisterWithAppleCallback(0, [[error localizedDescription] UTF8String]);
    }
}

- (void)application:(UIApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo
{
    CPushNotification_Cocoa* pushNotification = CPushNotification_Cocoa::GetInstance();
    if (pushNotification != NULL) {
        pushNotification->NotificationCallback(0, (const wchar_t*)[[userInfo description]
                                               cStringUsingEncoding:NSUTF32LittleEndianStringEncoding]);
    }
}

- (void)application:(UIApplication*)application didReceiveLocalNotification:(UILocalNotification*)notification
{
    CLocalNotification_Cocoa::NotificationCallback(0, notification);
}

- (void)DidRotate:(NSNotification*)notification
{
    ICCore* core = CApplet::m_pCore;
    ICCore::eAutoRotationOrientation autoRotation;
    ICCore::eDeviceOrientation deviceOrientation;

    switch ([[UIDevice currentDevice] orientation]) {
    case UIDeviceOrientationPortrait:
        autoRotation = ICCore::AutoRotation_Portrait;
        deviceOrientation = ICCore::DeviceOrientation_Portrait;
        break;
    case UIDeviceOrientationPortraitUpsideDown:
        autoRotation = ICCore::AutoRotation_PortraitUpsideDown;
        deviceOrientation = ICCore::DeviceOrientation_PortraitUpsideDown;
        break;
    case UIDeviceOrientationLandscapeLeft:
        autoRotation = ICCore::AutoRotation_LandscapeLeft;
        deviceOrientation = ICCore::DeviceOrientation_LandscapeLeft;
        break;
    case UIDeviceOrientationLandscapeRight:
        autoRotation = ICCore::AutoRotation_LandscapeRight;
        deviceOrientation = ICCore::DeviceOrientation_LandscapeRight;
        break;
    default:
        // face up / face down / unknown: keep the current orientation
        return;
    }

    if (core->AutoRotationOrientationIsEnabled(autoRotation)) {
        core->SetDeviceOrientation(deviceOrientation);
        CApplet::m_pThis->HandleTimer();
    }
}

@end
