#pragma once
#include <GLES/gl.h>

#ifdef __cplusplus
extern "C" {
#endif
const GLubyte* GL_APIENTRY ckzGlGetString(GLenum name);
#ifdef __cplusplus
}
#endif
#define glGetString ckzGlGetString
