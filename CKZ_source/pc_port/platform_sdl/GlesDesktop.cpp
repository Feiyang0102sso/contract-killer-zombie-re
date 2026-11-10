// Adapt the fixed-function GLES calls to an SDL desktop OpenGL context.
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <SwerveApi.h>
#include "PVRTDecompress.h"

#undef glGetString

namespace {

template<class Function>
Function Load(const char* name)
{
    auto function = reinterpret_cast<Function>(SDL_GL_GetProcAddress(name));
    if (function == nullptr) {
        fprintf(stderr, "OpenGL function unavailable: %s\n", name);
        abort();
    }
    return function;
}

float Fixed(GLfixed value)
{
    return static_cast<float>(value) / 65536.0f;
}

} // namespace

#define GL_FORWARD(Result, Name, DesktopName, Parameters, Arguments) \
    extern "C" Result GL_APIENTRY Name Parameters \
    { \
        static const auto function = Load<Result (GL_APIENTRY*) Parameters>(#DesktopName); \
        return function Arguments; \
    }

GL_FORWARD(void, glActiveTexture, glActiveTexture, (GLenum unit), (unit))
GL_FORWARD(void, glClientActiveTexture, glClientActiveTexture, (GLenum unit), (unit))
GL_FORWARD(void, glBindBuffer, glBindBuffer, (GLenum target, GLuint id), (target, id))
GL_FORWARD(void, glBufferData, glBufferData,
           (GLenum target, GLsizeiptr size, const void* data, GLenum usage), (target, size, data, usage))
GL_FORWARD(void, glBufferSubData, glBufferSubData,
           (GLenum target, GLintptr offset, GLsizeiptr size, const void* data), (target, offset, size, data))
GL_FORWARD(void, glGenBuffers, glGenBuffers, (GLsizei count, GLuint* ids), (count, ids))
GL_FORWARD(void, glDeleteBuffers, glDeleteBuffers, (GLsizei count, const GLuint* ids), (count, ids))
GL_FORWARD(void, ckzCompressedTexImage2D, glCompressedTexImage2D,
           (GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height,
            GLint border, GLsizei size, const void* data), (target, level, format, width, height, border, size, data))
GL_FORWARD(void, glBindFramebufferOES, glBindFramebuffer, (GLenum target, GLuint id), (target, id))
GL_FORWARD(void, glBindRenderbufferOES, glBindRenderbuffer, (GLenum target, GLuint id), (target, id))
GL_FORWARD(GLenum, glCheckFramebufferStatusOES, glCheckFramebufferStatus, (GLenum target), (target))
GL_FORWARD(void, glGenFramebuffersOES, glGenFramebuffers, (GLsizei count, GLuint* ids), (count, ids))
GL_FORWARD(void, glGenRenderbuffersOES, glGenRenderbuffers, (GLsizei count, GLuint* ids), (count, ids))
GL_FORWARD(void, glDeleteFramebuffersOES, glDeleteFramebuffers, (GLsizei count, const GLuint* ids), (count, ids))
GL_FORWARD(void, glDeleteRenderbuffersOES, glDeleteRenderbuffers, (GLsizei count, const GLuint* ids), (count, ids))
GL_FORWARD(void, glRenderbufferStorageOES, glRenderbufferStorage,
           (GLenum target, GLenum format, GLsizei width, GLsizei height), (target, format, width, height))
GL_FORWARD(void, glFramebufferRenderbufferOES, glFramebufferRenderbuffer,
           (GLenum target, GLenum attachment, GLenum bufferTarget, GLuint id), (target, attachment, bufferTarget, id))
GL_FORWARD(void, glFramebufferTexture2DOES, glFramebufferTexture2D,
           (GLenum target, GLenum attachment, GLenum textureTarget, GLuint id, GLint level),
           (target, attachment, textureTarget, id, level))
GL_FORWARD(void, glGetRenderbufferParameterivOES, glGetRenderbufferParameteriv,
           (GLenum target, GLenum name, GLint* value), (target, name, value))
GL_FORWARD(void, glGenerateMipmapOES, glGenerateMipmap, (GLenum target), (target))
GL_FORWARD(void, glBindVertexArrayOES, glBindVertexArray, (GLuint id), (id))
GL_FORWARD(void, glGenVertexArraysOES, glGenVertexArrays, (GLsizei count, GLuint* ids), (count, ids))
GL_FORWARD(void, glDeleteVertexArraysOES, glDeleteVertexArrays, (GLsizei count, const GLuint* ids), (count, ids))
GL_FORWARD(void, glPointParameterfv, glPointParameterfv, (GLenum name, const GLfloat* values), (name, values))
GL_FORWARD(void, glRenderbufferStorageMultisampleAPPLE, glRenderbufferStorageMultisample,
           (GLenum target, GLsizei samples, GLenum format, GLsizei width, GLsizei height),
           (target, samples, format, width, height))
GL_FORWARD(void, ckzBlitFramebuffer, glBlitFramebuffer,
           (GLint x0, GLint y0, GLint x1, GLint y1, GLint tx0, GLint ty0, GLint tx1, GLint ty1,
            GLbitfield mask, GLenum filter), (x0, y0, x1, y1, tx0, ty0, tx1, ty1, mask, filter))

#undef GL_FORWARD

extern "C" {

void GL_APIENTRY glClearDepth(double depth);
void GL_APIENTRY glDepthRange(double nearDepth, double farDepth);
void GL_APIENTRY glTexEnvi(GLenum target, GLenum name, GLint value);
void GL_APIENTRY glOrtho(double left, double right, double bottom, double top, double nearPlane, double farPlane);
void GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum format, GLsizei width,
                                        GLsizei height, GLint border, GLsizei size, const void* data)
{
    const bool pvrtc = format >= GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG &&
        format <= GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
    if (pvrtc && width > 0 && height > 0 && data != nullptr) {
        const bool twoBits = format == GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG ||
            format == GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        const bool alpha = format == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG ||
            format == GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        const int minimumWidth = twoBits ? 16 : 8;
        const size_t pixels = static_cast<size_t>(std::max(width, minimumWidth)) * std::max(height, 8);
        if (size >= static_cast<GLsizei>(pixels / (twoBits ? 4 : 2))) {
            std::vector<unsigned char> rgba(static_cast<size_t>(width) * height * 4);
            pvr::PVRTDecompressPVRTC(data, twoBits, width, height, rgba.data());
            glTexImage2D(target, level, alpha ? GL_RGBA : GL_RGB, width, height, border,
                         GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
            return;
        }
    }
    if (format == GL_ETC1_RGB8_OES && width > 0 && height > 0 && data != nullptr &&
        size >= ((width + 3) / 4) * ((height + 3) / 4) * 8) {
        std::vector<unsigned char> rgb(static_cast<size_t>(width) * height * 3);
        if (malij297_DecompressRGB_ETC(width, height, static_cast<const unsigned char*>(data), rgb.data(), 0)) {
            GLint previousAlignment;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(target, level, GL_RGB, width, height, border, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
            return;
        }
    }
    ckzCompressedTexImage2D(target, level, format, width, height, border, size, data);
}

const GLubyte* GL_APIENTRY ckzGlGetString(GLenum name)
{
    if (name == GL_VERSION) {
        return reinterpret_cast<const GLubyte*>("OpenGL ES-CM 1.1 (desktop OpenGL)");
    }
    if (name == GL_EXTENSIONS) {
        return reinterpret_cast<const GLubyte*>("GL_OES_texture_npot GL_OES_framebuffer_object");
    }
    return glGetString(name);
}

void GL_APIENTRY glClearDepthf(GLfloat depth) { glClearDepth(depth); }
void GL_APIENTRY glDepthRangef(GLfloat nearDepth, GLfloat farDepth) { glDepthRange(nearDepth, farDepth); }
void GL_APIENTRY glDepthRangex(GLfixed nearDepth, GLfixed farDepth) { glDepthRange(Fixed(nearDepth), Fixed(farDepth)); }
void GL_APIENTRY glAlphaFuncx(GLenum function, GLfixed reference) { glAlphaFunc(function, Fixed(reference)); }
void GL_APIENTRY glColor4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
    glColor4f(Fixed(red), Fixed(green), Fixed(blue), Fixed(alpha));
}
void GL_APIENTRY glScalex(GLfixed x, GLfixed y, GLfixed z) { glScalef(Fixed(x), Fixed(y), Fixed(z)); }
void GL_APIENTRY glFogx(GLenum name, GLfixed value)
{
    if (name == GL_FOG_MODE) {
        glFogf(name, static_cast<GLfloat>(value));
    } else {
        glFogf(name, Fixed(value));
    }
}
void GL_APIENTRY glFogxv(GLenum name, const GLfixed* values)
{
    if (name == GL_FOG_COLOR) {
        const GLfloat rgba[] = { Fixed(values[0]), Fixed(values[1]), Fixed(values[2]), Fixed(values[3]) };
        glFogfv(name, rgba);
    } else {
        glFogx(name, values[0]);
    }
}
void GL_APIENTRY glLightModelx(GLenum name, GLfixed value) { glLightModelf(name, Fixed(value)); }
// Fixed-point calls of the native M3G engine (m3gcore).
void GL_APIENTRY glClearColorx(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
    glClearColor(Fixed(red), Fixed(green), Fixed(blue), Fixed(alpha));
}
void GL_APIENTRY glClearDepthx(GLfixed depth) { glClearDepth(Fixed(depth)); }
void GL_APIENTRY glOrthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed nearPlane, GLfixed farPlane)
{
    glOrtho(Fixed(left), Fixed(right), Fixed(bottom), Fixed(top), Fixed(nearPlane), Fixed(farPlane));
}
// Texture parameters are enums (filters, wrap modes); GLES passes them unscaled.
void GL_APIENTRY glTexParameterx(GLenum target, GLenum name, GLfixed value) { glTexParameteri(target, name, value); }
void GL_APIENTRY glTexEnvx(GLenum target, GLenum name, GLfixed value)
{
    if (name == GL_RGB_SCALE || name == GL_ALPHA_SCALE) {
        glTexEnvf(target, name, Fixed(value));
    } else {
        glTexEnvi(target, name, value);
    }
}

// These extensions are deliberately absent from the advertised capabilities.
void GL_APIENTRY glPointSizePointerOES(GLenum, GLsizei, const void*) {}
void GL_APIENTRY glDiscardFramebufferEXT(GLenum, GLsizei, const GLenum*) {}

void GL_APIENTRY glResolveMultisampleFramebufferAPPLE()
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    ckzBlitFramebuffer(0, 0, viewport[2], viewport[3], 0, 0, viewport[2], viewport[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

} // extern "C"

namespace ckz {
namespace {

void GL_APIENTRY GlDiagnostic(GLenum, GLenum type, GLuint, GLenum, GLsizei, const char* message, const void*)
{
    if (type == 0x824c) { // GL_DEBUG_TYPE_ERROR
        fprintf(stderr, "OpenGL: %s\n", message);
    }
}

} // namespace

void ConfigureDesktopGl()
{
    fprintf(stderr, "OpenGL renderer: %s (%s)\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));
    if (SDL_getenv("CKZ_GL_DEBUG") != nullptr) {
        using DebugCallback = decltype(&GlDiagnostic);
        using SetCallback = void (GL_APIENTRY*)(DebugCallback, const void*);
        auto callback = reinterpret_cast<SetCallback>(SDL_GL_GetProcAddress("glDebugMessageCallback"));
        if (callback != nullptr) {
            glEnable(0x92e0); // GL_DEBUG_OUTPUT
            glEnable(0x8242); // GL_DEBUG_OUTPUT_SYNCHRONOUS
            callback(GlDiagnostic, nullptr);
        }
    }
}

} // namespace ckz
