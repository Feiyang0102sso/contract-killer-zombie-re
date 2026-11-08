// malij297_dynamicimage2d_runtime.cpp - bridges Glu render surfaces into Swerve (M3G) Image2D objects.
// "Data" binding reads the surface back into a new RGBA pixel copy; "Ref" binding shares the
// surface's OpenGL texture with the runtime.
#include <OpenGLES/ES1/gl.h>
#include "ICGraphics.h"
#include "ICRenderSurface.h"
#include "CRenderSurface_OGLES_Texture.h"
#include "SwerveApi.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;

// ---- constants ----
/** Image formats of the Swerve runtime. */
static const unsigned int IMAGE_FORMAT_RGB888 = 99;
static const unsigned int IMAGE_FORMAT_RGBA8888 = 100;
static const unsigned int IMAGE_FORMAT_RGB565 = 101;
/** ColorFormat bit set by formats that carry an alpha channel (e.g. ColorFormat_A8R8G8B8). */
static const unsigned int COLOR_FORMAT_ALPHA_BIT = 0x4000;

extern "C" {

/** Read `surface` back into a new RGBA pixel copy described by `image`. Returns 1 on success. */
int malij297_DynamicImage2DBindData(ICRenderSurface* surface, void* context, malij297_DynamicImage2D* image)
{
    if (surface == NULL) {
        return 0;
    }

    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    int size = width * height * 4;
    unsigned char* pixels = (unsigned char*)malloc(size);
    if (pixels == NULL) {
        return 0;
    }

    // Render the surface as target for the read back, then restore the previous target.
    ICRenderSurface* previousTarget = ICGraphics::GetInstance()->GetTargetSurface();
    ICGraphics::GetInstance()->SetTargetSurface(*surface);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // A surface without alpha may read back garbage alpha: make it opaque.
    if ((surface->GetColorFormat() & COLOR_FORMAT_ALPHA_BIT) == 0 && pixels[3] != 0xff) {
        unsigned char* alpha = pixels + 3;
        unsigned char* alphaEnd = alpha + size;
        while (alpha < alphaEnd) {
            *alpha = 0xff;
            alpha += 4;
        }
    }
    ICGraphics::GetInstance()->SetTargetSurface(*previousTarget);

    image->m_bound = 1;
    image->m_width = width;
    image->m_height = height;
    image->m_format = IMAGE_FORMAT_RGBA8888;
    image->m_pixels = pixels;
    image->m_size = size;
    return 1;
}

/** Free the pixel copy made by DynamicImage2DBindData. */
void malij297_DynamicImage2DReleaseData(ICRenderSurface* surface, void* context, malij297_DynamicImage2D* image)
{
    if (surface == NULL) {
        return;
    }
    if (image->m_pixels != NULL) {
        free(image->m_pixels);
    }
}

/** Describe `surface` for sharing its texture. Returns 0 for formats the runtime cannot use. */
int malij297_DynamicImage2DBindRef(ICRenderSurface* surface, malij297_DynamicImage2D* image)
{
    if (surface == NULL) {
        return 0;
    }

    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    unsigned int format;
    switch (surface->GetColorFormat()) {
    case ColorFormat_R8G8B8:
        format = IMAGE_FORMAT_RGB888;
        break;
    case ColorFormat_A8R8G8B8:
        format = IMAGE_FORMAT_RGBA8888;
        break;
    case ColorFormat_R5G6B5:
        format = IMAGE_FORMAT_RGB565;
        break;
    default:
        return 0;
    }

    image->m_bound = 1;
    image->m_width = width;
    image->m_height = height;
    image->m_format = format;
    image->m_pixels = NULL;
    return 1;
}

/** Nothing to release: the texture stays owned by the surface. */
void malij297_DynamicImage2DReleaseRef()
{
}

/** Refresh the description of a shared surface and bind its (first) texture. Returns 1 on success. */
int malij297_DynamicImage2DRebindRef(ICRenderSurface* surface, void* context, malij297_DynamicImage2D* image)
{
    if (surface == NULL) {
        return 0;
    }

    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    unsigned int format;
    switch (surface->GetColorFormat()) {
    case ColorFormat_R8G8B8:
        format = IMAGE_FORMAT_RGB888;
        break;
    case ColorFormat_A8R8G8B8:
        format = IMAGE_FORMAT_RGBA8888;
        break;
    case ColorFormat_R5G6B5:
        format = IMAGE_FORMAT_RGB565;
        break;
    default:
        return 0;
    }

    // The runtime shares the OpenGL texture, so the surface must have one.
    CRenderSurface_OGLES_Texture* texture = (CRenderSurface_OGLES_Texture*)surface;
    if (texture->m_textureCount < 1) {
        return 0;
    }
    image->m_bound = 1;
    image->m_width = width;
    image->m_height = height;
    image->m_format = format;
    // One texture is stored inline, tiled surfaces keep an array of names.
    unsigned int* textureNames;
    if (texture->m_textureCount == 1) {
        textureNames = &texture->m_texture;
    } else {
        textureNames = texture->m_textures;
    }
    glBindTexture(GL_TEXTURE_2D, textureNames[0]);
    return 1;
}

}
