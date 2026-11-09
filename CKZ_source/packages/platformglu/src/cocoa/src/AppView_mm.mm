// AppView_mm.mm - application views (see AppView.h): touches become CApplet system events in the
// coordinates of the current device orientation, scaled to the display.
#include <math.h>
#import <QuartzCore/QuartzCore.h>
#import "AppView.h"
#include "CApplet.h"
#include "ICCore.h"
#include "CMathFixed.h"

using namespace com::glu::platform::framework;
using namespace com::glu::platform::math;

// ---- constants ----
static const unsigned int SystemEvent_TouchBegan = 0x35c43e56;
static const unsigned int SystemEvent_TouchMoved = 0x35cb8f1c;
static const unsigned int SystemEvent_TouchEnded = 0x35c38d1c;
static const unsigned int SystemEvent_TouchCancelled = 0x5707b968;

/** Window coordinates -> display coordinates, then to pixels with the view's content scale. */
static void ScaleCoordinates(int& x, int& y, float viewScale)
{
    const CVector2dx& scale = CApplet::m_pCore->GetDisplayToWindowScale();
    x = CMathFixed::Mul(x, scale.x);
    y = CMathFixed::Mul(y, scale.y);
    x = (int)((float)x * viewScale);
    y = (int)((float)y * viewScale);
}

@implementation AppView

- (id)initWithCoder:(NSCoder*)coder
{
    m_viewScale = 1.0f;
    if ([super initWithCoder:coder] == self) {
        if ([self respondsToSelector:@selector(contentScaleFactor)]) {
            m_viewScale = [[UIScreen mainScreen] scale];
            [self setContentScaleFactor:m_viewScale];
        }
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
    CApplet* applet = CApplet::m_pThis;
    for (UITouch* touch in touches) {
        CGPoint location = [touch locationInView:self];
        CGPoint point = [self convertPoint:location toView:nil];
        int x = (int)floorf(point.x);
        int y = (int)floorf(point.y);

        // view coordinates -> coordinates of the current device orientation
        ICCore* core = CApplet::m_pCore;
        if (core->m_deviceOrientationSet) {
            if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeLeft) {
                int previousX = x;
                x = y;
                y = m_width - previousX;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_PortraitUpsideDown) {
                x = m_width - x;
                y = m_height - y;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeRight) {
                int previousX = x;
                x = m_height - y;
                y = previousX;
            }
        }
        ScaleCoordinates(x, y, m_viewScale);

        // param2: tap count (2 bits), y (14 bits), x (14 bits)
        applet->QueueSystemEvent(SystemEvent_TouchBegan, (unsigned int)touch,
                                 (([touch tapCount] & 3) << 28) | (x & 0x3fff) | ((y & 0x3fff) << 14));
    }
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
    CApplet* applet = CApplet::m_pThis;
    for (UITouch* touch in touches) {
        CGPoint location = [touch locationInView:self];
        CGPoint point = [self convertPoint:location toView:nil];
        int x = (int)floorf(point.x);
        int y = (int)floorf(point.y);

        // view coordinates -> coordinates of the current device orientation
        ICCore* core = CApplet::m_pCore;
        if (core->m_deviceOrientationSet) {
            if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeLeft) {
                int previousX = x;
                x = y;
                y = m_width - previousX;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_PortraitUpsideDown) {
                x = m_width - x;
                y = m_height - y;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeRight) {
                int previousX = x;
                x = m_height - y;
                y = previousX;
            }
        }
        ScaleCoordinates(x, y, m_viewScale);

        // param2: tap count (2 bits), y (14 bits), x (14 bits)
        applet->QueueSystemEvent(SystemEvent_TouchMoved, (unsigned int)touch,
                                 (([touch tapCount] & 3) << 28) | (x & 0x3fff) | ((y & 0x3fff) << 14));
    }
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
    CApplet* applet = CApplet::m_pThis;
    for (UITouch* touch in touches) {
        CGPoint location = [touch locationInView:self];
        CGPoint point = [self convertPoint:location toView:nil];
        int x = (int)floorf(point.x);
        int y = (int)floorf(point.y);

        // view coordinates -> coordinates of the current device orientation
        ICCore* core = CApplet::m_pCore;
        if (core->m_deviceOrientationSet) {
            if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeLeft) {
                int previousX = x;
                x = y;
                y = m_width - previousX;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_PortraitUpsideDown) {
                x = m_width - x;
                y = m_height - y;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeRight) {
                int previousX = x;
                x = m_height - y;
                y = previousX;
            }
        }
        ScaleCoordinates(x, y, m_viewScale);

        // param2: tap count (2 bits), y (14 bits), x (14 bits)
        applet->QueueSystemEvent(SystemEvent_TouchEnded, (unsigned int)touch,
                                 (([touch tapCount] & 3) << 28) | (x & 0x3fff) | ((y & 0x3fff) << 14));
    }
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
    CApplet* applet = CApplet::m_pThis;
    for (UITouch* touch in touches) {
        applet->QueueSystemEvent(SystemEvent_TouchCancelled, (unsigned int)touch, 0);
    }
}

