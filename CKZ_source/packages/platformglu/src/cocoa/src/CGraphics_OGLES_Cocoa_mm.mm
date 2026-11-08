// CGraphics_OGLES_Cocoa_mm.mm - Objective-C side of the EAGL graphics device: EAGL version query.
#import <OpenGLES/EAGL.h>

/** EAGL framework version (major, minor). */
void CGraphics_OGLES_Cocoa_GetEAGLVersion(unsigned int* major, unsigned int* minor)
{
    EAGLGetVersion(major, minor);
}
