// Native Swerve Image2D (IImage2D of SwerveApi.h), used by the PNG / JPEG / KTX loaders of the
// porting layer: CreateObject(Image2D), Initialize, SetSubImage or SetPalettedImage, Commit.
// m3gcore wants the whole image when it is created, so the pixels are collected first and the
// m3gcore image is made in Commit.
#include "NativeRuntime.h"
#include <stdio.h>
#include <string.h>
#include <vector>

namespace ckz::native {
namespace {

// Image formats (Swerve and M3G share the values 0x60-0x64).
constexpr unsigned int FormatAlpha = 0x60;
constexpr unsigned int FormatLuminance = 0x61;
constexpr unsigned int FormatLuminanceAlpha = 0x62;
constexpr unsigned int FormatRGB = 0x63;
constexpr unsigned int FormatRGBA = 0x64;
// Initialize mode of a paletted image (malij297_pngload.cpp).
constexpr int ModePaletted = 2;
constexpr int MaxPaletteEntries = 256;

struct ImageObject : Object {
    unsigned int format;
    int width;
    int height;
    int mode;
    std::vector<unsigned char> pixels;    // width * height * BytesPerPixel(format), or palette indices
    std::vector<unsigned char> palette;   // entries of paletteFormat
    unsigned int paletteFormat;
};

ImageObject* AsImage(Object* object)
{
    return static_cast<ImageObject*>(object);
}

void DestroyImage(Object* object)
{
    delete AsImage(object);
}

int BytesPerPixel(unsigned int format)
{
    switch (format) {
    case FormatAlpha:
    case FormatLuminance:
        return 1;
    case FormatLuminanceAlpha:
        return 2;
    case FormatRGB:
        return 3;
    case FormatRGBA:
        return 4;
    default:
        return 0;
    }
}

int ImageInitialize(Object* self, unsigned int format, unsigned int width, unsigned int height, int mode)
{
    ImageObject* image = AsImage(self);
    if (self->handle != nullptr) {
        return ErrorInvalidState;
    }
    const int bytes = BytesPerPixel(format);
    if (bytes == 0 || width == 0 || height == 0) {
        fprintf(stderr, "Swerve native: Image2D format 0x%x %ux%u is not supported\n", format, width, height);
        return ErrorInvalidParameter;
    }
    image->format = format;
    image->width = static_cast<int>(width);
    image->height = static_cast<int>(height);
    image->mode = mode;
    image->pixels.assign(static_cast<size_t>(width) * height * static_cast<size_t>(bytes), 0);
    return ErrorNone;
}

int ImageInitializeCompressed(Object*, unsigned int format, unsigned int width, unsigned int height,
                              unsigned int glType, unsigned int glFormat)
{
    fprintf(stderr, "Swerve native: compressed Image2D (0x%x %ux%u, GL 0x%x/0x%x) is not supported\n",
            format, width, height, glType, glFormat);
    return ErrorClassNotSupported;
}

/** Copy a rectangle of pixels into the image; only level 0 matters (m3gcore builds the mipmaps). */
int ImageSetSubImage(Object* self, unsigned int dataFormat, int level, int x, int y, int width, int height,
                     unsigned int dataSize, const void* data, int)
{
    ImageObject* image = AsImage(self);
    if (level != 0) {
        return ErrorNone;
    }
    if (dataFormat != image->format) {
        fprintf(stderr, "Swerve native: Image2D data format 0x%x into a 0x%x image is not supported\n",
                dataFormat, image->format);
        return ErrorInvalidParameter;
    }
    if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > image->width || y + height > image->height) {
        return ErrorInvalidParameter;
    }
    const size_t pixelBytes = static_cast<size_t>(BytesPerPixel(image->format));
    const size_t rowBytes = static_cast<size_t>(width) * pixelBytes;
    if (dataSize < rowBytes * static_cast<size_t>(height)) {
        return ErrorInvalidParameter;
    }
    const auto* source = static_cast<const unsigned char*>(data);
    for (int row = 0; row < height; row++) {
        unsigned char* destination = image->pixels.data() +
            (static_cast<size_t>(y + row) * static_cast<size_t>(image->width) + static_cast<size_t>(x)) * pixelBytes;
        memcpy(destination, source + static_cast<size_t>(row) * rowBytes, rowBytes);
    }
    return ErrorNone;
}

/** Palette indices (one byte per pixel) and a palette of `format` entries (paletteSize bytes). */
int ImageSetPalettedImage(Object* self, unsigned int format, int level, unsigned int indexSize,
                          const void* indices, unsigned int paletteSize, const void* palette)
{
    ImageObject* image = AsImage(self);
    if (level != 0) {
        return ErrorNone;
    }
    const unsigned int pixelCount = static_cast<unsigned int>(image->width * image->height);
    const int entryBytes = BytesPerPixel(format);
    if (indexSize < pixelCount || entryBytes == 0) {
        return ErrorInvalidParameter;
    }
    image->pixels.assign(static_cast<const unsigned char*>(indices),
                         static_cast<const unsigned char*>(indices) + pixelCount);
    image->palette.assign(static_cast<const unsigned char*>(palette),
                          static_cast<const unsigned char*>(palette) + paletteSize);
    image->paletteFormat = format;
    return ErrorNone;
}

/** Create the m3gcore image from the collected pixels and make it immutable. */
int ImageCommit(Object* self)
{
    ImageObject* image = AsImage(self);
    if (self->handle != nullptr) {
        return ErrorNone;
    }
    if (image->width == 0) {
        return ErrorInvalidState;
    }
    TakeError();
    const bool paletted = !image->palette.empty();
    M3GImage created = nullptr;
    if (paletted) {
        // A paletted M3G image takes its format from the palette entries.
        int entries = static_cast<int>(image->palette.size()) / BytesPerPixel(image->paletteFormat);
        if (entries > MaxPaletteEntries) {
            entries = MaxPaletteEntries;
        }
        created = m3gCreateImage(Interface(), static_cast<M3GImageFormat>(image->paletteFormat), image->width,
                                 image->height, M3G_PALETTED);
        if (created != nullptr) {
            m3gSetImage(created, image->pixels.data());
            m3gSetImagePalette(created, entries, image->palette.data());
        }
    } else {
        created = m3gCreateImage(Interface(), static_cast<M3GImageFormat>(image->format), image->width,
                                 image->height, 0);
        if (created != nullptr) {
            m3gSetImage(created, image->pixels.data());
        }
    }
    if (created == nullptr) {
        return ErrorNoMemory;
    }
    m3gCommitImage(created);
    AttachHandle(self, reinterpret_cast<M3GObject>(created));
    image->pixels.clear();
    image->pixels.shrink_to_fit();
    image->palette.clear();
    return TakeError();
}

/** An image object with no pixels collected (also used to wrap images m3gcore loaded). */
Object* AllocateImage()
{
    ImageObject* image = new ImageObject();
    image->format = 0;
    image->width = 0;
    image->height = 0;
    image->mode = 0;
    image->paletteFormat = 0;
    return image;
}

} // namespace

Object* CreateImage2D()
{
    Object* image = AllocateImage();
    InitObject(image, ClassImage2D);
    return image;
}

void FillImage2DSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    objectClass.allocate = AllocateImage;
    objectClass.destroy = DestroyImage;
    SetSlot(objectClass, 33, &ImageCommit);                  // 0x84
    SetSlot(objectClass, 35, &ImageInitialize);              // 0x8c
    SetSlot(objectClass, 36, &ImageInitializeCompressed);    // 0x90
    SetSlot(objectClass, 37, &ImageSetSubImage);             // 0x94
    SetSlot(objectClass, 40, &ImageSetPalettedImage);        // 0xa0
}

} // namespace ckz::native
