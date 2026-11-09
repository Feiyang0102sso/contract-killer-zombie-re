// Swerve's GLES 1.1 and C imports use the ARM base procedure-call standard.
#include "ArmImports.h"
#include "ArmCall.h"
#include <SwerveApi.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <math.h>
#include <string.h>
#include <utility>

extern "C" {
void GL_APIENTRY glFogxv(GLenum name, const GLfixed* values);
void GL_APIENTRY glLightModelx(GLenum name, GLfixed value);
}

namespace ckz::arm {
namespace {

template<class T>
T Unpack(uint32_t word)
{
    if constexpr (std::is_pointer_v<T>) {
        if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
            return reinterpret_cast<T>(word);
        } else {
            return static_cast<T>(const_cast<void*>(ResolvePointer(word)));
        }
    } else if constexpr (std::is_same_v<T, float>) {
        float value;
        memcpy(&value, &word, sizeof(value));
        return value;
    } else {
        return static_cast<T>(word);
    }
}

template<class Result, class... Args, class Function, size_t... indices>
uint64_t CallNative(Function function, const uint32_t* words, std::index_sequence<indices...>)
{
    if constexpr (std::is_void_v<Result>) {
        function(Unpack<Args>(words[indices])...);
        return 0;
    } else {
        return Pack(function(Unpack<Args>(words[indices])...));
    }
}

template<class Result, class... Args>
uint64_t CallNative(Result (*function)(Args...), const uint32_t* words)
{
    return CallNative<Result, Args...>(function, words, std::index_sequence_for<Args...>{});
}

template<class Result, class... Args>
uint64_t CallNative(Result (__stdcall *function)(Args...), const uint32_t* words)
{
    return CallNative<Result, Args...>(function, words, std::index_sequence_for<Args...>{});
}

uint64_t WideArg(const uint32_t* words)
{
    return static_cast<uint64_t>(words[0]) | (static_cast<uint64_t>(words[1]) << 32);
}

uint64_t Divide64(const uint32_t* words)
{
    return static_cast<int64_t>(WideArg(words)) / static_cast<int64_t>(WideArg(words + 2));
}

uint64_t ArcTan(const uint32_t* words)
{
    double first;
    double second;
    memcpy(&first, words, sizeof(first));
    memcpy(&second, words + 2, sizeof(second));
    const double angle = atan2(first, second);
    uint64_t result;
    memcpy(&result, &angle, sizeof(result));
    return result;
}

struct Import {
    const char* name;
    NativeImport call;
};

#define IMPORT(Name) { #Name, +[](const uint32_t* args) { return CallNative(&Name, args); } }

const Import imports[] = {
    { "__divdi3", Divide64 },
    { "__divsi3", +[](const uint32_t* args) -> uint64_t { return Unpack<int>(args[0]) / Unpack<int>(args[1]); } },
    { "__modsi3", +[](const uint32_t* args) -> uint64_t { return Unpack<int>(args[0]) % Unpack<int>(args[1]); } },
    { "__udivsi3", +[](const uint32_t* args) -> uint64_t { return args[0] / args[1]; } },
    { "atan2", ArcTan },
    IMPORT(cosf),
    IMPORT(glActiveTexture),
    IMPORT(glAlphaFunc),
    IMPORT(glBindBuffer),
    IMPORT(glBindFramebufferMALIJ297),
    IMPORT(glBindRenderbufferMALIJ297),
    IMPORT(glBindTexture),
    IMPORT(glBindVertexArrayMALIJ297),
    IMPORT(glBlendFunc),
    IMPORT(glBufferData),
    IMPORT(glBufferSubData),
    IMPORT(glClear),
    IMPORT(glClearColor),
    IMPORT(glClearDepthf),
    IMPORT(glClientActiveTexture),
    IMPORT(glColor4f),
    IMPORT(glColorMask),
    IMPORT(glColorPointer),
    IMPORT(glCompressedTexImage2D),
    IMPORT(glCullFace),
    IMPORT(glDeleteBuffers),
    IMPORT(glDeleteFramebuffersMALIJ297),
    IMPORT(glDeleteRenderbuffersMALIJ297),
    IMPORT(glDeleteTextures),
    IMPORT(glDeleteVertexArraysMALIJ297),
    IMPORT(glDepthFunc),
    IMPORT(glDepthMask),
    IMPORT(glDepthRangef),
    IMPORT(glDisable),
    IMPORT(glDisableClientState),
    IMPORT(glDrawArrays),
    IMPORT(glDrawElements),
    IMPORT(glEnable),
    IMPORT(glEnableClientState),
    IMPORT(glFogf),
    IMPORT(glFogx),
    IMPORT(glFogxv),
    IMPORT(glFramebufferRenderbufferMALIJ297),
    IMPORT(glFramebufferTexture2DMALIJ297),
    IMPORT(glFrontFace),
    IMPORT(glGenBuffers),
    IMPORT(glGenFramebuffersMALIJ297),
    IMPORT(glGenRenderbuffersMALIJ297),
    IMPORT(glGenTextures),
    IMPORT(glGenVertexArraysMALIJ297),
    IMPORT(glGenerateMipmapMALIJ297),
    IMPORT(glGetError),
    IMPORT(glHint),
    IMPORT(glLightModelfv),
    IMPORT(glLightModelx),
    IMPORT(glLightf),
    IMPORT(glLightfv),
    IMPORT(glLineWidth),
    IMPORT(glLoadIdentity),
    IMPORT(glLoadMatrixf),
    IMPORT(glMaterialf),
    IMPORT(glMaterialfv),
    IMPORT(glMatrixMode),
    IMPORT(glNormalPointer),
    IMPORT(glPixelStorei),
    IMPORT(glPointParameterfv),
    IMPORT(glPointSize),
    IMPORT(glPointSizePointerOES),
    IMPORT(glPolygonOffset),
    IMPORT(glPopMatrix),
    IMPORT(glPushMatrix),
    IMPORT(glReadPixels),
    IMPORT(glRenderbufferStorageMALIJ297),
    IMPORT(glScissor),
    IMPORT(glShadeModel),
    IMPORT(glTexCoordPointer),
    IMPORT(glTexEnvf),
    IMPORT(glTexEnvfv),
    IMPORT(glTexEnvx),
    IMPORT(glTexImage2D),
    IMPORT(glTexParameterf),
    IMPORT(glTexSubImage2D),
    IMPORT(glVertexPointer),
    IMPORT(glViewport),
    IMPORT(malij297_Adler32),
    IMPORT(malij297_Allocate),
    IMPORT(malij297_DecompressError),
    IMPORT(malij297_DecompressOnData),
    IMPORT(malij297_DecompressOnDataEnd),
    IMPORT(malij297_DecompressOnDataStart),
    IMPORT(malij297_DecompressRGB_ETC),
    IMPORT(malij297_DynamicImage2DBindData),
    IMPORT(malij297_DynamicImage2DBindRef),
    IMPORT(malij297_DynamicImage2DRebindRef),
    IMPORT(malij297_DynamicImage2DReleaseData),
    IMPORT(malij297_DynamicImage2DReleaseRef),
    IMPORT(malij297_ErrorMessageAppend),
    IMPORT(malij297_ErrorMessageBufferDestroy),
    IMPORT(malij297_ErrorMessageBufferInit),
    IMPORT(malij297_Free),
    IMPORT(malij297_GetEnvironment),
    IMPORT(malij297_MemCpy),
    IMPORT(malij297_MemSet),
    IMPORT(malij297_OffScreenImageBind),
    IMPORT(malij297_OffScreenImageMakeCurrent),
    IMPORT(malij297_Panic),
    IMPORT(malij297_PopulateVBO_Contiguous),
    IMPORT(malij297_PrimaryDisplayGetDepth),
    IMPORT(malij297_PrimaryDisplayInitialize),
    IMPORT(malij297_PrimaryDisplayMakeCurrent),
    IMPORT(malij297_PrimaryDisplayUninitialize),
    IMPORT(memcpy),
    IMPORT(sinf),
    IMPORT(tanf),
};
#undef IMPORT

} // namespace

NativeImport FindImport(const char* name)
{
    for (const Import& entry : imports) {
        if (strcmp(name, entry.name) == 0) {
            return entry.call;
        }
    }
    return nullptr;
}

} // namespace ckz::arm
