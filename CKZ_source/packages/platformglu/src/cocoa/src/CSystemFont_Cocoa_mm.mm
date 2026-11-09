// CSystemFont_Cocoa_mm.mm - system font drawn with UIKit; text is rendered into a PNG that
// initializes a render surface.
#import <UIKit/UIKit.h>
#include "CSystemFont_Cocoa.h"

#include <wchar.h>
#include "ICRenderSurface.h"
#include "CArrayInputStream.h"

using namespace com::glu::platform;

/** Text encoding of wchar_t strings on iOS: NSUTF32LittleEndianStringEncoding. */
static const NSStringEncoding kWideStringEncoding = 0x9c000100;

/** Share of the point size used as the minimal character spacing. */
static const float kCharSpacingFactor = 0.1f;

/** Surface settings for a PNG image (key / value pairs; values from the original). */
enum {
    SurfaceParam_Format = 6,
    SurfaceParam_Stream = 0xe,
    SurfaceParam_MimeType = 0xf,
    SurfaceParam_15 = 0x15,
    SurfaceParam_16 = 0x16
};
static const unsigned int kSurfaceFormatPng = 0x75402;
static const unsigned int kMimeTypePng = 0xb7178678;

CSystemFont_Cocoa::CSystemFont_Cocoa()
{
}

CSystemFont_Cocoa::~CSystemFont_Cocoa()
{
    CSystemFont::Destroy();
}

void CSystemFont_Cocoa::Destroy()
{
    CSystemFont::Destroy();
}

int CSystemFont_Cocoa::GetFontHeight() const
{
    if (m_font == nil) {
        return 0;
    }
    return (int)[m_font pointSize];
}

int CSystemFont_Cocoa::GetMinLineSpacing() const
{
    if (m_font == nil) {
        return 0;
    }
    return (int)([m_font leading] - (float)GetFontHeight());
}

int CSystemFont_Cocoa::GetMinCharSpacing() const
{
    return (int)([m_font pointSize] * kCharSpacingFactor);
}

/** New (retained) NSString of the first `length` characters of `text` (length < 0: all). */
static NSString* Unicode2NSString(const wchar_t* text, int length)
{
    int count = wcslen(text);
    if (length >= 0) {
        if (count < length) {
            length = count;
        }
        count = length;
    }
    return [[NSString alloc] initWithBytes:text length:count * sizeof(wchar_t) encoding:kWideStringEncoding];
}

int CSystemFont_Cocoa::GetWidth(wchar_t character) const
{
    wchar_t text[2];
    text[0] = character;
    text[1] = 0;
    NSString* string = Unicode2NSString(text, 2);
    CGSize size = [string sizeWithFont:m_font];
    int width = (int)size.width;
    [string release];
    return width;
}

int CSystemFont_Cocoa::MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount)
{
    int width = 0;
    int count = 0;
    if (m_font != nil && text != NULL) {
        if (length < 0) {
            length = wcslen(text);
        }
        // grow the measured prefix until it no longer fits
        int measured = 0;
        int next = 0;
        for (;;) {
            count = next;
            width = measured;
            if (count >= length) {
                break;
            }
            next = count + 1;
            NSString* prefix = Unicode2NSString(text, next);
            CGSize size = [prefix sizeWithFont:m_font];
            measured = (int)size.width;
            [prefix release];
            if (maxWidth >= 0 && measured > maxWidth) {
                break;
            }
        }
    }
    if (fitCount != NULL) {
        *fitCount = count;
    }
    return width;
}

unsigned char CSystemFont_Cocoa::InitializeSurfaceWithText(graphics::ICRenderSurface& surface, const wchar_t* text,
                                                           int length, int maxWidth, int maxHeight)
{
    if (m_font == nil || text == NULL) {
        return 0;
    }
    NSString* string = Unicode2NSString(text, length);
    CGSize size = [string sizeWithFont:m_font];
    int width = (unsigned int)size.width;
    if (maxWidth >= 0) {
        width = MIN(maxWidth, width);
    }
    unsigned int height = GetFontHeight() + GetMinLineSpacing();
    if (maxHeight >= 0) {
        height = MIN(height, (unsigned int)maxHeight);
    }
    CreatePngWithText(string, width, height, surface);
    [string release];
    return 1;
}

unsigned char CSystemFont_Cocoa::CreatePngWithText(const NSString* text, int width, int height,
                                                   graphics::ICRenderSurface& surface)
{
    CGSize size = CGSizeMake(width, height);
    UIGraphicsBeginImageContext(size);
    CGContextRef context = UIGraphicsGetCurrentContext();
    if (context != NULL) {
        CGContextSetAllowsAntialiasing(context, m_antialias != 0);
        // transparent background, white text
        CGContextSetFillColorWithColor(context, [[UIColor colorWithRed:0 green:0 blue:0 alpha:0] CGColor]);
        CGContextFillRect(context, CGRectMake(0, 0, size.width, size.height));
        CGContextSetFillColorWithColor(context, [[UIColor colorWithRed:1 green:1 blue:1 alpha:1] CGColor]);
        [(NSString*)text drawAtPoint:CGPointMake(0, 0) withFont:m_font];
        UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
        if (image != nil) {
            NSData* png = UIImagePNGRepresentation(image);
            const void* bytes = [png bytes];
            unsigned int pngLength = [png length];
            components::CArrayInputStream stream;
            stream.Open((const unsigned char*)bytes, pngLength);
            graphics::ICRenderSurface::InitParam params[] = {
                { SurfaceParam_Format, kSurfaceFormatPng },
                { SurfaceParam_Stream, (unsigned int)&stream },
                { SurfaceParam_MimeType, kMimeTypePng },
                { SurfaceParam_15, 0 },
                { SurfaceParam_16, 0 },
                { 0, 0 }
            };
            surface.Initialize(params);
        }
    }
    UIGraphicsEndImageContext();
    return 1;
}

unsigned char CSystemFont_Cocoa::FinalizeInitialization(const unsigned char* data, unsigned int size)
{
    if (Parse(data, size)) {
        if (m_fontName.m_length != 0) {
            NSString* name = Unicode2NSString(m_fontName.m_string, -1);
            m_font = [UIFont fontWithName:name size:(float)m_pointSize];
            [name release];
        } else {
            m_font = [UIFont systemFontOfSize:(float)m_pointSize];
        }
    }
    return m_font != nil;
}
