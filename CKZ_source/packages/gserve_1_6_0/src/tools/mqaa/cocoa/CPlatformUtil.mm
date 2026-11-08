// CPlatformUtil.mm - device information for the gServe message layer.
#include "CPlatformUtil.h"
#import <UIKit/UIKit.h>

using com::glu::platform::components::CStrWChar;

CStrWChar CPlatformUtil::getDeviceName()
{
    // Original quirk: despite the name, this is the device model ("iPhone", "iPod touch", ...).
    const char* model = [[[UIDevice currentDevice] model] cStringUsingEncoding:NSASCIIStringEncoding];
    return CStrWChar(model);
}
