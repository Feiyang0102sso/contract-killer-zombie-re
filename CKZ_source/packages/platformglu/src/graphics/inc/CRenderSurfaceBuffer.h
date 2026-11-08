// CRenderSurfaceBuffer.h - pixel buffers of the render surfaces and their decoders.
// (first drafted by b037x as PROVISIONAL with the formats only; completed by b039x, the owner of
// graphics/src/CRenderSurfaceBuffer.cpp)
#ifndef CRENDERSURFACEBUFFER_H
#define CRENDERSURFACEBUFFER_H

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
}

namespace graphics {

/**
 * Pixel formats of render surface buffers (same values as components::ColorFormat).
 * Bits 0..4: bytes per pixel; 0x4000: has alpha; 0x8000: palettized; 0x80000000: compressed.
 */
enum RSBFormat {
    RSBFormat_None          = 0,
    RSBFormat_R5G6B5        = 0x21302,
    RSBFormat_R5G5B5A1      = 0x55402,
    RSBFormat_R4G4B4A4      = 0x75402,
    RSBFormat_R8G8B8        = 0x90303,
    RSBFormat_B8G8R8        = 0xa0303,
    RSBFormat_X8R8G8B8      = 0xb0304,
    RSBFormat_A8R8G8B8      = 0xc4404,
    RSBFormat_A8B8G8R8      = 0xf4404,
    RSBFormat_ARGB_Fixed    = 0x1e4410,     // components::Color_ARGB_fixed
    RSBFormat_P16X8R8G8B8   = 0x328300,     // 16 entry palette
    RSBFormat_P16A8R8G8B8   = 0x33c400,     // 16 entry palette with alpha
    RSBFormat_P256X8R8G8B8  = 0x348301,     // 256 entry palette
    RSBFormat_P256A8R8G8B8  = 0x35c401,     // 256 entry palette with alpha
    RSBFormat_Depth16       = 0x5a0102,
    RSBFormat_PVRTC_RGB2    = 0x80461300,
    RSBFormat_PVRTC_RGBA2   = 0x80475400,
    RSBFormat_PVRTC_RGB4    = 0x80481300,
    RSBFormat_PVRTC_RGBA4   = 0x80495400
};

/** Bytes per pixel of a format. */
enum { RSBFormat_BytesPerPixelMask = 0x1f };

/** Single pixel ("fragment") conversion. */
class CRSBFrag {
public:
    /** Convert one pixel of `sourceFormat` at `source` to `destinationFormat` at `destination`. */
    static unsigned char Convert(RSBFormat sourceFormat, const void* source, RSBFormat destinationFormat,
                                 void* destination);
};

/**
 * One pixel buffer. Initialize() either allocates the pixels, uses memory given by the caller or
 * decodes them from a stream (PNG, BMP, JPEG, M3G, SBI).
 */
class CRenderSurfaceBuffer {
public:
    /** How the pitch of the buffer is chosen. */
    enum PitchCondition {
        PitchCondition_Positive = 0,    // bytes per pixel * width, top-down
        PitchCondition_Negative = 1,    // bytes per pixel * width, bottom-up
        PitchCondition_Given = 2        // the pitch passed by the caller
    };

    /** m_flags bits. */
    enum Flag {
        Flag_OwnsData = 0x1,            // m_data was allocated by the buffer
        Flag_ExternalData = 0x2,        // m_data was given by the caller
        Flag_PowerOfTwo = 0x4,          // m_width / m_height are padded to powers of two
        Flag_Mipmap = 0x8               // CRenderSurfaceBufferMipmap
    };

    /** Format replacement used when a decoder picks the destination format. */
    struct FormatMapEntry {
        RSBFormat m_from;               // 0x00
        RSBFormat m_to;                 // 0x04
    };

