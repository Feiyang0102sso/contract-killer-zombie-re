// malij297_glruntime.cpp - OpenGL ES runtime of the Swerve (M3G) porting layer on iOS.
// The precompiled Swerve core calls these C entry points to query the GL environment, to render
// into off-screen (mutable) images through Glu render surfaces, and to reach the OES framebuffer /
// vertex array extensions under engine-private names (gl*MALIJ297).
#include <string.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "CSwerve.h"
#include "CSwerveGraphics3D.h"
#include "ICGraphics.h"

using namespace com::glu::platform;

// Version codes stored in malij297_Environment::m_glVersion.
static const int GLVersion_ES11 = 0x10001;
static const int GLVersion_ES20 = 0x20000;

// Pipeline reported by the runtime (SwerveRuntimeProperties::m_pipeline).
static const int Pipeline_FixedFunction = 0;
static const int Pipeline_Programmable = 1;

extern "C" {

/** Create the render state of `image` and make its surface the current target. Returns 1 on success. */
int malij297_OffScreenImageBind(malij297_Image* image)
{
    GLRuntime* runtime = new GLRuntime();
    if (runtime == NULL) {
        return 0;
    }

    graphics::ICRenderSurface* surface = CSwerve::GetInstance()->m_graphics3D->GetMutableSurface(image);
    if (surface != NULL) {
        runtime->m_surface = surface;
        runtime->m_previousTarget = graphics::ICGraphics::GetInstance()->GetTargetSurface();
        image->m_glRuntime = runtime;
        graphics::ICRenderSurface* target = runtime->m_surface;
        graphics::ICGraphics::GetInstance()->SetTargetSurface(*target);
        return 1;
    }

    delete runtime;
    return 0;
}

/** Make the surface of a bound `image` the current target again. Returns 0 if it has none. */
int malij297_OffScreenImageMakeCurrent(malij297_Image* image)
{
    GLRuntime* runtime = image->m_glRuntime;
    if (runtime == NULL) {
        return 0;
    }
    graphics::ICRenderSurface* surface = runtime->m_surface;
    if (surface == NULL) {
        return 0;
    }
    graphics::ICGraphics::GetInstance()->SetTargetSurface(*surface);
    return 1;
}

/** Fill `environment` with the limits and extensions of the current GL context. Always returns 1. */
int malij297_GetEnvironment(malij297_Environment* environment)
{
    malij297_Environment* env = environment;
    env->m_value00 = 1;
    env->m_value8c = 0;
    env->m_value04 = 1;
    env->m_value08 = 2;
    env->m_value0c = 0;
    env->m_populateVBO = malij297_PopulateVBO_Contiguous;
    env->m_value14 = 1;
    env->m_value90 = 0;
    env->m_value94 = 0;

    GLfloat range[2];
    SwerveRuntimeProperties properties;
    ISwerve* swerve = CSwerve::GetInstance()->m_swerve;
    swerve->m_functions->GetProperties(&properties);

    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, env->m_maxViewportDims);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &env->m_maxTextureSize);
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
    env->m_maxLineWidth = range[1];
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, range);
    env->m_maxPointSize = range[1];
    glGetIntegerv(GL_SAMPLES, &env->m_samples);
    if (properties.m_pipeline == Pipeline_FixedFunction) {
        glGetIntegerv(GL_MAX_LIGHTS, &env->m_maxLights);
        glGetIntegerv(GL_MAX_TEXTURE_UNITS, &env->m_maxTextureUnits);
    }

    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    if (strstr(extensions, "OES_vertex_half_float") != NULL) {
        env->m_hasVertexHalfFloat = 1;
    }
    if (strstr(extensions, "OES_texture_npot") != NULL) {
        env->m_hasTextureNPOT = 1;
    }
    if (strstr(extensions, "OES_compressed_ETC1_RGB8_texture") != NULL) {
        env->m_hasETC1 = 1;
    }
    if (strstr(extensions, "OES_point_size_array") != NULL) {
        env->m_hasPointSizeArray = 1;
    }
    if (strstr(extensions, "OES_compressed_paletted_texture") != NULL) {
        env->m_hasPalettedTexture = 1;
    }
    if (strstr(extensions, "OES_framebuffer_object") != NULL) {
        env->m_hasFramebufferObject = 1;
    }
    if (strstr(extensions, "OES_vertex_array_object") != NULL) {
        env->m_hasVertexArrayObject = 1;
    }
    // the original turns vertex array objects off right after detecting them
    env->m_hasVertexArrayObject = 0;
    env->m_extensions = (const GLubyte*)extensions;

    const char* version = (const char*)glGetString(GL_VERSION);
    if (strstr(version, "OpenGL ES-CM 1.1") != NULL) {
        env->m_glVersion = GLVersion_ES11;
    }
    if (strstr(version, "OpenGL ES 2.0") != NULL) {
        env->m_glVersion = GLVersion_ES20;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    env->m_renderer = renderer;
    // PVRVFrame is the PowerVR desktop emulator
    const char* emulator = strstr((const char*)renderer, "PVRVFrame"); // PORT: C++ UCRT const overload.
    env->m_value88 = 0;
    if (emulator != NULL) {
        env->m_hasVertexHalfFloat = 0;
        if (properties.m_pipeline == Pipeline_Programmable) {
            env->m_hasPalettedTexture = 0;
        }
    }
    return 1;
}