- (BOOL)isMultipleTouchEnabled
{
    return CApplet::m_pThis->m_multipleTouchEnabled == 1;
}

@end

@implementation AppGLView

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame
{
    m_viewScale = 1.0f;
    if ([super initWithFrame:frame] == self) {
        if ([self respondsToSelector:@selector(contentScaleFactor)]) {
            m_viewScale = [[UIScreen mainScreen] scale];
            [self setContentScaleFactor:m_viewScale];
        }

        CGRect screenBounds = [[UIScreen mainScreen] bounds];
        CGPoint corner = [self convertPoint:CGPointMake(screenBounds.size.width, screenBounds.size.height) toView:nil];
        m_width = (int)floorf(corner.x);
        m_height = (int)floorf(corner.y);
        CApplet::m_eaglLayer = [self layer];
        CApplet::m_UIScreenMainScreenWidth = (short)(int)((int)screenBounds.size.width * m_viewScale);
        CApplet::m_UIScreenMainScreenHeight = (short)(int)((int)screenBounds.size.height * m_viewScale);
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
    CApplet* applet = CApplet::m_pThis;
    for (UITouch* touch in touches) {
        CGPoint location = [touch locationInView:self];
        CGPoint point = [self convertPoint:location toView:nil];
        int x = (int)floorf(point.x);
        int y = (int)floorf(point.y);

        // view coordinates -> coordinates of the current device orientation
        ICCore* core = CApplet::m_pCore;
        if (core->m_deviceOrientationSet) {
            if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeLeft) {
                int previousX = x;
                x = y;
                y = m_width - previousX;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_PortraitUpsideDown) {
                x = m_width - x;
                y = m_height - y;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeRight) {
                int previousX = x;
                x = m_height - y;
                y = previousX;
            }
        }
        ScaleCoordinates(x, y, m_viewScale);

        // param2: tap count (2 bits), y (14 bits), x (14 bits)
        applet->QueueSystemEvent(SystemEvent_TouchBegan, (unsigned int)touch,
                                 (([touch tapCount] & 3) << 28) | (x & 0x3fff) | ((y & 0x3fff) << 14));
    }
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
    CApplet* applet = CApplet::m_pThis;
    for (UITouch* touch in touches) {
        CGPoint location = [touch locationInView:self];
        CGPoint point = [self convertPoint:location toView:nil];
        int x = (int)floorf(point.x);
        int y = (int)floorf(point.y);

        // view coordinates -> coordinates of the current device orientation
        ICCore* core = CApplet::m_pCore;
        if (core->m_deviceOrientationSet) {
            if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeLeft) {
                int previousX = x;
                x = y;
                y = m_width - previousX;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_PortraitUpsideDown) {
                x = m_width - x;
                y = m_height - y;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeRight) {
                int previousX = x;
                x = m_height - y;
                y = previousX;
            }
        }
        ScaleCoordinates(x, y, m_viewScale);

        // param2: tap count (2 bits), y (14 bits), x (14 bits)
        applet->QueueSystemEvent(SystemEvent_TouchMoved, (unsigned int)touch,
                                 (([touch tapCount] & 3) << 28) | (x & 0x3fff) | ((y & 0x3fff) << 14));
    }
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
    CApplet* applet = CApplet::m_pThis;
    for (UITouch* touch in touches) {
        CGPoint location = [touch locationInView:self];
        CGPoint point = [self convertPoint:location toView:nil];
        int x = (int)floorf(point.x);
        int y = (int)floorf(point.y);

        // view coordinates -> coordinates of the current device orientation
        ICCore* core = CApplet::m_pCore;
        if (core->m_deviceOrientationSet) {
            if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeLeft) {
                int previousX = x;
                x = y;
                y = m_width - previousX;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_PortraitUpsideDown) {
                x = m_width - x;
                y = m_height - y;
            } else if (core->m_deviceOrientation == ICCore::DeviceOrientation_LandscapeRight) {
                int previousX = x;
                x = m_height - y;
                y = previousX;
            }
        }
        ScaleCoordinates(x, y, m_viewScale);

        // param2: tap count (2 bits), y (14 bits), x (14 bits)
        applet->QueueSystemEvent(SystemEvent_TouchEnded, (unsigned int)touch,
                                 (([touch tapCount] & 3) << 28) | (x & 0x3fff) | ((y & 0x3fff) << 14));
    }
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
    CApplet* applet = CApplet::m_pThis;
    for (UITouch* touch in touches) {
        applet->QueueSystemEvent(SystemEvent_TouchCancelled, (unsigned int)touch, 0);
    }
}

- (BOOL)isMultipleTouchEnabled
{
    return CApplet::m_pThis->m_multipleTouchEnabled == 1;
}

@end
