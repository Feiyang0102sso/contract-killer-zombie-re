#pragma once
#include "gl.h"
#include <GLES/glext.h>

#define GL_READ_FRAMEBUFFER_APPLE 0x8ca8
#define GL_DRAW_FRAMEBUFFER_APPLE 0x8ca9
#define GL_BGRA_EXT 0x80e1
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT 0x8366

#ifdef __cplusplus
extern "C" {
#endif
void GL_APIENTRY glRenderbufferStorageMultisampleAPPLE(GLenum target, GLsizei samples,
                                                       GLenum format, GLsizei width, GLsizei height);
void GL_APIENTRY glResolveMultisampleFramebufferAPPLE(void);
#ifdef __cplusplus
}
#endif
