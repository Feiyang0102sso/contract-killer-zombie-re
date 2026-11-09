// Win32 copy of the recovered API with one ARM word per function-table entry.
#include "ArmCall.h"
// SwerveApi.h - interface between the precompiled Swerve (M3G) runtime, its iOS porting layer
// (malij297_*.cpp) and the Glu wrapper (CSwerve*.cpp).
// PROVISIONAL (created by b040x): the runtime ships without symbols or headers; the object
// "interfaces" are C structs whose first word points to a table of function pointers, laid out
// from the call sites in CSwerve.cpp / CSwerveGraphics3D.cpp / CSwerveLoader.cpp. Names are
// descriptive guesses; unknown table slots are padding.
// b104: unified with the recovered porting layer (b101a/b/c): full environment layout, Image2D
// table, loader / VBO / dynamic image / decompressor types and every malij297 entry point.
#ifndef SWERVEAPI_H
#define SWERVEAPI_H

#include <stddef.h>

namespace com { namespace glu { namespace platform { namespace graphics {
class ICRenderSurface;
} } } }

/** Object kinds the runtime creates (ISwerve::CreateObject). */
enum SwerveObjectType {
    SwerveObjectType_Graphics3D = 8,
    SwerveObjectType_Image2D = 10,
    SwerveObjectType_Loader = 0xe
};

/** Image formats of the runtime (Image2D, loaders, dynamic images). */
enum SwerveImageFormat {
    SwerveImageFormat_Luminance = 0x61,
    SwerveImageFormat_LuminanceAlpha = 0x62,
    SwerveImageFormat_RGB = 0x63,
    SwerveImageFormat_RGBA = 0x64,
    SwerveImageFormat_RGB565 = 0x65
};

struct GLRuntime;

/** Image description (mutable images rendered into render surfaces). */
struct malij297_Image {
    unsigned int m_width;       // 0x00
    unsigned int m_height;      // 0x04
    unsigned int m_format;      // 0x08 SwerveImageFormat_RGBA
    unsigned int m_id;          // 0x0c
    GLRuntime* m_glRuntime;     // 0x10 render state while bound (malij297_OffScreenImageBind)
};

struct malij297_VertexBufferData;
struct malij297_VBOFormat;

/** Rendering environment of the runtime (filled by malij297_GetEnvironment, 0x98 bytes). */
struct malij297_Environment {
    int m_value00;                  // 0x00 = 1
    int m_value04;                  // 0x04 = 1
    int m_value08;                  // 0x08 = 2
    int m_value0c;                  // 0x0c = 0
    int (*m_populateVBO)(const malij297_VertexBufferData* data, void* buffer, malij297_VBOFormat* format);  // 0x10
    int m_value14;                  // 0x14 = 1
    int m_maxViewportDims[2];       // 0x18 GL_MAX_VIEWPORT_DIMS
    int m_maxTextureSize;           // 0x20 GL_MAX_TEXTURE_SIZE
    int m_maxLights;                // 0x24 GL_MAX_LIGHTS (fixed function only)
    int m_maxTextureUnits;          // 0x28 GL_MAX_TEXTURE_UNITS (fixed function only)
    int m_unknown2c[(0x50 - 0x2c) / 4];
    int m_samples;                  // 0x50 GL_SAMPLES
    float m_maxLineWidth;           // 0x54 upper end of GL_ALIASED_LINE_WIDTH_RANGE
    float m_maxPointSize;           // 0x58 upper end of GL_ALIASED_POINT_SIZE_RANGE
    int m_glVersion;                // 0x5c 0x10001: ES 1.1, 0x20000: ES 2.0
    int m_unknown60;                // 0x60
    int m_hasVertexHalfFloat;       // 0x64 OES_vertex_half_float
    int m_hasTextureNPOT;           // 0x68 OES_texture_npot
    int m_hasETC1;                  // 0x6c OES_compressed_ETC1_RGB8_texture
    int m_hasPointSizeArray;        // 0x70 OES_point_size_array
    int m_hasPalettedTexture;       // 0x74 OES_compressed_paletted_texture
    int m_hasFramebufferObject;     // 0x78 OES_framebuffer_object (mutable images restore the previous target)
    int m_hasVertexArrayObject;     // 0x7c OES_vertex_array_object (always cleared again)
    const unsigned char* m_extensions;  // 0x80 GL_EXTENSIONS
    const unsigned char* m_renderer;    // 0x84 GL_RENDERER
    int m_value88;                  // 0x88 = 0
    int m_value8c;                  // 0x8c = 0
    int m_value90;                  // 0x90 = 0
    int m_value94;                  // 0x94 = 0
};

/** One source vertex array: format word (type, signed flag, component count) and its data. */
struct malij297_VertexArray {
    int m_format;               // 0x00
    unsigned char* m_data;      // 0x04 NULL when absent
};

