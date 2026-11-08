// CBlit.cpp - software blit loops. Every loop is one of the bodies in the CBlit_*.i files,
// specialized for a pair of pixel formats by the macros defined in front of it.
#include "CBlit.h"
#include "CRenderSurfaceBuffer.h"
#include "CInputStream.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// ---- pixel conversions (source pixel as read: A8B8G8R8 = R in the low byte, A8R8G8B8 = B in the low byte) ----
#define BLIT_ABGR_TO_R5G6B5(p)      ((((p) & 0xf8) << 8) | (((p) >> 5) & 0x7e0) | (((p) >> 19) & 0x1f))
#define BLIT_ABGR_TO_R5G5B5A1(p)    ((((p) & 0xf8) << 8) | (((p) >> 5) & 0x7c0) | (((p) >> 18) & 0x3e) | \
                                     (((p) >> 24) != 0))
#define BLIT_ABGR_TO_R4G4B4A4(p)    ((((p) >> 4) & 0xf00) | (((p) & 0xf0) << 8) | ((p) >> 28) | (((p) >> 16) & 0xf0))
#define BLIT_ARGB_TO_R5G6B5(p) ((((p) & 0xff) >> 3) | (((p) >> 5) & 0x7e0) | (((p) >> 8) & 0xf800))
#define BLIT_ARGB_TO_R5G5B5A1(p)    ((((p) >> 8) & 0xf800) | (((p) >> 5) & 0x7c0) | (((p) >> 2) & 0x3e) | \
                                     ((p) > 0xffffff))
#define BLIT_ARGB_TO_R4G4B4A4(p)    ((((p) >> 8) & 0xf000) | (((p) >> 4) & 0xf00) | ((p) & 0xf0) | ((p) >> 28))
#define BLIT_ARGB_TO_ABGR(p)        (((p) & 0xff00ff00) | (((p) & 0xff) << 16) | (((p) >> 16) & 0xff))

// ======== buffer to buffer ========

void CBlit::Buffer_R5G6B5_To_R5G6B5(const BufferOpDesc& op)
{
#define BLIT_BUFFER_KIND 16
#define BLIT_SOURCE_BYTES 2
#include "CBlit_Buffer.i"
#undef BLIT_SOURCE_BYTES
#undef BLIT_BUFFER_KIND
}

void CBlit::Buffer_R5G5B5A1_To_R5G5B5A1(const BufferOpDesc& op)
{
#define BLIT_BUFFER_KIND 16
#define BLIT_SOURCE_BYTES 2
#include "CBlit_Buffer.i"
#undef BLIT_SOURCE_BYTES
#undef BLIT_BUFFER_KIND
}

void CBlit::Buffer_R4G4B4A4_To_R4G4B4A4(const BufferOpDesc& op)
{
#define BLIT_BUFFER_KIND 16
#define BLIT_SOURCE_BYTES 2
#include "CBlit_Buffer.i"
#undef BLIT_SOURCE_BYTES
#undef BLIT_BUFFER_KIND
}

void CBlit::Buffer_B8G8R8_To_B8G8R8(const BufferOpDesc& op)
{
#define BLIT_BUFFER_KIND 0
#define BLIT_SOURCE_BYTES 3
#define BLIT_BUFFER_STORE_AT(d, x, p) \
    (d)[(x) * 3] = (unsigned char)(p); \
    (d)[(x) * 3 + 1] = (unsigned char)((p) >> 8); \
    (d)[(x) * 3 + 2] = (unsigned char)((p) >> 16);
#define BLIT_BUFFER_STORE_NEXT(d, p) \
    (d)[0] = (unsigned char)(p); \
    (d)[1] = (unsigned char)((p) >> 8); \
    (d)[2] = (unsigned char)((p) >> 16); \
    (d) += 3;
#include "CBlit_Buffer.i"
#undef BLIT_BUFFER_STORE_NEXT
#undef BLIT_BUFFER_STORE_AT
#undef BLIT_SOURCE_BYTES
#undef BLIT_BUFFER_KIND
}

