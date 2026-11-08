// SquareTransform.h - the 8 transforms of a square sprite (created by b042).
// Numbering as the J2ME Sprite.TRANS_* constants: bit 0 mirrors vertically, bit 1 mirrors
// horizontally and bit 2 swaps the axes (transposed, i.e. rotated by 90 degrees).
#ifndef SQUARETRANSFORM_H
#define SQUARETRANSFORM_H

enum SquareTransform {
    SquareTransform_None = 0,
    SquareTransform_MirrorVertical = 1,
    SquareTransform_MirrorHorizontal = 2,
    SquareTransform_Rotate180 = 3,
    SquareTransform_Transposed = 4
};

/** Transforms 4..7 swap width and height. */
inline bool SquareTransform_IsTransposed(unsigned char transform)
{
    return transform >= SquareTransform_Transposed;
}

/**
 * Transform of a sprite drawn with `inner` inside a presenter drawn with `outer`: the mirror
 * bits of `outer` swap places when the sprite itself is transposed.
 */
inline unsigned char SquareTransform_Compose(unsigned char inner, unsigned char outer)
{
    int swap = inner >> 2;
    return inner ^ ((outer & 4) | ((outer << swap) & 2) | ((outer >> swap) & 1));
}

#endif
