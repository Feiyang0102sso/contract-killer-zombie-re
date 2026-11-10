// Native Swerve renderer (IGraphics3D of SwerveApi.h plus the IGraphics3DRenderer slots of tws.h).
// CSwerveGraphics3D binds the game's render surface itself (an FBO or the window), then calls
// BindTarget / SetViewport / Render / ReleaseTarget; m3gcore draws into whatever framebuffer is bound.
//
// The game's 2D renderer shares the GL context. m3gcore sets every state it needs for each draw but
// leaves its state behind, so the game's GL state is saved at BindTarget and restored at ReleaseTarget.
#include "NativeRuntime.h"
#include "CkzEgl.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <set>

extern "C" {
// Desktop OpenGL 1.1 calls (opengl32) outside the GLES headers.
void GL_APIENTRY glPushAttrib(GLbitfield mask);
void GL_APIENTRY glPopAttrib(void);
void GL_APIENTRY glPushClientAttrib(GLbitfield mask);
void GL_APIENTRY glPopClientAttrib(void);
}

namespace ckz::native {
namespace {

constexpr GLbitfield AllAttributeBits = 0xffffffff;
constexpr GLenum ArrayBufferBinding = 0x8894;
constexpr GLenum ElementArrayBufferBinding = 0x8895;
constexpr GLenum VertexArrayBinding = 0x85b5;
constexpr GLenum TextureMatrixMode = 0x1702;
// m3gcore uses two texture units (M3G_NUM_TEXTURE_UNITS).
constexpr int SavedTextureUnits = 2;

struct Graphics3DObject : Object {
    M3GRenderContext context;
    EGLSurface surface;
    bool bound;
    int rotation;
    // GL state of the game, restored at ReleaseTarget
    GLint savedArrayBuffer;
    GLint savedElementBuffer;
    GLint savedVertexArray;
    GLint savedActiveTexture;
    GLint savedClientActiveTexture;
};

Graphics3DObject* AsGraphics(Object* object)
{
    return static_cast<Graphics3DObject*>(object);
}

void DestroyGraphics(Object* object)
{
    Graphics3DObject* graphics = AsGraphics(object);
    if (graphics->context != nullptr) {
        m3gDeleteObject(reinterpret_cast<M3GObject>(graphics->context));
    }
    if (graphics->surface != EGL_NO_SURFACE) {
        egl::DestroySurface(graphics->surface);
    }
    delete graphics;
}

/** Save the game's GL state and give m3gcore a clean one (no buffer objects, its own defaults). */
void SaveGameState(Graphics3DObject* graphics)
{
    glGetIntegerv(ArrayBufferBinding, &graphics->savedArrayBuffer);
    glGetIntegerv(ElementArrayBufferBinding, &graphics->savedElementBuffer);
    glGetIntegerv(VertexArrayBinding, &graphics->savedVertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &graphics->savedActiveTexture);
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &graphics->savedClientActiveTexture);
    glPushAttrib(AllAttributeBits);
    glPushClientAttrib(AllAttributeBits);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    for (int unit = 0; unit < SavedTextureUnits; unit++) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glMatrixMode(TextureMatrixMode);
        glPushMatrix();
    }
    glActiveTexture(GL_TEXTURE0);
    glMatrixMode(GL_MODELVIEW);

    // m3gcore draws from client memory, never from buffer objects.
    glBindVertexArrayOES(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // m3gcore's m3gSetGLDefaults, which it only runs once when it believes it made a new context.
    static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glEnable(GL_NORMALIZE);
    glEnable(GL_SCISSOR_TEST);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // A Background clears depth with whatever clear value is set; m3gcore only sets it without one.
    glClearDepthf(1.0f);
}

void RestoreGameState(Graphics3DObject* graphics)
{
    for (int unit = SavedTextureUnits - 1; unit >= 0; unit--) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glMatrixMode(TextureMatrixMode);
        glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
    glActiveTexture(static_cast<GLenum>(graphics->savedActiveTexture));
    glClientActiveTexture(static_cast<GLenum>(graphics->savedClientActiveTexture));
    glBindVertexArrayOES(static_cast<GLuint>(graphics->savedVertexArray));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(graphics->savedArrayBuffer));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(graphics->savedElementBuffer));
}

/** Bind the game's current framebuffer (targetWidth x targetHeight) with viewport x, y, width, height. */
int BindTarget(Object* self, unsigned int targetWidth, unsigned int targetHeight, int x, int y, int width,
               int height, int flags)
{
    Graphics3DObject* graphics = AsGraphics(self);
    if (graphics->bound) {
        return ErrorInvalidState;
    }
    static bool flagsLogged = false;
    if (!flagsLogged) {
        flagsLogged = true;
        fprintf(stderr, "Swerve native: first BindTarget %ux%u viewport %d,%d %dx%d flags 0x%x\n",
                targetWidth, targetHeight, x, y, width, height, flags);
    }
    if (graphics->surface == EGL_NO_SURFACE) {
        graphics->surface = egl::CreateSurface(static_cast<EGLint>(targetWidth), static_cast<EGLint>(targetHeight));
    } else {
        egl::ResizeSurface(graphics->surface, static_cast<EGLint>(targetWidth), static_cast<EGLint>(targetHeight));
    }
    SaveGameState(graphics);
    TakeError();
    m3gBindEGLSurfaceTarget(graphics->context, static_cast<M3GEGLSurface>(reinterpret_cast<uintptr_t>(graphics->surface)));
    int error = TakeError();
    if (error != ErrorNone) {
        RestoreGameState(graphics);
        return error;
    }
    m3gSetViewport(graphics->context, x, y, width, height);
    graphics->bound = true;
    return TakeError();
}

