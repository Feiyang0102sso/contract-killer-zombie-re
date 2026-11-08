// CBlit_Buffer_Scale.i - scaled part of CBlit_Buffer.i (same parameters). The scale factors are
// whole numbers (16.16 values with an empty fraction): every source pixel becomes a block of
// scaleX x scaleY destination pixels (up to 8 wide); pixel by pixel conversions only step the rows.

#ifndef BLIT_REPLICATE
/** Write `value` `times` times (1 to 8) from `row` on. */
#define BLIT_REPLICATE(TYPE, row, times, value) \
    switch (times) { \
    case 8: ((TYPE*)(row))[7] = (value); \
    case 7: ((TYPE*)(row))[6] = (value); \
    case 6: ((TYPE*)(row))[5] = (value); \
    case 5: ((TYPE*)(row))[4] = (value); \
    case 4: ((TYPE*)(row))[3] = (value); \
    case 3: ((TYPE*)(row))[2] = (value); \
    case 2: ((TYPE*)(row))[1] = (value); \
    case 1: ((TYPE*)(row))[0] = (value); \
    }
#endif

{
    const unsigned char* source = (const unsigned char*)op.m_source;
    unsigned char* destination = (unsigned char*)op.m_destination;
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
#if BLIT_BUFFER_KIND == 16
    int scaleX = op.m_scaleX >> 16;
    int scaleY = op.m_scaleY >> 16;
    int blockBytes = scaleY * op.m_destinationPitch;
    int pixelBytes = scaleX * 2;
    const unsigned char* sourceRow = source + sourceOffset;
    unsigned char* destinationRow = destination;
    for (int y = 0; y < op.m_height; y++) {
        int count = op.m_width;
        unsigned char* pixel = destinationRow;
        const unsigned char* sourcePixel = sourceRow + sourceStep;
        if (!op.m_flipX) {
            if (((unsigned int)sourceRow & 3) == 0) {
                sourcePixel = sourceRow;
            } else {
                // one pixel to reach a 4 byte aligned source
                unsigned short value = *(const unsigned short*)sourceRow;
                for (unsigned char* row = destinationRow; row < destinationRow + blockBytes; row += op.m_destinationPitch) {
                    BLIT_REPLICATE(unsigned short, row, scaleX, value);
                }
                count--;
                pixel = destination + y * blockBytes + pixelBytes;
                sourcePixel = source + y * sourcePitch + sourceOffset + sourceStep;
            }
            int pairs = count & ~1;
            if (scaleX != 2 || scaleY != 2) {
                unsigned char* block = pixel;
                const unsigned char* sourcePair = sourcePixel;
                for (int x = 0; x < pairs; x += 2) {
                    unsigned int two = *(const unsigned int*)sourcePair;
                    for (unsigned char* row = block; row < block + blockBytes; row += op.m_destinationPitch) {
                        BLIT_REPLICATE(unsigned short, row, scaleX, (unsigned short)two);
                    }
                    for (unsigned char* row = block + pixelBytes; row < block + blockBytes + pixelBytes;
                         row += op.m_destinationPitch) {
                        BLIT_REPLICATE(unsigned short, row, scaleX, (unsigned short)(two >> 16));
                    }
                    block += pixelBytes * 2;
                    sourcePair += sourceStep * 2;
                }
                pixel += pairs * pixelBytes;
                sourcePixel += pairs * sourceStep;
            } else {
                // 2 x 2 blocks
                unsigned short* block = (unsigned short*)pixel;
                for (int x = 0; x < pairs; x += 2) {
                    unsigned int two = *(const unsigned int*)sourcePixel;
                    sourcePixel += sourceStep * 2;
                    unsigned short first = (unsigned short)two;
                    block[0] = first;
                    block[1] = first;
                    *(unsigned short*)((unsigned char*)block + op.m_destinationPitch) = first;
                    *(unsigned short*)((unsigned char*)block + op.m_destinationPitch + 2) = first;
                    unsigned short second = (unsigned short)(two >> 16);
                    block[2] = second;
                    block[3] = second;
                    *(unsigned short*)((unsigned char*)block + op.m_destinationPitch + 4) = second;
                    *(unsigned short*)((unsigned char*)block + op.m_destinationPitch + 6) = second;
                    block += 4;
                }
                pixel += pairs * 4;
            }
            if (count & 1) {
                unsigned short value = *(const unsigned short*)sourcePixel;
                for (unsigned char* row = pixel; row < pixel + blockBytes; row += op.m_destinationPitch) {
                    BLIT_REPLICATE(unsigned short, row, scaleX, value);
                }
            }
        } else {
            // reading backwards: the pair below the current pixel
            if (((unsigned int)sourceRow & 3) == 0) {
                unsigned short value = *(const unsigned short*)sourceRow;
                for (unsigned char* row = destinationRow; row < destinationRow + blockBytes; row += op.m_destinationPitch) {
                    BLIT_REPLICATE(unsigned short, row, scaleX, value);
                }
                count--;
                pixel = destinationRow + pixelBytes;
            } else {
                sourcePixel = source + y * sourcePitch + sourceOffset;
                pixel = destination + y * blockBytes;
            }
            if (count > 1) {
                sourcePixel += sourceStep;
            }
            int pairs = count & ~1;
            if (scaleX != 2 || scaleY != 2) {
                unsigned char* block = pixel;
                const unsigned char* sourcePair = sourcePixel;
                for (int x = 0; x < pairs; x += 2) {
                    unsigned int two = *(const unsigned int*)sourcePair;
                    for (unsigned char* row = block + pixelBytes; row < block + blockBytes + pixelBytes;
                         row += op.m_destinationPitch) {
                        BLIT_REPLICATE(unsigned short, row, scaleX, (unsigned short)two);
                    }
                    for (unsigned char* row = block; row < block + blockBytes; row += op.m_destinationPitch) {
                        BLIT_REPLICATE(unsigned short, row, scaleX, (unsigned short)(two >> 16));
                    }
                    block += pixelBytes * 2;
                    sourcePair += sourceStep * 2;
                }
                pixel += pairs * pixelBytes;
                sourcePixel += pairs * sourceStep;
            } else {
                // 2 x 2 blocks
                unsigned short* block = (unsigned short*)pixel;
                for (int x = 0; x < pairs; x += 2) {
                    unsigned int two = *(const unsigned int*)sourcePixel;
                    unsigned short first = (unsigned short)two;
                    block[2] = first;
                    block[3] = first;
                    *(unsigned short*)((unsigned char*)block + op.m_destinationPitch + 4) = first;
                    *(unsigned short*)((unsigned char*)block + op.m_destinationPitch + 6) = first;
                    unsigned short second = (unsigned short)(two >> 16);
                    block[0] = second;
                    block[1] = second;
                    *(unsigned short*)((unsigned char*)block + op.m_destinationPitch) = second;
                    *(unsigned short*)((unsigned char*)block + op.m_destinationPitch + 2) = second;
                    sourcePixel += sourceStep * 2;
                    block += 4;
                }
                pixel += pairs * 4;
            }
            if (count & 1) {
                if (count > 1) {
                    sourcePixel -= sourceStep;
                }
                unsigned short value = *(const unsigned short*)sourcePixel;
                for (unsigned char* row = pixel; row < pixel + blockBytes; row += op.m_destinationPitch) {
                    BLIT_REPLICATE(unsigned short, row, scaleX, value);
                }
            }
        }
        sourceRow += sourcePitch;
        destinationRow += blockBytes;
    }
#elif BLIT_BUFFER_KIND == 32
    int scaleY = op.m_scaleY >> 16;
    int scaleX = op.m_scaleX >> 16;
    int rowBytes = op.m_destinationPitch * scaleY;
    int blockBytes = scaleY * op.m_destinationPitch;
    int pixelBytes = scaleX * 4;
    unsigned char twoByTwo = (scaleY == 2 && scaleX == 2);
    const unsigned char* sourceRow = source + sourceOffset;
    for (int y = 0; y < op.m_height; y++) {
        const unsigned char* sourcePixel = sourceRow;
        unsigned char* pixel = destination;
        if (twoByTwo) {
            for (int x = op.m_width; x > 0; x--) {
                unsigned int value = *(const unsigned int*)sourcePixel;
                sourcePixel += sourceStep;
                ((unsigned int*)pixel)[0] = value;
                ((unsigned int*)pixel)[1] = value;
                *(unsigned int*)(pixel + op.m_destinationPitch) = value;
                *(unsigned int*)(pixel + op.m_destinationPitch + 4) = value;
                pixel += 8;
            }
        } else {
            for (int x = op.m_width; x > 0; x--) {
                if (rowBytes > 0) {
                    unsigned int value = *(const unsigned int*)sourcePixel;
                    for (unsigned char* row = pixel; row < pixel + blockBytes; row += op.m_destinationPitch) {
                        BLIT_REPLICATE(unsigned int, row, scaleX, value);
                    }
                }
                sourcePixel += sourceStep;
                pixel += pixelBytes;
            }
        }
        sourceRow += sourcePitch;
        destination += blockBytes;
    }
#else
    // conversions: one destination pixel per source pixel, rows stepped by the vertical scale
    int destinationStep = (op.m_scaleY >> 16) * op.m_destinationPitch;
    const unsigned char* sourceRow = source + sourceOffset;
    for (int y = 0; y < op.m_height; y++) {
        const unsigned char* sourcePixel = sourceRow;
        unsigned char* pixel = destination;
        for (int x = 0; x < op.m_width; x++) {
            unsigned int value = *(const unsigned int*)sourcePixel;
            sourcePixel += sourceStep;
            BLIT_BUFFER_STORE_NEXT(pixel, value);
        }
        sourceRow += sourcePitch;
        destination += destinationStep;
    }
#endif
}