void CBlit::Buffer_A8B8G8R8_To_R5G6B5(const BufferOpDesc& op)
{
#define BLIT_BUFFER_KIND 0
#define BLIT_SOURCE_BYTES 4
#define BLIT_BUFFER_STORE_AT(d, x, p) ((unsigned short*)(d))[x] = BLIT_ABGR_TO_R5G6B5(p);
#define BLIT_BUFFER_STORE_NEXT(d, p) *(unsigned short*)(d) = BLIT_ABGR_TO_R5G6B5(p); (d) += 2;
#include "CBlit_Buffer.i"
#undef BLIT_BUFFER_STORE_NEXT
#undef BLIT_BUFFER_STORE_AT
#undef BLIT_SOURCE_BYTES
#undef BLIT_BUFFER_KIND
}

void CBlit::Buffer_A8B8G8R8_To_R5G5B5A1(const BufferOpDesc& op)
{
#define BLIT_BUFFER_KIND 0
#define BLIT_SOURCE_BYTES 4
#define BLIT_BUFFER_STORE_AT(d, x, p) ((unsigned short*)(d))[x] = BLIT_ABGR_TO_R5G5B5A1(p);
#define BLIT_BUFFER_STORE_NEXT(d, p) *(unsigned short*)(d) = BLIT_ABGR_TO_R5G5B5A1(p); (d) += 2;
#include "CBlit_Buffer.i"
#undef BLIT_BUFFER_STORE_NEXT
#undef BLIT_BUFFER_STORE_AT
#undef BLIT_SOURCE_BYTES
#undef BLIT_BUFFER_KIND
}

void CBlit::Buffer_A8B8G8R8_To_R4G4B4A4(const BufferOpDesc& op)
{
#define BLIT_BUFFER_KIND 0
#define BLIT_SOURCE_BYTES 4
#define BLIT_BUFFER_STORE_AT(d, x, p) ((unsigned short*)(d))[x] = BLIT_ABGR_TO_R4G4B4A4(p);
#define BLIT_BUFFER_STORE_NEXT(d, p) *(unsigned short*)(d) = BLIT_ABGR_TO_R4G4B4A4(p); (d) += 2;
#include "CBlit_Buffer.i"
#undef BLIT_BUFFER_STORE_NEXT
#undef BLIT_BUFFER_STORE_AT
#undef BLIT_SOURCE_BYTES
#undef BLIT_BUFFER_KIND
}

void CBlit::Buffer_A8B8G8R8_To_A8B8G8R8(const BufferOpDesc& op)
{
#define BLIT_BUFFER_KIND 32
#define BLIT_SOURCE_BYTES 4
#include "CBlit_Buffer.i"
#undef BLIT_SOURCE_BYTES
#undef BLIT_BUFFER_KIND
}

// ======== rectangle fills ========

void CBlit::Rectangle_To_R5G6B5(const RectangleOpDesc& op)
{
#define BLIT_RECTANGLE_KIND 16
#define BLIT_RECTANGLE_CONVERT(c) BLIT_ARGB_TO_R5G6B5(c)
#include "CBlit_Rectangle.i"
#undef BLIT_RECTANGLE_CONVERT
#undef BLIT_RECTANGLE_KIND
}

void CBlit::Rectangle_To_R5G5B5A1(const RectangleOpDesc& op)
{
#define BLIT_RECTANGLE_KIND 16
#define BLIT_RECTANGLE_CONVERT(c) \
    ((((c) >> 8) & 0xf800) | (((c) >> 2) & 0x3e) | (((c) >> 5) & 0x7c0) | ((c) > 0xffffff))
#include "CBlit_Rectangle.i"
#undef BLIT_RECTANGLE_CONVERT
#undef BLIT_RECTANGLE_KIND
}

