// CBlitUtil.h - registry of the software blit operations (application singleton) and blit helpers.
// (drafted by b037x as PROVISIONAL; completed by b034x, the writer of graphics/src/CBlitUtil.cpp.
// The class's weak destructor / vtable are first emitted in graphics/2d/src/CGraphics2d_OGLES.cpp.)
#ifndef CBLITUTIL_H
#define CBLITUTIL_H

#include <stddef.h>
#include "CSingleton.h"
#include "CBlit.h"
#include "CRenderSurfaceBuffer.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CBlitUtil : public components::CSingleton {
public:
    /** Key in the application singleton table. */
    enum { SingletonId = 0x4ae1222b };

    enum ColorKeyTest {
        ColorKeyTest_None = 0,
        ColorKeyTest_Enabled = 1
    };

    enum AlphaTest {
        AlphaTest_None = 0
    };

    enum Blend {
        Blend_None = 0
    };

    /** Kind of blit loop (selects the operation description type). */
    enum BlitOpId {
        BlitOpId_Buffer = 0,
        BlitOpId_Stream = 2,
        BlitOpId_Rectangle = 3
    };

    /** ClipSubRectangle() result bits. */
    enum ClipResult {
        ClipResult_Left = 0x01,         // clipped at the left edge
        ClipResult_Right = 0x02,
        ClipResult_Top = 0x04,
        ClipResult_Bottom = 0x08,
        ClipResult_Visible = 0x20,      // something is left to draw
        ClipResult_Empty = 0x40         // nothing is left
    };

    /** Query / registration of a blit operation; m_function is the blit loop (NULL: none). */
    struct BlitOpDesc {
        RSBFormat m_source;             // 0x00 RSBFormat_None in a registration: any source
        RSBFormat m_destination;        // 0x04
        ColorKeyTest m_colorKeyTest;    // 0x08
        AlphaTest m_alphaTest;          // 0x0c
        Blend m_blend;                  // 0x10
        BlitOpId m_opId;                // 0x14
        void* m_function;               // 0x18
    };

    /** A registered operation (singly linked list). */
    struct BlitOpNode {
        BlitOpNode()
        {
            m_next = NULL;
        }

        BlitOpDesc m_desc;              // 0x00
        BlitOpNode* m_next;             // 0x1c
    };

    CBlitUtil() : components::CSingleton(SingletonId)
    {
        m_first = NULL;
        m_last = NULL;
    }

    inline virtual ~CBlitUtil()
    {
        BlitOpNode* node = m_first;
        while (node != NULL) {
            BlitOpNode* next = node->m_next;
            delete node;
            node = next;
        }
    }

    /** The application's registry, created on first use. */
    static inline CBlitUtil* GetInstance()
    {
        CBlitUtil* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, (void**)&found);
        CBlitUtil* instance = found;
        if (instance == NULL) {
            instance = new CBlitUtil();
        }
        return instance;
    }

    /**
     * Clip the (x, y, width, height) rectangle to (clipX, clipY, clipWidth, clipHeight); the source
     * position moves with the clipped edge (the opposite edge when flipped). Returns ClipResult bits.
     */
    static int ClipSubRectangle(int& x, int& y, short clipX, short clipY, short clipWidth, short clipHeight,
                                short& sourceX, short& sourceY, short& width, short& height,
                                unsigned char flipX, unsigned char flipY);
    /** Register `function` for blits from `source` to `destination` with the given options. */
    static void AddBlitOp(RSBFormat source, RSBFormat destination, ColorKeyTest colorKeyTest, AlphaTest alphaTest,
                          Blend blend, BlitOpId opId, void* function);
    /** Look up the operation matching `desc` into desc.m_function; returns 1 when found. */
    static unsigned char GetBlitOp(BlitOpDesc& desc);

    BlitOpNode* m_first;                // 0x08
    BlitOpNode* m_last;                 // 0x0c
};

} } } }

