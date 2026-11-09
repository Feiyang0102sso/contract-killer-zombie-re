// CBlit_Stream.i - body of the CBlit::Stream_*_ColorKeyE decoders for direct color sources
// (included inside the function).
// Parameters (macros defined by CBlit.cpp before the include):
//   BLIT_STREAM_READ()           read one source pixel from op.m_stream into the locals it declares
//   BLIT_STREAM_KEY              the pixel's color as compared with the color key (24 bit)
//   BLIT_STREAM_COLOR_KEY        the color key (24 bit)
//   BLIT_DESTINATION_BYTES       bytes per destination pixel
//   BLIT_STREAM_STORE(d)         write the converted pixel at d
{
    unsigned char* destination = (unsigned char*)op.m_destination;
    for (int y = 0; y < op.m_height; y++) {
        for (int x = 0; x < op.m_width; x++) {
            BLIT_STREAM_READ();
            if (!op.m_colorKeyEnabled || BLIT_STREAM_KEY != BLIT_STREAM_COLOR_KEY) {
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