void CBlit::Rectangle_To_R4G4B4A4(const RectangleOpDesc& op)
{
#define BLIT_RECTANGLE_KIND 16
#define BLIT_RECTANGLE_CONVERT(c) BLIT_ARGB_TO_R4G4B4A4(c)
#include "CBlit_Rectangle.i"
#undef BLIT_RECTANGLE_CONVERT
#undef BLIT_RECTANGLE_KIND
}

void CBlit::Rectangle_To_B8G8R8(const RectangleOpDesc& op)
{
#define BLIT_RECTANGLE_KIND 24
#include "CBlit_Rectangle.i"
#undef BLIT_RECTANGLE_KIND
}

void CBlit::Rectangle_To_A8B8G8R8(const RectangleOpDesc& op)
{
#define BLIT_RECTANGLE_KIND 32
#define BLIT_RECTANGLE_CONVERT(c) BLIT_ARGB_TO_ABGR(c)
#include "CBlit_Rectangle.i"
#undef BLIT_RECTANGLE_CONVERT
#undef BLIT_RECTANGLE_KIND
}

// ======== stream decoders, R8G8B8 sources (three bytes: first, second, third) ========

#define BLIT_STREAM_READ() \
    unsigned int first = op.m_stream->ReadUInt8(); \
    unsigned int second = op.m_stream->ReadUInt8(); \
    unsigned int third = op.m_stream->ReadUInt8();
#define BLIT_STREAM_KEY (first | (second << 8) | (third << 16))
#define BLIT_STREAM_COLOR_KEY (op.m_colorKey[0] | (op.m_colorKey[2] << 16) | (op.m_colorKey[1] << 8))

void CBlit::Stream_R8G8B8_To_R5G6B5_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) \
    *(unsigned short*)(d) = ((third & 0xf8) << 8) | ((second & 0xfc) << 3) | (first >> 3);
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

void CBlit::Stream_R8G8B8_To_R5G5B5A1_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) \
    *(unsigned short*)(d) = ((third & 0xf8) << 8) | ((second & 0xf8) << 3) | (first >> 2) | 1;
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

void CBlit::Stream_R8G8B8_To_B8G8R8_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 3
#define BLIT_STREAM_STORE(d) \
    (d)[0] = (unsigned char)third; \
    (d)[1] = (unsigned char)second; \
    (d)[2] = (unsigned char)first;
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

void CBlit::Stream_R8G8B8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 4
#define BLIT_STREAM_STORE(d) \
    *(unsigned int*)(d) = third | (first << 16) | (second << 8) | 0xff000000;
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

#undef BLIT_STREAM_COLOR_KEY
#undef BLIT_STREAM_KEY
#undef BLIT_STREAM_READ

// ======== stream decoders, 32 bit sources ========

#define BLIT_STREAM_READ() unsigned int value = op.m_stream->ReadUInt32();
#define BLIT_STREAM_KEY (value & 0xffffff)
#define BLIT_STREAM_COLOR_KEY (unsigned int)(op.m_colorKey[0] | (op.m_colorKey[1] << 8) | (op.m_colorKey[2] << 16))

void CBlit::Stream_A8R8G8B8_To_R5G6B5_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) *(unsigned short*)(d) = BLIT_ARGB_TO_R5G6B5(value);
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

void CBlit::Stream_A8R8G8B8_To_R4G4B4A4_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) *(unsigned short*)(d) = BLIT_ARGB_TO_R4G4B4A4(value);
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

void CBlit::Stream_A8R8G8B8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 4
#define BLIT_STREAM_STORE(d) *(unsigned int*)(d) = BLIT_ARGB_TO_ABGR(value);
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

void CBlit::Stream_A8B8G8R8_To_R4G4B4A4_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) *(unsigned short*)(d) = BLIT_ABGR_TO_R4G4B4A4(value);
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

void CBlit::Stream_A8B8G8R8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_DESTINATION_BYTES 4
#define BLIT_STREAM_STORE(d) *(unsigned int*)(d) = value;
#include "CBlit_Stream.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
}

