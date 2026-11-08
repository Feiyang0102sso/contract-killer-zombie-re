// AppViewController.mm - view controller that holds the GL view while auto-rotation is enabled:
// accepts the orientations the application enabled and turns the GL view to match.
#include <math.h>
#import "AppViewController.h"
#import "AppView.h"
#include "CApplet.h"
#include "ICCore.h"

using namespace com::glu::platform::framework;

@implementation AppViewController

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    ICCore* core = CApplet::m_pCore;
    bool shouldRotate = false;

    switch (interfaceOrientation) {
    case UIInterfaceOrientationPortrait: {
        unsigned char enabled = core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_Portrait);
        if (!enabled) {
            // portrait is the fallback when nothing else is enabled
            enabled = !core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_LandscapeRight) &&
                      !core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_PortraitUpsideDown) &&
                      !core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_LandscapeLeft);
        }
        shouldRotate = enabled != 0;
        break;
    }
    case UIInterfaceOrientationPortraitUpsideDown:
        shouldRotate = core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_PortraitUpsideDown) != 0;
        break;
    case UIInterfaceOrientationLandscapeRight:
        shouldRotate = core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_LandscapeLeft) != 0;
        break;
    case UIInterfaceOrientationLandscapeLeft:
        shouldRotate = core->AutoRotationOrientationIsEnabled(ICCore::AutoRotation_LandscapeRight) != 0;
        break;
    }

    if (shouldRotate) {
        UIView* rootView = [self view];
        for (UIView* view in [rootView subviews]) {
            if ([view isKindOfClass:[AppGLView class]]) {
                core = CApplet::m_pCore;
                CGRect screenBounds = [[UIScreen mainScreen] bounds];
                CGPoint corner = [view convertPoint:CGPointMake(screenBounds.size.width, screenBounds.size.height) toView:nil];
                float cornerX = floorf(corner.x);
                float cornerY = floorf(corner.y);

                // turn the GL view around the screen centre
                CGAffineTransform transform;
                switch (interfaceOrientation) {
                case UIInterfaceOrientationPortrait:
                    core->SetDeviceOrientation(ICCore::DeviceOrientation_Portrait);
                    transform = CGAffineTransformIdentity;
                    break;
                case UIInterfaceOrientationPortraitUpsideDown:
                    core->SetDeviceOrientation(ICCore::DeviceOrientation_PortraitUpsideDown);
                    transform = CGAffineTransformMakeRotation(M_PI);
                    break;
                case UIInterfaceOrientationLandscapeRight:
                    core->SetDeviceOrientation(ICCore::DeviceOrientation_LandscapeLeft);
                    [view setTransform:CGAffineTransformMakeTranslation((cornerY - cornerX) * 0.5f,
                                                                        (cornerX - cornerY) * 0.5f)];
                    transform = CGAffineTransformRotate([view transform], -M_PI_2);
                    break;
                case UIInterfaceOrientationLandscapeLeft:
                    core->SetDeviceOrientation(ICCore::DeviceOrientation_LandscapeRight);
                    [view setTransform:CGAffineTransformMakeTranslation((cornerY - cornerX) * 0.5f,
                                                                        (cornerX - cornerY) * 0.5f)];
                    transform = CGAffineTransformRotate([view transform], M_PI_2);
                    break;
                default:
                    transform = CGAffineTransformIdentity;
                    break;
                }
                [view setTransform:transform];
                break;
            }
        }
    }
    return shouldRotate;
}

@end