/** Vertex arrays of a Swerve vertex buffer (layout from the loads of PopulateVBO_Contiguous). */
struct malij297_VertexBufferData {
    int m_vertexCount;                      // 0x00
    malij297_VertexArray m_positions;       // 0x04
    malij297_VertexArray m_normals;         // 0x0c
    malij297_VertexArray m_colors;          // 0x14 3 or 4 components
    float m_alpha;                          // 0x1c alpha of RGB colors / scale of RGBA alpha
    malij297_VertexArray m_pointSizes;      // 0x20 one component
    malij297_VertexArray m_matrixIndices;   // 0x28 bytes (OES_matrix_palette)
    malij297_VertexArray m_weights;         // 0x30 (OES_matrix_palette)
    int m_texCoordCount;                    // 0x38
    const malij297_VertexArray* m_texCoords;// 0x3c m_texCoordCount arrays
};

/** Where one attribute lives inside the packed VBO. */
struct malij297_VBOArrayFormat {
    int m_offset;               // 0x00 byte offset of the first vertex, -1 when absent
    int m_size;                 // 0x04 bytes of the attribute per vertex
    int m_type;                 // 0x08 GL component type
    int m_components;           // 0x0c
};

/** Layout of the packed VBO (layout from the stores of PopulateVBO_Contiguous). */
struct malij297_VBOFormat {
    malij297_VBOArrayFormat m_positions;        // 0x00
    malij297_VBOArrayFormat m_normals;          // 0x10
    malij297_VBOArrayFormat m_colors;           // 0x20
    int m_texCoordCount;                        // 0x30 not touched by PopulateVBO_Contiguous (name assumed)
    malij297_VBOArrayFormat* m_texCoords;       // 0x34 one entry per texture coordinate array
    malij297_VBOArrayFormat m_pointSizes;       // 0x38
    malij297_VBOArrayFormat m_matrixIndices;    // 0x48
    malij297_VBOArrayFormat m_weights;          // 0x58
};

/**
 * Image description handed back by the dynamic image bridge (another structure than
 * malij297_Image). BindData writes six words, BindRef five, RebindRef four.
 */
struct malij297_DynamicImage2D {
    unsigned int m_bound;       // 0x00 1 after a successful bind
    unsigned int m_width;       // 0x04
    unsigned int m_height;      // 0x08
    unsigned int m_format;      // 0x0c SwerveImageFormat_*
    unsigned char* m_pixels;    // 0x10 RGBA copy owned by the image (NULL for references)
    unsigned int m_size;        // 0x14 bytes in m_pixels
};

/** Receives one inflated block of the decompressor; returns 0 to stop decompressing. */
typedef int (*malij297_DecompressCallback)(void* userData, int size, const unsigned char* data);

struct malij297_Decompressor;

struct IObject3D;

/** Function table of IObject3D. */
struct IObject3DFunctions {
    void* m_slot00;
    ckz::arm::Function<void(IObject3D* self)> Release;                                           // 0x04
    void* m_slots08[(0x3c - 0x08) / 4];
    ckz::arm::Function<int(IObject3D* self, int userId, IObject3D** found)> Find;                // 0x3c
};

/** A scene graph object. */
struct IObject3D {
    ckz::arm::Table<IObject3DFunctions> m_functions;
};

struct IImage2D;

/** Function table of an Image2D object (an IObject3D; slots used by the loaders). */
struct IImage2DFunctions {
    void* m_slot00;
    ckz::arm::Function<void(IImage2D* self)> Release;                                            // 0x04
    void* m_slots08[(0x3c - 0x08) / 4];
    ckz::arm::Function<int(IImage2D* self, int userId, IObject3D** found)> Find;                 // 0x3c
    void* m_slots40[(0x84 - 0x40) / 4];
    ckz::arm::Function<int(IImage2D* self)> Commit;                                              // 0x84
    void* m_slot88;
    ckz::arm::Function<int(IImage2D* self, unsigned int format, unsigned int width, unsigned int height,
                      int mode)> Initialize;                                                // 0x8c
    ckz::arm::Function<int(IImage2D* self, unsigned int format, unsigned int width, unsigned int height,
                                unsigned int glType, unsigned int glFormat)> InitializeCompressed;    // 0x90
    ckz::arm::Function<int(IImage2D* self, unsigned int dataFormat, int level, int x, int y, int width,
                       int height, unsigned int dataSize, const void* data, int flags)> SetSubImage;   // 0x94
    void* m_slots98[(0xa0 - 0x98) / 4];
    ckz::arm::Function<int(IImage2D* self, unsigned int format, int level, unsigned int indexSize,
                            const void* indices, unsigned int paletteSize, const void* palette)> SetPalettedImage;  // 0xa0
};

