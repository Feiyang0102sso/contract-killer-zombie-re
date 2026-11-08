// CBlit_Buffer.i - body of the CBlit::Buffer_* copies (included inside the function).
// Parameters (macros defined by CBlit.cpp before the include):
//   BLIT_BUFFER_KIND             0: pixel by pixel conversion
//                                16: 16 bit copy of the same format (two pixels per 32 bit read)
//                                32: 32 bit copy of the same format (two pixels per iteration)
//   BLIT_SOURCE_BYTES            bytes per source pixel
//   BLIT_BUFFER_STORE_AT(d, x, p)  kind 0: write source pixel p (read as 32 bits) as pixel x of row d
//   BLIT_BUFFER_STORE_NEXT(d, p)   kind 0: write source pixel p at d and advance d
// Scaled copies (m_scaleX / m_scaleY not 1.0) are in CBlit_Buffer_Scale.i.
{
    if (op.m_scaleX == 0x10000 && op.m_scaleY == 0x10000) {
        const unsigned char* source = (const unsigned char*)op.m_source;
        unsigned char* destination = (unsigned char*)op.m_destination;
        // source walk: right to left when flipped horizontally, bottom-up when flipped vertically
        int sourceStep;
        int sourceOffset;
        if (!op.m_flipX) {
            sourceStep = BLIT_SOURCE_BYTES;
            sourceOffset = 0;
        } else {
            sourceStep = -BLIT_SOURCE_BYTES;
            sourceOffset = op.m_width * BLIT_SOURCE_BYTES - BLIT_SOURCE_BYTES;
        }
        int sourcePitch = op.m_sourcePitch;
        if (op.m_flipY) {
            sourceOffset += (op.m_height - 1) * sourcePitch;
            sourcePitch = -sourcePitch;
        }
        const unsigned char* sourceRow = source + sourceOffset;
        for (int y = 0; y < op.m_height; y++) {
#if BLIT_BUFFER_KIND == 16
            int count = op.m_width;
            unsigned short* pixel = (unsigned short*)destination;
            const unsigned char* sourcePixel = sourceRow + sourceStep;
            if (!op.m_flipX) {
                // one pixel to reach a 4 byte aligned source
                if (((unsigned int)sourceRow & 3) == 0) {
                    sourcePixel = sourceRow;
                } else {
                    *pixel = *(const unsigned short*)sourceRow;
                    pixel++;
                    count--;
                }
                int pairs = count & ~1;
                for (int x = 0; x < pairs; x += 2) {
                    unsigned int two = *(const unsigned int*)sourcePixel;
                    pixel[x] = (unsigned short)two;
                    pixel[x + 1] = (unsigned short)(two >> 16);
                    sourcePixel += sourceStep * 2;
                }
                pixel += pairs;
                if (count & 1) {
                    *pixel = *(const unsigned short*)sourcePixel;
                }
            } else {
                // reading backwards: the pair below the current pixel
                if (((unsigned int)sourceRow & 3) == 0) {
                    *pixel = *(const unsigned short*)sourceRow;
                    pixel++;
                    count--;
                } else {
                    sourcePixel = sourceRow;
                }
                if (count > 1) {
                    sourcePixel += sourceStep;
                }
                int pairs = count & ~1;
                for (int x = 0; x < pairs; x += 2) {
                    unsigned int two = *(const unsigned int*)sourcePixel;
                    sourcePixel += sourceStep * 2;
                    pixel[x + 1] = (unsigned short)two;
                    pixel[x] = (unsigned short)(two >> 16);
                }
                pixel += pairs;
                if (count & 1) {
                    if (count > 1) {
                        sourcePixel -= sourceStep;
                    }
                    *pixel = *(const unsigned short*)sourcePixel;
                }
            }
#elif BLIT_BUFFER_KIND == 32
            int count = op.m_width;
            const unsigned char* sourcePixel = sourceRow;
            unsigned int* pixel = (unsigned int*)destination;
            int pairs = count & ~1;
            for (int x = 0; x < pairs; x += 2) {
                pixel[x] = *(const unsigned int*)sourcePixel;
                pixel[x + 1] = *(const unsigned int*)(sourcePixel + sourceStep);
                sourcePixel += sourceStep * 2;
            }
            pixel += pairs;
            if (count & 1) {
                *pixel = *(const unsigned int*)sourcePixel;
            }
#else
            const unsigned char* sourcePixel = sourceRow;
            for (int x = 0; x < op.m_width; x++) {
                unsigned int value = *(const unsigned int*)sourcePixel;
                sourcePixel += sourceStep;
                BLIT_BUFFER_STORE_AT(destination, x, value);
            }
#endif
            sourceRow += sourcePitch;
            destination += op.m_destinationPitch;
        }
    } else {
#include "CBlit_Buffer_Scale.i"
    }
}