#undef BLIT_STREAM_COLOR_KEY
#undef BLIT_STREAM_KEY
#undef BLIT_STREAM_READ

// ======== stream decoders, paletted sources (X8R8G8B8 entries) ========

#define BLIT_STORE_R5G6B5(d) *(unsigned short*)(d) = BLIT_ARGB_TO_R5G6B5(entry);
#define BLIT_STORE_R5G5B5A1(d) \
    *(unsigned short*)(d) = (((entry) >> 2) & 0x3e) | (((entry) >> 5) & 0x7c0) | (((entry) >> 8) & 0xf800) | 1;
#define BLIT_STORE_B8G8R8(d) \
    (d)[0] = (unsigned char)(entry >> 16); \
    (d)[1] = (unsigned char)(entry >> 8); \
    (d)[2] = (unsigned char)entry;
#define BLIT_STORE_A8B8G8R8(d) \
    *(unsigned int*)(d) = (entry & 0xff00) | (entry << 16) | ((entry >> 16) & 0xff) | 0xff000000;

void CBlit::Stream_P16X8R8G8B8_To_R5G6B5_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_PALETTE_ENTRIES 16
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) BLIT_STORE_R5G6B5(d)
#include "CBlit_StreamSrcPal.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
#undef BLIT_PALETTE_ENTRIES
}

void CBlit::Stream_P16X8R8G8B8_To_R5G5B5A1_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_PALETTE_ENTRIES 16
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) BLIT_STORE_R5G5B5A1(d)
#include "CBlit_StreamSrcPal.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
#undef BLIT_PALETTE_ENTRIES
}

void CBlit::Stream_P16X8R8G8B8_To_B8G8R8_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_PALETTE_ENTRIES 16
#define BLIT_DESTINATION_BYTES 3
#define BLIT_STREAM_STORE(d) BLIT_STORE_B8G8R8(d)
#include "CBlit_StreamSrcPal.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
#undef BLIT_PALETTE_ENTRIES
}

void CBlit::Stream_P16X8R8G8B8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_PALETTE_ENTRIES 16
#define BLIT_DESTINATION_BYTES 4
#define BLIT_STREAM_STORE(d) BLIT_STORE_A8B8G8R8(d)
#include "CBlit_StreamSrcPal.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
#undef BLIT_PALETTE_ENTRIES
}

void CBlit::Stream_P256X8R8G8B8_To_R5G6B5_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_PALETTE_ENTRIES 256
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) BLIT_STORE_R5G6B5(d)
#include "CBlit_StreamSrcPal.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
#undef BLIT_PALETTE_ENTRIES
}

void CBlit::Stream_P256X8R8G8B8_To_R5G5B5A1_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_PALETTE_ENTRIES 256
#define BLIT_DESTINATION_BYTES 2
#define BLIT_STREAM_STORE(d) BLIT_STORE_R5G5B5A1(d)
#include "CBlit_StreamSrcPal.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
#undef BLIT_PALETTE_ENTRIES
}

void CBlit::Stream_P256X8R8G8B8_To_B8G8R8_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_PALETTE_ENTRIES 256
#define BLIT_DESTINATION_BYTES 3
#define BLIT_STREAM_STORE(d) BLIT_STORE_B8G8R8(d)
#include "CBlit_StreamSrcPal.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
#undef BLIT_PALETTE_ENTRIES
}

void CBlit::Stream_P256X8R8G8B8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op)
{
#define BLIT_PALETTE_ENTRIES 256
#define BLIT_DESTINATION_BYTES 4
#define BLIT_STREAM_STORE(d) BLIT_STORE_A8B8G8R8(d)
#include "CBlit_StreamSrcPal.i"
#undef BLIT_STREAM_STORE
#undef BLIT_DESTINATION_BYTES
#undef BLIT_PALETTE_ENTRIES
}

} } } }
