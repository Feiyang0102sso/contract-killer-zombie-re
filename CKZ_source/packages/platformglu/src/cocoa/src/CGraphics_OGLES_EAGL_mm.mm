// CGraphics_OGLES_EAGL_mm.mm - EAGL calls of the OpenGL ES platform glue (see CGraphics_OGLES_EAGL.h).
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/glext.h>
#include "CGraphics_OGLES_EAGL.h"

void* CGraphics_OGLES_EAGL_CreateContext(void* layer, unsigned int api, NSString* colorFormat)
{
    CAEAGLLayer* eaglLayer = (CAEAGLLayer*)layer;
    EAGLContext* context = nil;

    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:NO], @"false",
                                    colorFormat, kEAGLDrawablePropertyColorFormat,
                                    nil];

    if (api == 1) {
        context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    } else if (api == 2) {
        context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    }
    return context;
}

void* CGraphics_OGLES_EAGL_CreateContext_R5G6B5(void* layer, unsigned int api)
{
    return CGraphics_OGLES_EAGL_CreateContext(layer, api, kEAGLColorFormatRGB565);
}

void* CGraphics_OGLES_EAGL_CreateContext_A8B8G8R8(void* layer, unsigned int api)
{
    return CGraphics_OGLES_EAGL_CreateContext(layer, api, kEAGLColorFormatRGBA8);
}

void CGraphics_OGLES_EAGL_ReleaseContext(void* context)
{
    [(EAGLContext*)context release];
}

void CGraphics_OGLES_EAGL_SetCurrentContext(void* context)
{
    [EAGLContext setCurrentContext:(EAGLContext*)context];
}

unsigned char CGraphics_OGLES_EAGL_RenderBufferStorage(void* layer, void* context)
{
    return [(EAGLContext*)context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(id<EAGLDrawable>)layer] != NO;
}

void CGraphics_OGLES_EAGL_PresentRenderBuffer(void* context)
{
    [(EAGLContext*)context presentRenderbuffer:GL_RENDERBUFFER_OES];
}
