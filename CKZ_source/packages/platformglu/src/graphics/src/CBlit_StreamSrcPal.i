// CBlit_StreamSrcPal.i - body of the CBlit::Stream_P*_ColorKeyE decoders for paletted sources
// (included inside the function).
// Parameters (macros defined by CBlit.cpp before the include):
//   BLIT_PALETTE_ENTRIES         16: two 4 bit indices per byte (high nibble first), 256: one per byte
//   BLIT_DESTINATION_BYTES       bytes per destination pixel
//   BLIT_STREAM_STORE(d)         write the converted palette entry `entry` at d
{
    unsigned char* destination = (unsigned char*)op.m_destination;
#if BLIT_PALETTE_ENTRIES == 16
    unsigned int indices = 0;
#endif
    for (int y = 0; y < op.m_height; y++) {
        for (int x = 0; x < op.m_width; x++) {
#if BLIT_PALETTE_ENTRIES == 16
            // even pixels read a byte, odd pixels use its low nibble
            // (b147: the original places the odd-pixel block first)
            if ((x & 1) != 0) {
                indices = (indices & 0x0f) << 4;
            } else {
                indices = op.m_stream->ReadUInt8();
            }
            if (!op.m_colorKeyEnabled ||
                (op.m_palette[indices >> 4] & 0xffffff) !=
                    (unsigned int)(op.m_colorKey[0] | (op.m_colorKey[1] << 8) | (op.m_colorKey[2] << 16))) {
                unsigned int entry = op.m_palette[indices >> 4];
#else
            unsigned int entry = op.m_palette[op.m_stream->ReadUInt8()];
            if (!op.m_colorKeyEnabled ||
                (entry & 0xffffff) !=
                    (unsigned int)(op.m_colorKey[0] | (op.m_colorKey[1] << 8) | (op.m_colorKey[2] << 16))) {
#endif
                // destination position: optionally transposed, then mirrored
                int column;
                int line;
                if (op.m_transpose) {
                    column = y;
                    if (op.m_flipX) {
                        column = op.m_height - 1 - y;
                    }
                    line = x;
                    if (op.m_flipY) {
                        line = op.m_width - 1 - x;
                    }
                } else {
                    column = x;
                    if (op.m_flipX) {
                        column = op.m_width - 1 - x;
                    }
                    line = y;
                    if (op.m_flipY) {
                        line = op.m_height - 1 - y;
                    }
                }
                BLIT_STREAM_STORE(destination + (line * op.m_destinationPitch + column * BLIT_DESTINATION_BYTES));
            }
        }
        // b147: the row padding is read into a local first; the original keeps the op.m_stream load
        // inside the pixel loop instead of sharing it with this Skip call
        unsigned int rowSkip = op.m_rowSkip;
        op.m_stream->Skip(rowSkip);
    }
}
