// CBlitUtil.cpp - registry of the software blit operations and the sub-rectangle clipper.
#include "CBlitUtil.h"

namespace com { namespace glu { namespace platform { namespace graphics {

int CBlitUtil::ClipSubRectangle(int& x, int& y, short clipX, short clipY, short clipWidth, short clipHeight,
                                short& sourceX, short& sourceY, short& width, short& height,
                                unsigned char flipX, unsigned char flipY)
{
    int result = 0;

    // horizontal
    if (x < clipX) {
        short cut = clipX - x;
        width -= cut;
        if (width <= 0) {
            return result | ClipResult_Left | ClipResult_Empty;
        }
        if (!flipX) {
            sourceX += cut;
        }
        result = ClipResult_Left;
        x = clipX;
    }
    if (x + width > clipX + clipWidth) {
        short cut = x + width - (clipX + clipWidth);
        width -= cut;
        if (width <= 0) {
            return result | ClipResult_Right | ClipResult_Empty;
        }
        result |= ClipResult_Right;
        if (flipX) {
            sourceX += cut;
        }
    }

    // vertical
    if (y < clipY) {
        short cut = clipY - y;
        height -= cut;
        if (height <= 0) {
            return result | ClipResult_Top | ClipResult_Empty;
        }
        if (!flipY) {
            sourceY += cut;
        }
        result |= ClipResult_Top;
        y = clipY;
    }
    if (y + height > clipY + clipHeight) {
        short cut = y + height - (clipY + clipHeight);
        height -= cut;
        if (height <= 0) {
            return result | ClipResult_Bottom | ClipResult_Empty;
        }
        result |= ClipResult_Bottom;
        if (flipY) {
            sourceY += cut;
        }
    }
    return result | ClipResult_Visible;
}

void CBlitUtil::AddBlitOp(RSBFormat source, RSBFormat destination, ColorKeyTest colorKeyTest, AlphaTest alphaTest,
                          Blend blend, BlitOpId opId, void* function)
{
    CBlitUtil* util = GetInstance();
    // append a node to the list
    if (util->m_first == NULL) {
        util->m_first = new BlitOpNode();
        util->m_last = util->m_first;
    } else {
        util->m_last->m_next = new BlitOpNode();
        util->m_last = util->m_last->m_next;
    }
    util->m_last->m_desc.m_source = source;
    util->m_last->m_desc.m_destination = destination;
    util->m_last->m_desc.m_colorKeyTest = colorKeyTest;
    util->m_last->m_desc.m_alphaTest = alphaTest;
    util->m_last->m_desc.m_blend = blend;
    util->m_last->m_desc.m_opId = opId;
    util->m_last->m_desc.m_function = function;
}

unsigned char CBlitUtil::GetBlitOp(BlitOpDesc& desc)
{
    CBlitUtil* util = GetInstance();
    for (BlitOpNode* node = util->m_first; node != NULL; node = node->m_next) {
        // a registration with source RSBFormat_None accepts any source; a color keyed loop also
        // serves requests without color key
        if ((node->m_desc.m_source == desc.m_source || node->m_desc.m_source == RSBFormat_None) &&
            node->m_desc.m_destination == desc.m_destination &&
            ((node->m_desc.m_colorKeyTest == ColorKeyTest_Enabled && desc.m_colorKeyTest == ColorKeyTest_None) ||
             node->m_desc.m_colorKeyTest == desc.m_colorKeyTest) &&
            node->m_desc.m_alphaTest == desc.m_alphaTest &&
            node->m_desc.m_blend == desc.m_blend &&
            node->m_desc.m_opId == desc.m_opId) {
            desc.m_function = node->m_desc.m_function;
            return 1;
        }
    }
    desc.m_function = NULL;
    return 0;
}

} } } }