/** A 2D image of the scene graph. */
struct IImage2D {
    ckz::arm::Table<IImage2DFunctions> m_functions;
};

/** Properties the runtime reports through ISwerveFunctions::GetProperties (20 bytes). */
struct SwerveRuntimeProperties {
    int m_unknown00[4];     // 0x00
    int m_pipeline;         // 0x10 0: fixed function, 1: programmable
};

struct ISwerve;

/** Function table of the runtime root object. */
struct ISwerveFunctions {
    void* m_slot00;
    ckz::arm::Function<void(ISwerve* self)> Release;                                             // 0x04
    ckz::arm::Function<int(SwerveRuntimeProperties* properties)> GetProperties;                  // 0x08 (no self)
    ckz::arm::Function<int(int type, void** object)> CreateObject;                               // 0x0c (no self)
    void* m_slot10;
    ckz::arm::Function<int(ISwerve* self)> Initialize;                                           // 0x14
    ckz::arm::Function<int(void* object)> GetObjectIdentity;                                     // 0x18 (no self)  // added by b047x
};

/** Root object of the runtime (swvNew). */
struct ISwerve {
    ckz::arm::Table<ISwerveFunctions> m_functions;
};

struct IGraphics3D;

/** Function table of the renderer object. */
struct IGraphics3DFunctions {
    void* m_slot00;
    ckz::arm::Function<void(IGraphics3D* self)> Release;                                         // 0x04
    void* m_slots08[(0x20 - 0x08) / 4];
    ckz::arm::Function<int(IGraphics3D* self, unsigned int targetWidth, unsigned int targetHeight,
                      int x, int y, int width, int height, int flags)> BindTarget;          // 0x20
    ckz::arm::Function<int(IGraphics3D* self)> ReleaseTarget;                                    // 0x24
    void* m_slot28;
    ckz::arm::Function<int(IGraphics3D* self)> ReleaseImageTarget;                               // 0x2c
    void* m_slot30;  // changed by fx2 (was m_slots30[(0x74 - 0x30) / 4])
    ckz::arm::Function<int(IGraphics3D* self, int x, int y, int width, int height)> SetViewport;  // 0x34  // added by fx2 (Window::RunPaint3D)
    void* m_slots38[(0x74 - 0x38) / 4];  // added by fx2
    ckz::arm::Function<int(IGraphics3D* self, int degrees)> SetDisplayRotation;                  // 0x74
    ckz::arm::Function<int(IGraphics3D* self)> OnContextLost;                                    // 0x78
};

/** The renderer. */
struct IGraphics3D {
    ckz::arm::Table<IGraphics3DFunctions> m_functions;
};

struct ILoader;

/** Function table of the M3G stream loader. */
struct ILoaderFunctions {
    void* m_slot00;
    ckz::arm::Function<void(ILoader* self)> Release;                                             // 0x04
    void* m_slot08;
    ckz::arm::Function<int(ILoader* self)> Begin;                                                // 0x0c
    ckz::arm::Function<int(ILoader* self)> End;                                                  // 0x10
    ckz::arm::Function<int(ILoader* self, int bufferSize, const unsigned char* buffer, int offset, int length,
                  int* consumed)> Decode;                                               // 0x14
    ckz::arm::Function<int(ILoader* self, int* count)> GetObjectCount;                           // 0x18
    ckz::arm::Function<int(ILoader* self, int index, IObject3D** object)> GetObject;             // 0x1c
    ckz::arm::Function<int(ILoader* self, int* type)> GetXREFType;                               // 0x20
    ckz::arm::Function<int(ILoader* self, int bufferSize, unsigned char* buffer, int* length)> GetXREFURI;   // 0x24
    ckz::arm::Function<int(ILoader* self, int bufferCount, int* userIds, int* count)> GetXREFUserIDs;      // 0x28
    ckz::arm::Function<int(ILoader* self, IObject3D* object)> ResolveXREF;                       // 0x2c
    ckz::arm::Function<int(ILoader* self, int* format)> GetXREFImageFormat;                      // 0x30
};

/** Incremental M3G loader. */
struct ILoader {
    ckz::arm::Table<ILoaderFunctions> m_functions;
};

/** Vertex buffer filler (C++ linkage, malij297_glpopulatevbo.cpp); returns the byte size written. */
int malij297_PopulateVBO_Contiguous(const malij297_VertexBufferData* data, void* buffer, malij297_VBOFormat* format);

