// CRectangle.h - axis aligned rectangle (x, y, width, height) of any scalar type.
// (created by b037x, header-only; first emitted instances in graphics/src/CGraphics.cpp)
#ifndef CRECTANGLE_H
#define CRECTANGLE_H

namespace com { namespace glu { namespace platform { namespace components {

template <class T>
class CRectangle {
public:
    /** Clip this rectangle to `bounds` (an empty result gets a zero width / height). */
    void Clip(const CRectangle<T>& bounds);

    /**
     * Clip the 1d region [start, start + length) to [clipStart, clipStart + clipLength); the
     * length becomes 0 when nothing is left or when clipping flipped its sign.
     */
    void ClipRegionToRegion(T clipStart, T clipLength, T& start, T& length);

    T m_x;          // 0x00
    T m_y;
    T m_width;
    T m_height;
};

template <class T>
void CRectangle<T>::Clip(const CRectangle<T>& bounds)
{
    ClipRegionToRegion(bounds.m_x, bounds.m_width, m_x, m_width);
    ClipRegionToRegion(bounds.m_y, bounds.m_height, m_y, m_height);
}

template <class T>
void CRectangle<T>::ClipRegionToRegion(T clipStart, T clipLength, T& start, T& length)
{
    T originalLength = length;
    if (clipStart + clipLength > start && clipStart < start + length) {
        if (start + length > clipStart + clipLength) {
            length = clipStart + clipLength - start;
        }
        if (start < clipStart) {
            length = start - clipStart + length;
            start = clipStart;
        }
        if (originalLength > 0 && length < 0) {
            length = 0;
        } else if (originalLength < 0 && length > 0) {
            length = 0;
        }
    } else {
        length = 0;
    }
}

} } } }

#endif
