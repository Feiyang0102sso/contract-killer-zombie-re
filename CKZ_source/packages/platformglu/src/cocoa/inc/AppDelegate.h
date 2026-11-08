// AppDelegate.h - UIApplication delegate of the Glu application: creates the GL view and the
// CApplet, forwards lifecycle, accelerometer, orientation and notification callbacks, and runs
// the frame timer.
#import <UIKit/UIKit.h>

class CApplet;
#import "AppViewController.h"

@interface AppDelegate : NSObject <UIApplicationDelegate, UIAccelerometerDelegate> {
    UIWindow* window;                       // main window (from the nib)
    AppViewController* viewController;      // auto-rotation controller
    NSTimer* m_timer;                       // frame timer
    CApplet* m_pApplet;                     // platform host
    bool m_active;                          // the application is active
}

@property (nonatomic, retain) IBOutlet UIWindow* window;
@property (nonatomic, retain) AppViewController* viewController;

/** 16.16 fixed point from an accelerometer value (sign and magnitude, 4 integer bits). */
- (unsigned int)ConvertAccelToFixed:(float)value;
/** Schedule the next frame after the applet tick rate. */
- (void)SetTimer;
- (void)CancelTimer;
/** Run one frame and schedule the next one. */
- (void)HandleTimer;
- (void)SetNeedsDisplay;
- (void)MakeKeyAndVisible;
/** UIDeviceOrientationDidChangeNotification. */
- (void)DidRotate:(NSNotification*)notification;

@end
