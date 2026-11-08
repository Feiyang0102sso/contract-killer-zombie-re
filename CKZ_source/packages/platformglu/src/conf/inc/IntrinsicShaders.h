// IntrinsicShaders.h - the built-in shader programs of the OpenGL ES device (created by b037x).
// Included by graphics/src/CGraphics_OGLES.cpp only: the raw program data is defined here.
#ifndef INTRINSICSHADERS_H
#define INTRINSICSHADERS_H

/** Ids of the built-in shader programs (the list ends with 0). */
static const unsigned int hcommon_sdrpgm[] = {
    0x3b01104c,     // common_sdrpgm_mvp_conditional_constcolor_mod_tex0
    0
};

/** Parameter declarations of "common_mvp_conditional_constcolor_mod_tex0" (no terminator). */
unsigned char common_mvp_conditional_constcolor_mod_tex0_rawData[55] = {
    99, 111, 110, 115, 116, 67, 111, 108,
    111, 114, 58, 73, 110, 116, 101, 103,
    101, 114, 52, 100, 58, 49, 32, 109,
    118, 112, 58, 77, 97, 116, 114, 105,
    120, 52, 100, 58, 49, 32, 116, 101,
    120, 69, 110, 118, 48, 58, 73, 110,
    116, 101, 103, 101, 114, 58, 49
};

/** Shader program resource "common_sdrpgm_mvp_conditional_constcolor_mod_tex0". */
unsigned char common_sdrpgm_mvp_conditional_constcolor_mod_tex0_rawData[65] = {
    27, 136, 213, 165, 99, 111, 109, 109,
    111, 110, 95, 109, 118, 112, 95, 99,
    111, 110, 100, 105, 116, 105, 111, 110,
    97, 108, 95, 99, 111, 110, 115, 116,
    99, 111, 108, 111, 114, 95, 109, 111,
    100, 95, 116, 101, 120, 48, 0, 169,
    190, 241, 165, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0
};

#endif