/** The primary display needs no setup on iOS. */
int malij297_PrimaryDisplayInitialize()
{
    return 1;
}

void malij297_PrimaryDisplayUninitialize()
{
}

int malij297_PrimaryDisplayMakeCurrent()
{
    return 1;
}

/** Colour depth of the primary display in bits. */
int malij297_PrimaryDisplayGetDepth()
{
    return 16;
}

// Engine-private names of the OES framebuffer object extension.
void glBindRenderbufferMALIJ297(GLenum target, GLuint renderbuffer)
{
    glBindRenderbufferOES(target, renderbuffer);
}

void glDeleteRenderbuffersMALIJ297(GLsizei count, const GLuint* renderbuffers)
{
    glDeleteRenderbuffersOES(count, renderbuffers);
}

void glGenRenderbuffersMALIJ297(GLsizei count, GLuint* renderbuffers)
{
    glGenRenderbuffersOES(count, renderbuffers);
}

void glRenderbufferStorageMALIJ297(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height)
{
    glRenderbufferStorageOES(target, internalFormat, width, height);
}

void glBindFramebufferMALIJ297(GLenum target, GLuint framebuffer)
{
    glBindFramebufferOES(target, framebuffer);
}

void glDeleteFramebuffersMALIJ297(GLsizei count, const GLuint* framebuffers)
{
    glDeleteFramebuffersOES(count, framebuffers);
}

void glGenFramebuffersMALIJ297(GLsizei count, GLuint* framebuffers)
{
    glGenFramebuffersOES(count, framebuffers);
}

void glFramebufferRenderbufferMALIJ297(GLenum target, GLenum attachment, GLenum renderbufferTarget,
                                       GLuint renderbuffer)
{
    glFramebufferRenderbufferOES(target, attachment, renderbufferTarget, renderbuffer);
}

void glFramebufferTexture2DMALIJ297(GLenum target, GLenum attachment, GLenum textureTarget, GLuint texture,
                                    GLint level)
{
    glFramebufferTexture2DOES(target, attachment, textureTarget, texture, level);
}

void glGenerateMipmapMALIJ297(GLenum target)
{
    glGenerateMipmapOES(target);
}

// Engine-private names of the OES vertex array object extension.
void glBindVertexArrayMALIJ297(GLuint array)
{
    glBindVertexArrayOES(array);
}

void glDeleteVertexArraysMALIJ297(GLsizei count, const GLuint* arrays)
{
    glDeleteVertexArraysOES(count, arrays);
}

void glGenVertexArraysMALIJ297(GLsizei count, GLuint* arrays)
{
    glGenVertexArraysOES(count, arrays);
}

}
