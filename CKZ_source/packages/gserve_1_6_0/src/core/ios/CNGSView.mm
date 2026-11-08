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
    if (orientation == ICCore::DeviceOrientation_LandscapeRight) {
        transform = CGAffineTransformConcat(CGAffineTransformMakeRotation(-M_PI_2),
                                            CGAffineTransformMakeTranslation(-m_offset, 0));
    } else if (orientation == ICCore::DeviceOrientation_LandscapeLeft) {
        transform = CGAffineTransformConcat(CGAffineTransformMakeRotation(M_PI_2),
                                            CGAffineTransformMakeTranslation(m_offset, 0));
    }
}

@end