extern "C" {
/** Create the runtime root object. Returns 0 on success. (The only exported runtime symbol.) */
int swvNew(ISwerve** swerve);

// ---- malij297_runtime.cpp ----
void* malij297_Allocate(size_t size);
void malij297_Free(void* pointer);
void malij297_MemSet(void* destination, int value, size_t size);
void malij297_MemCpy(void* destination, const void* source, size_t size);
void malij297_Panic();

// ---- malij297_error.cpp ----
int malij297_ErrorMessageBufferInit();
void malij297_ErrorMessageReset();
void malij297_ErrorMessageBufferDestroy();
const char* malij297_ErrorMessageGet();
void malij297_ErrorMessageAppend(const char* message);

// ---- malij297_glruntime.cpp ----
/** Bind `image` to a render surface and make it the target. Returns 1 on success. */
int malij297_OffScreenImageBind(malij297_Image* image);
int malij297_OffScreenImageMakeCurrent(malij297_Image* image);
/** Fill `environment`; returns 0 on failure. */
int malij297_GetEnvironment(malij297_Environment* environment);
int malij297_PrimaryDisplayInitialize();
void malij297_PrimaryDisplayUninitialize();
int malij297_PrimaryDisplayMakeCurrent();
int malij297_PrimaryDisplayGetDepth();
// OES framebuffer / vertex array object entry points under engine-private names (GL types spelled out)
void glBindRenderbufferMALIJ297(unsigned int target, unsigned int renderbuffer);
void glDeleteRenderbuffersMALIJ297(int count, const unsigned int* renderbuffers);
void glGenRenderbuffersMALIJ297(int count, unsigned int* renderbuffers);
void glRenderbufferStorageMALIJ297(unsigned int target, unsigned int internalFormat, int width, int height);
void glBindFramebufferMALIJ297(unsigned int target, unsigned int framebuffer);
void glDeleteFramebuffersMALIJ297(int count, const unsigned int* framebuffers);
void glGenFramebuffersMALIJ297(int count, unsigned int* framebuffers);
void glFramebufferRenderbufferMALIJ297(unsigned int target, unsigned int attachment, unsigned int renderbufferTarget,
                                       unsigned int renderbuffer);
void glFramebufferTexture2DMALIJ297(unsigned int target, unsigned int attachment, unsigned int textureTarget,
                                    unsigned int texture, int level);
void glGenerateMipmapMALIJ297(unsigned int target);
void glBindVertexArrayMALIJ297(unsigned int array);
void glDeleteVertexArraysMALIJ297(int count, const unsigned int* arrays);
void glGenVertexArraysMALIJ297(int count, unsigned int* arrays);

// ---- malij297_pngload.cpp / malij297_jpegload.cpp / malij297_ktxload.cpp ----
// `formatHint` is the requested image format (0: the decoded one); KTX ignores it.
int malij297_HasPNGIdentifier(unsigned int size, const unsigned char* data);
int malij297_HasJPEGIdentifier(unsigned int size, const unsigned char* data);
int malij297_HasKTXIdentifier(unsigned int size, const unsigned char* data);
int malij297_PNGLoadBuffer(ISwerve* swerve, unsigned int size, const unsigned char* data,
                           unsigned int formatHint, IImage2D** image);
int malij297_JPEGLoadBuffer(ISwerve* swerve, unsigned int size, const unsigned char* data,
                            unsigned int formatHint, IImage2D** image);
int malij297_KTXLoadBuffer(ISwerve* swerve, unsigned int size, const unsigned char* data,
                           unsigned int formatHint, IImage2D** image);

// ---- malij297_etcdecompress.cpp ----
/** Decode an ETC1 image into RGB888 (or RGB565 when `format` is SwerveImageFormat_RGB565). */
int malij297_DecompressRGB_ETC(int width, int height, const unsigned char* data, unsigned char* output, int format);

// ---- malij297_dynamicimage2d_runtime.cpp ----
int malij297_DynamicImage2DBindData(com::glu::platform::graphics::ICRenderSurface* surface, void* context,
                                    malij297_DynamicImage2D* image);
void malij297_DynamicImage2DReleaseData(com::glu::platform::graphics::ICRenderSurface* surface, void* context,
                                        malij297_DynamicImage2D* image);
int malij297_DynamicImage2DBindRef(com::glu::platform::graphics::ICRenderSurface* surface,
                                   malij297_DynamicImage2D* image);
void malij297_DynamicImage2DReleaseRef();
int malij297_DynamicImage2DRebindRef(com::glu::platform::graphics::ICRenderSurface* surface, void* context,
                                     malij297_DynamicImage2D* image);

// ---- malij297_decompressorapi.cpp ----
int malij297_DecompressOnDataStart(unsigned int format, void* userData, malij297_DecompressCallback callback,
                                   malij297_Decompressor** handle);
int malij297_DecompressOnData(const unsigned char* data, unsigned int size, malij297_Decompressor** handle);
int malij297_DecompressOnDataEnd(malij297_Decompressor** handle);
void malij297_DecompressError(malij297_Decompressor** handle);
unsigned long malij297_Adler32(unsigned long adler, const unsigned char* data, unsigned int size);
}

#endif
