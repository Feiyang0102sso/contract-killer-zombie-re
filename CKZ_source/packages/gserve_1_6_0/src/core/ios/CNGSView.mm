// CNGSView.mm - base view of the gServe dialogs.
#import "CNGSView.h"
#include "CApplet.h"
#include "ICCore.h"

using com::glu::platform::framework::ICCore;

/** Default dialog size in points. */
static const int DEFAULT_DIALOG_WIDTH = 320;
static const int DEFAULT_DIALOG_HEIGHT = 155;

@implementation CNGSView

- (void)loadView
{
    NSLog(@"CNGSView loadView");
    if (m_width == 0) {
        m_width = DEFAULT_DIALOG_WIDTH;
    }
    if (m_height == 0) {
        m_height = DEFAULT_DIALOG_HEIGHT;
    }
    m_backgroundImage = [UIImage imageNamed:@"alert_background.png"];
    m_buttonImage = [UIImage imageNamed:@"footnote_button.png"];

    // centre the dialog box on the screen
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    unsigned int screenWidth = screenBounds.size.width;
    unsigned int screenHeight = screenBounds.size.height;
    m_dialog_box_x = (screenWidth - m_width) / 2;
    m_dialog_box_y = (screenHeight - m_height) / 2;

    // follow the device orientation in landscape
    ICCore::eDeviceOrientation orientation = CApplet::m_pCore->GetDeviceOrientation();
    // b147: LandscapeLeft written first (block order of the original); the rotation is a named local
    // (the original copies it into an 8-byte aligned argument temporary)
    if (orientation == ICCore::DeviceOrientation_LandscapeLeft) {
        CGAffineTransform rotation = CGAffineTransformMakeRotation(M_PI_2);
        transform = CGAffineTransformConcat(rotation, CGAffineTransformMakeTranslation(m_offset, 0));
    } else if (orientation == ICCore::DeviceOrientation_LandscapeRight) {
        CGAffineTransform rotation = CGAffineTransformMakeRotation(-M_PI_2);
        transform = CGAffineTransformConcat(rotation, CGAffineTransformMakeTranslation(-m_offset, 0));
    }
}

@end
