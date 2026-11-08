// CBlit_Rectangle.i - body of the CBlit::Rectangle_To_* fills (included inside the function).
// Parameters (macros defined by CBlit.cpp before the include):
//   BLIT_RECTANGLE_KIND          16: 16 bit pixels written in pairs once 4 byte aligned
//                                24: 3 byte pixels written byte by byte
//                                32: 32 bit pixels written two at a time
//   BLIT_RECTANGLE_CONVERT(c)    destination pixel of the A8R8G8B8 color c (16 / 32 bit kinds)
{
    unsigned int color = 0;
    CRSBFrag::Convert(op.m_colorFormat, op.m_color, RSBFormat_A8R8G8B8, &color);
#if BLIT_RECTANGLE_KIND == 16
    unsigned short pixel = BLIT_RECTANGLE_CONVERT(color);
    unsigned int pixelPair = pixel | (pixel << 16);
    unsigned char* row = (unsigned char*)op.m_destination;
    for (int y = 0; y < op.m_height; y++) {
        unsigned int count = op.m_width;
        unsigned short* destination = (unsigned short*)row;
        // one pixel to reach a 4 byte boundary
        if ((unsigned int)destination & 3) {
            *destination = pixel;
            destination++;
            count--;
        }
        int pairs = count & ~1;
        if (pairs > 0) {
            // Keep the byte extent available across the pair-writing loop.
            unsigned int rowBytes = count * 2;
            for (int x = 0; x < pairs; x += 2) {
                *(unsigned int*)(destination + x) = pixelPair;
            }
            destination = (unsigned short*)((unsigned char*)destination + (rowBytes & ~3));
        }
        if (count & 1) {
            *destination = pixel;
        }
        row += op.m_destinationPitch;
    }
#elif BLIT_RECTANGLE_KIND == 24
    unsigned char red = (unsigned char)(color >> 16);
    unsigned char green = (unsigned char)(color >> 8);
    unsigned char blue = (unsigned char)color;
    unsigned char* row = (unsigned char*)op.m_destination;
    for (int y = 0; y < op.m_height; y++) {
        unsigned char* destination = row;
        for (int x = op.m_width; x > 0; x--) {
            destination[0] = red;
            destination[1] = green;
            destination[2] = blue;
            destination += 3;
        }
        row += op.m_destinationPitch;
    }
#else
    unsigned int pixel = BLIT_RECTANGLE_CONVERT(color);
    unsigned char* row = (unsigned char*)op.m_destination;
    for (int y = 0; y < op.m_height; y++) {
        unsigned int count = op.m_width;
        unsigned int* destination = (unsigned int*)row;
        int pairs = count & ~1;
        if (pairs > 0) {
            // Advance only when at least one complete pair was written.
            unsigned int rowBytes = count * 4;
            for (int x = 0; x < pairs; x += 2) {
                destination[x] = pixel;
                destination[x + 1] = pixel;
            }
            destination = (unsigned int*)((unsigned char*)destination + (rowBytes & ~7));
        }
        if (count & 1) {
            *destination = pixel;
        }
        row += op.m_destinationPitch;
    }
#endif
}