int ReleaseTarget(Object* self)
{
    Graphics3DObject* graphics = AsGraphics(self);
    if (!graphics->bound) {
        return ErrorNone;
    }
    m3gReleaseTarget(graphics->context);
    graphics->bound = false;
    RestoreGameState(graphics);
    return TakeError();
}

int SetViewport(Object* self, int x, int y, int width, int height)
{
    m3gSetViewport(AsGraphics(self)->context, x, y, width, height);
    return TakeError();
}

/** Render `node` with `transform` using the camera and lights of the last rendered world. */
void RenderNode(Object* self, Object* node, Object* transform)
{
    Graphics3DObject* graphics = AsGraphics(self);
    if (!graphics->bound || node == nullptr) {
        return;
    }
    m3gRenderNode(graphics->context, reinterpret_cast<M3GNode>(node->handle), MatrixOf(transform));
    TakeError();
}

/** Debugging (CKZ_NATIVE_TRACE): describe each world the first time it is rendered. */
void TraceWorld(Object* world)
{
    static std::set<Object*> traced;
    if (SDL_getenv("CKZ_NATIVE_TRACE") == nullptr || !traced.insert(world).second) {
        return;
    }
    const M3GWorld handle = reinterpret_cast<M3GWorld>(world->handle);
    const M3GCamera camera = m3gGetActiveCamera(handle);
    fprintf(stderr, "Swerve native: render world %p, %d children, camera %p\n", static_cast<void*>(world),
            m3gGetChildCount(reinterpret_cast<M3GGroup>(handle)), static_cast<void*>(camera));
    if (camera == nullptr) {
        return;
    }
    M3Gfloat params[4] = {};
    const M3Gint type = m3gGetProjectionAsParams(camera, params);
    M3GMatrix toWorld;
    const M3Gbool connected = m3gGetTransformTo(reinterpret_cast<M3GNode>(camera), reinterpret_cast<M3GNode>(handle), &toWorld);
    float rows[16] = {};
    m3gGetMatrixRows(&toWorld, rows);
    fprintf(stderr, "  projection type %d params %g %g %g %g; camera->world %d translation %g %g %g\n", type,
            params[0], params[1], params[2], params[3], connected, rows[3], rows[7], rows[11]);
}

void RenderWorld(Object* self, Object* world)
{
    Graphics3DObject* graphics = AsGraphics(self);
    if (!graphics->bound || world == nullptr) {
        return;
    }
    TraceWorld(world);
    m3gRenderWorld(graphics->context, reinterpret_cast<M3GWorld>(world->handle));
    TakeError();
}

/** Only an upright display is supported (the PC port never rotates). */
int SetDisplayRotation(Object* self, int degrees)
{
    Graphics3DObject* graphics = AsGraphics(self);
    if (degrees != 0 && degrees != graphics->rotation) {
        fprintf(stderr, "Swerve native: display rotation %d is not supported\n", degrees);
    }
    graphics->rotation = degrees;
    return ErrorNone;
}

int OnContextLost(Object* self)
{
    m3gFreeGLESResources(AsGraphics(self)->context);
    return ErrorNone;
}

} // namespace

Object* CreateGraphics3D()
{
    Graphics3DObject* graphics = new Graphics3DObject();
    InitObject(graphics, ClassGraphics3D);
    graphics->context = m3gCreateContext(Interface());
    graphics->surface = EGL_NO_SURFACE;
    graphics->bound = false;
    graphics->rotation = 0;
    if (graphics->context == nullptr) {
        fprintf(stderr, "Swerve native: m3gCreateContext failed\n");
    }
    return graphics;
}

void FillGraphics3DSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    objectClass.destroy = DestroyGraphics;
    SetSlot(objectClass, 8, &BindTarget);            // 0x20
    SetSlot(objectClass, 9, &ReleaseTarget);         // 0x24
    SetSlot(objectClass, 11, &ReleaseTarget);        // 0x2c ReleaseImageTarget (no image targets)
    SetSlot(objectClass, 13, &SetViewport);          // 0x34
    SetSlot(objectClass, 17, &RenderNode);           // 0x44
    SetSlot(objectClass, 18, &RenderWorld);          // 0x48
    SetSlot(objectClass, 29, &SetDisplayRotation);   // 0x74
    SetSlot(objectClass, 30, &OnContextLost);        // 0x78
}

} // namespace ckz::native