    /** Where and how to decode the pixels from (Initialize). */
    struct SourceStreamDesc {
        unsigned char m_colorKeyEnabled;        // 0x00 pixels of m_colorKey become transparent
        unsigned char m_transpose;              // 0x01 InitParam_Orientation: rows become columns
        unsigned char m_flipX;                  // 0x02
        unsigned char m_flipY;                  // 0x03
        unsigned char m_flag04;                 // 0x04
        components::CInputStream* m_stream;     // 0x08
        unsigned int m_type;                    // 0x0c mime type key
        unsigned int m_colorKey;                // 0x10 A8R8G8B8
        FormatMapEntry m_formatMap[2][10];      // 0x14 [has color key][entry]
    };

    CRenderSurfaceBuffer()
    {
        m_format = RSBFormat_None;
        m_flags = 0;
        m_pitch = 0;
        m_width = 0;
        m_height = 0;
        m_imageWidth = 0;
        m_imageHeight = 0;
        m_data = NULL;
    }

    virtual ~CRenderSurfaceBuffer();

    /**
     * Set the buffer up for a `width` x `height` image of `format`. `palette` receives the palette of a
     * decoded image; `pixels` is memory to use instead of allocating.
     */
    virtual unsigned char Initialize(RSBFormat format, unsigned short width, unsigned short height,
                                     unsigned char powerOfTwo, void** palette, const SourceStreamDesc* desc,
                                     PitchCondition pitchCondition, short pitch, void* pixels);
    virtual void MakePitchPositive();
    virtual void MakePitchNegative();
    /** Pixels of mip level `level`. */
    virtual void* GetLevel(unsigned int level) const;

    void Destroy();
    unsigned char DestroyBufferData();
    /** Fill the whole image with `color` of `format`. */
    unsigned char Fill(RSBFormat format, void* color);

    unsigned char InitializeFromDIB(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                    PitchCondition pitchCondition, short pitch, void* pixels);
    unsigned char InitializeFromJPEG(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                     PitchCondition pitchCondition, short pitch, void* pixels);
    unsigned char InitializeFromM3G(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                    PitchCondition pitchCondition, short pitch, void* pixels);
    unsigned char InitializeFromPNG(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                    PitchCondition pitchCondition, short pitch, void* pixels);
    unsigned char InitializeFromSBI(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                    PitchCondition pitchCondition, short pitch, void* pixels);

    RSBFormat m_format;             // 0x04
    unsigned short m_flags;         // 0x08 Flag bits
    short m_pitch;                  // 0x0a bytes per row (negative: bottom-up)
    unsigned short m_width;         // 0x0c allocated width
    unsigned short m_height;        // 0x0e allocated height
    unsigned short m_imageWidth;    // 0x10 width of the image
    unsigned short m_imageHeight;   // 0x12 height of the image
    void* m_data;                   // 0x14 first row in memory order
};

// the key function is defined inline: the vtable is emitted (weak) where it is used, as the original
inline CRenderSurfaceBuffer::~CRenderSurfaceBuffer()
{
    Destroy();
}

/** A buffer with up to 10 mip levels (level 0 is the buffer itself). */
class CRenderSurfaceBufferMipmap : public CRenderSurfaceBuffer {
public:
    enum { MaxLevels = 10 };

    CRenderSurfaceBufferMipmap() : CRenderSurfaceBuffer()
    {
        m_flags = 8;
        m_levelCount = 0;
        memset(m_levels, 0, sizeof(m_levels));
    }

    virtual ~CRenderSurfaceBufferMipmap() {}

    virtual unsigned char Initialize(RSBFormat format, unsigned short width, unsigned short height,
                                     unsigned char powerOfTwo, void** palette, const SourceStreamDesc* desc,
                                     PitchCondition pitchCondition, short pitch, void* pixels);
    virtual void MakePitchPositive();
    virtual void MakePitchNegative();
    virtual void* GetLevel(unsigned int level) const;

    unsigned int m_levelCount;      // 0x18
    void* m_levels[MaxLevels];      // 0x1c
};

} } } }

#endif