/** Register the blit loops the OpenGL ES device uses. */
inline void CBlitUtil_AddBlitOps_OGLES()
{
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_R5G6B5, com::glu::platform::graphics::RSBFormat_R5G6B5,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Buffer,
        (void*)com::glu::platform::graphics::CBlit::Buffer_R5G6B5_To_R5G6B5);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8B8G8R8, com::glu::platform::graphics::RSBFormat_R5G6B5,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Buffer,
        (void*)com::glu::platform::graphics::CBlit::Buffer_A8B8G8R8_To_R5G6B5);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_R5G5B5A1, com::glu::platform::graphics::RSBFormat_R5G5B5A1,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Buffer,
        (void*)com::glu::platform::graphics::CBlit::Buffer_R5G5B5A1_To_R5G5B5A1);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8B8G8R8, com::glu::platform::graphics::RSBFormat_R5G5B5A1,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Buffer,
        (void*)com::glu::platform::graphics::CBlit::Buffer_A8B8G8R8_To_R5G5B5A1);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_R4G4B4A4, com::glu::platform::graphics::RSBFormat_R4G4B4A4,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Buffer,
        (void*)com::glu::platform::graphics::CBlit::Buffer_R4G4B4A4_To_R4G4B4A4);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8B8G8R8, com::glu::platform::graphics::RSBFormat_R4G4B4A4,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Buffer,
        (void*)com::glu::platform::graphics::CBlit::Buffer_A8B8G8R8_To_R4G4B4A4);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_B8G8R8, com::glu::platform::graphics::RSBFormat_B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Buffer,
        (void*)com::glu::platform::graphics::CBlit::Buffer_B8G8R8_To_B8G8R8);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8B8G8R8, com::glu::platform::graphics::RSBFormat_A8B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Buffer,
        (void*)com::glu::platform::graphics::CBlit::Buffer_A8B8G8R8_To_A8B8G8R8);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_None, com::glu::platform::graphics::RSBFormat_R5G6B5,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Rectangle,
        (void*)com::glu::platform::graphics::CBlit::Rectangle_To_R5G6B5);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_None, com::glu::platform::graphics::RSBFormat_R5G5B5A1,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Rectangle,
        (void*)com::glu::platform::graphics::CBlit::Rectangle_To_R5G5B5A1);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_None, com::glu::platform::graphics::RSBFormat_R4G4B4A4,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Rectangle,
        (void*)com::glu::platform::graphics::CBlit::Rectangle_To_R4G4B4A4);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_None, com::glu::platform::graphics::RSBFormat_B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Rectangle,
        (void*)com::glu::platform::graphics::CBlit::Rectangle_To_B8G8R8);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_None, com::glu::platform::graphics::RSBFormat_A8B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_None, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Rectangle,
        (void*)com::glu::platform::graphics::CBlit::Rectangle_To_A8B8G8R8);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_R8G8B8, com::glu::platform::graphics::RSBFormat_R5G6B5,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_R8G8B8_To_R5G6B5_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8R8G8B8, com::glu::platform::graphics::RSBFormat_R5G6B5,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_A8R8G8B8_To_R5G6B5_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_P16X8R8G8B8, com::glu::platform::graphics::RSBFormat_R5G6B5,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_P16X8R8G8B8_To_R5G6B5_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_P256X8R8G8B8, com::glu::platform::graphics::RSBFormat_R5G6B5,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_P256X8R8G8B8_To_R5G6B5_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_R8G8B8, com::glu::platform::graphics::RSBFormat_R5G5B5A1,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_R8G8B8_To_R5G5B5A1_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_P16X8R8G8B8, com::glu::platform::graphics::RSBFormat_R5G5B5A1,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_P16X8R8G8B8_To_R5G5B5A1_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_P256X8R8G8B8, com::glu::platform::graphics::RSBFormat_R5G5B5A1,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_P256X8R8G8B8_To_R5G5B5A1_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8R8G8B8, com::glu::platform::graphics::RSBFormat_R4G4B4A4,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_A8R8G8B8_To_R4G4B4A4_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8B8G8R8, com::glu::platform::graphics::RSBFormat_R4G4B4A4,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_A8B8G8R8_To_R4G4B4A4_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_R8G8B8, com::glu::platform::graphics::RSBFormat_B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_R8G8B8_To_B8G8R8_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_R8G8B8, com::glu::platform::graphics::RSBFormat_A8B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_R8G8B8_To_A8B8G8R8_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_P16X8R8G8B8, com::glu::platform::graphics::RSBFormat_B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_P16X8R8G8B8_To_B8G8R8_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_P256X8R8G8B8, com::glu::platform::graphics::RSBFormat_B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_P256X8R8G8B8_To_B8G8R8_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8R8G8B8, com::glu::platform::graphics::RSBFormat_A8B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_A8R8G8B8_To_A8B8G8R8_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_A8B8G8R8, com::glu::platform::graphics::RSBFormat_A8B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_A8B8G8R8_To_A8B8G8R8_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_P16X8R8G8B8, com::glu::platform::graphics::RSBFormat_A8B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_P16X8R8G8B8_To_A8B8G8R8_ColorKeyE);
    com::glu::platform::graphics::CBlitUtil::AddBlitOp(com::glu::platform::graphics::RSBFormat_P256X8R8G8B8, com::glu::platform::graphics::RSBFormat_A8B8G8R8,
        com::glu::platform::graphics::CBlitUtil::ColorKeyTest_Enabled, com::glu::platform::graphics::CBlitUtil::AlphaTest_None,
        com::glu::platform::graphics::CBlitUtil::Blend_None, com::glu::platform::graphics::CBlitUtil::BlitOpId_Stream,
        (void*)com::glu::platform::graphics::CBlit::Stream_P256X8R8G8B8_To_A8B8G8R8_ColorKeyE);
}

#endif
