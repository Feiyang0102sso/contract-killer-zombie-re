// CGraphics_OGLES.h - OpenGL ES (1.1 fixed function) graphics device.
// (first drafted by b002 as PROVISIONAL; completed by b037x, the owner)
#ifndef CGRAPHICS_OGLES_H
#define CGRAPHICS_OGLES_H

#include <OpenGLES/ES1/gl.h>
#include "CGraphics.h"
#include "ICShaderProgram.h"
#include "CStrWChar.h"
#include "CVector.h"
#include "CTypedVariableTable.h"
#include "CArrayInputStream.h"
#include "CIdToObjectRouter.h"
#include "CProxy_OGLES.h"

namespace com { namespace glu { namespace platform {

namespace systems {
class CEvent;
}

namespace graphics {

/**
 * Last values sent to OpenGL, so that redundant calls are skipped. Every value has a dirty flag;
 * a dirty value is always sent again (Invalidate() after the context was touched elsewhere).
 */
struct CGraphics_OGLES_GLState {
    CGraphics_OGLES_GLState()
    {
        Invalidate();
        m_activeTexture = 0;
    }

    /** Mark every cached value as unknown. */
    inline void Invalidate()
    {
        m_activeTextureDirty = 1;
        m_blendDirty = 1;
        m_bindTextureDirty = 1;
        m_blendFuncDirty = 1;
        m_cullFaceDirty = 1;
        m_cullFaceEnableDirty = 1;
        m_clearColorDirty = 1;
        m_clearDepthDirty = 1;
        m_colorMaskDirty = 1;
        m_depthFuncDirty = 1;
        m_depthMaskDirty = 1;
        m_depthRangeDirty = 1;
        m_depthTestDirty = 1;
        m_frontFaceDirty = 1;
        m_polygonOffsetDirty = 1;
        m_scissorDirty = 1;
        m_scissorTestDirty = 1;
        m_stencilMaskDirty = 1;
        m_viewportDirty = 1;
    }

    // capability switches (member functions: accessed through a reference clang 211 assumes a wrong alignment and widens the loads)
    /** glEnable / glDisable(GL_BLEND) unless already in that state. */
    inline void EnableBlend(unsigned char enable)
    {
        if (enable) {
            if (m_blendDirty || !m_blendEnabled) {
                m_blendDirty = 0;
                m_blendEnabled = 1;
                glEnable(GL_BLEND);
            }
        } else {
            if (m_blendDirty || m_blendEnabled) {
                m_blendDirty = 0;
                m_blendEnabled = 0;
                glDisable(GL_BLEND);
            }
        }
    }

    /** glEnable / glDisable(GL_CULL_FACE) unless already in that state. */
    inline void EnableCullFace(unsigned char enable)
    {
        if (enable) {
            if (m_cullFaceEnableDirty || !m_cullFaceEnabled) {
                m_cullFaceEnableDirty = 0;
                m_cullFaceEnabled = 1;
                glEnable(GL_CULL_FACE);
            }
        } else {
            if (m_cullFaceEnableDirty || m_cullFaceEnabled) {
                m_cullFaceEnableDirty = 0;
                m_cullFaceEnabled = 0;
                glDisable(GL_CULL_FACE);
            }
        }
    }

    /** glEnable / glDisable(GL_DEPTH_TEST) unless already in that state. */
    inline void EnableDepthTest(unsigned char enable)
    {
        if (enable) {
            if (m_depthTestDirty || !m_depthTestEnabled) {
                m_depthTestDirty = 0;
                m_depthTestEnabled = 1;
                glEnable(GL_DEPTH_TEST);
            }
        } else {
            if (m_depthTestDirty || m_depthTestEnabled) {
                m_depthTestDirty = 0;
                m_depthTestEnabled = 0;
                glDisable(GL_DEPTH_TEST);
            }
        }
    }

    /** glEnable / glDisable(GL_SCISSOR_TEST) unless already in that state. */
    inline void EnableScissorTest(unsigned char enable)
    {
        if (enable) {
            if (m_scissorTestDirty || !m_scissorTestEnabled) {
                m_scissorTestDirty = 0;
                m_scissorTestEnabled = 1;
                glEnable(GL_SCISSOR_TEST);
            }
        } else {
            if (m_scissorTestDirty || m_scissorTestEnabled) {
                m_scissorTestDirty = 0;
                m_scissorTestEnabled = 0;
                glDisable(GL_SCISSOR_TEST);
            }
        }
    }

    inline void ActiveTexture(GLenum texture)
    {
        if (m_activeTextureDirty || m_activeTexture != texture) {
            m_activeTextureDirty = 0;
            m_activeTexture = texture;
            glActiveTexture(texture);
        }
    }

    /**
     * Bind `texture` to `target` of the active unit unless it already is. The name is read once up
     * front; glBindTexture gets it through the reference again (as the original).
     */
    inline void BindTexture(GLenum target, const GLuint& texture)
    {
        GLuint name = texture;
        if (m_bindTextureDirty || m_textureTargets[m_activeTexture - GL_TEXTURE0] != target ||
            m_textures[m_activeTexture - GL_TEXTURE0] != name) {
            m_bindTextureDirty = 0;
            m_textureTargets[m_activeTexture - GL_TEXTURE0] = target;
            m_textures[m_activeTexture - GL_TEXTURE0] = name;
            glBindTexture(target, texture);
        }
    }

    /** BindTexture() of `textures[index]` (the array pointer is read again for the glBindTexture call). */
    inline void BindTexture(GLenum target, GLuint* const& textures, unsigned int index)
    {
        GLuint name = textures[index];
        if (m_bindTextureDirty || m_textureTargets[m_activeTexture - GL_TEXTURE0] != target ||
            m_textures[m_activeTexture - GL_TEXTURE0] != name) {
            m_bindTextureDirty = 0;
            m_textureTargets[m_activeTexture - GL_TEXTURE0] = target;
            m_textures[m_activeTexture - GL_TEXTURE0] = name;
            glBindTexture(target, textures[index]);
        }
    }

    inline void BlendFunc(GLenum source, GLenum destination)
    {
        if (m_blendFuncDirty || m_blendSource != source || m_blendDestination != destination) {
            m_blendFuncDirty = 0;
            m_blendSource = source;
            m_blendDestination = destination;
            glBlendFunc(source, destination);
        }
    }

    inline void ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
    {
        if (m_clearColorDirty || m_clearColor[0] != red || m_clearColor[1] != green || m_clearColor[2] != blue ||
            m_clearColor[3] != alpha) {
            m_clearColorDirty = 0;
            m_clearColor[0] = red;
            m_clearColor[1] = green;
            m_clearColor[2] = blue;
            m_clearColor[3] = alpha;
            glClearColor(red, green, blue, alpha);
        }
    }

    inline void ClearDepth(GLfloat depth)
    {
        if (m_clearDepthDirty || m_clearDepth != depth) {
            m_clearDepthDirty = 0;
            m_clearDepth = depth;
            glClearDepthf(depth);
        }
    }

    inline void ClearStencil(GLint stencil)
    {
        if (m_clearStencilDirty || m_clearStencil != stencil) {
            m_clearStencilDirty = 0;
            m_clearStencil = stencil;
            glClearStencil(stencil);
        }
    }

    inline void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
    {
        if (m_colorMaskDirty || m_colorMask[0] != red || m_colorMask[1] != green || m_colorMask[2] != blue ||
            m_colorMask[3] != alpha) {
            m_colorMaskDirty = 0;
            m_colorMask[0] = red;
            m_colorMask[1] = green;
            m_colorMask[2] = blue;
            m_colorMask[3] = alpha;
            glColorMask(red, green, blue, alpha);
        }
    }

    inline void CullFace(GLenum mode)
    {
        if (m_cullFaceDirty || m_cullFace != mode) {
            m_cullFaceDirty = 0;
            m_cullFace = mode;
            glCullFace(mode);
        }
    }

    inline void DepthFunc(GLenum func)
    {
        if (m_depthFuncDirty || m_depthFunc != func) {
            m_depthFuncDirty = 0;
            m_depthFunc = func;
            glDepthFunc(func);
        }
    }

    /** glDepthMask(`enable` != 0); `enable` is read once for the cache and again for the call. */
    template <class T>
    inline void DepthMask(const T& enable)
    {
        bool flag = (enable != 0);
        if (m_depthMaskDirty || m_depthMask != flag) {
            m_depthMaskDirty = 0;
            m_depthMask = flag;
            glDepthMask(enable != 0);
        }
    }

    inline void DepthRange(const GLfloat& nearValue, const GLfloat& farValue)
    {
        GLfloat nearCopy = nearValue;
        GLfloat farCopy = farValue;
        if (m_depthRangeDirty || m_depthNear != nearCopy || m_depthFar != farCopy) {
            m_depthRangeDirty = 0;
            m_depthNear = nearCopy;
            m_depthFar = farCopy;
            glDepthRangef(nearValue, farValue);
        }
    }

    inline void FrontFace(GLenum mode)
    {
        if (m_frontFaceDirty || m_frontFace != mode) {
            m_frontFaceDirty = 0;
            m_frontFace = mode;
            glFrontFace(mode);
        }
    }

    inline void PolygonOffset(const GLfloat& factor, const GLfloat& units)
    {
        GLfloat factorCopy = factor;
        GLfloat unitsCopy = units;
        if (m_polygonOffsetDirty || m_polygonOffsetFactor != factorCopy || m_polygonOffsetUnits != unitsCopy) {
            m_polygonOffsetDirty = 0;
            m_polygonOffsetFactor = factorCopy;
            m_polygonOffsetUnits = unitsCopy;
            glPolygonOffset(factor, units);
        }
    }

    inline void Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
    {
        if (m_scissorDirty || m_scissor[0] != x || m_scissor[1] != y || m_scissor[2] != width ||
            m_scissor[3] != height) {
            m_scissorDirty = 0;
            m_scissor[0] = x;
            m_scissor[1] = y;
            m_scissor[2] = width;
            m_scissor[3] = height;
            glScissor(x, y, width, height);
        }
    }

    inline void StencilMask(GLuint mask)
    {
        if (m_stencilMaskDirty || m_stencilMask != mask) {
            m_stencilMaskDirty = 0;
            m_stencilMask = mask;
            glStencilMask(mask);
        }
    }

    inline void Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
    {
        if (m_viewportDirty || m_viewport[0] != x || m_viewport[1] != y || m_viewport[2] != width ||
            m_viewport[3] != height) {
            m_viewportDirty = 0;
            m_viewport[0] = x;
            m_viewport[1] = y;
            m_viewport[2] = width;
            m_viewport[3] = height;
            glViewport(x, y, width, height);
        }
    }

    unsigned int m_unknown00[2];            // 0x00 (0x364 in CGraphics_OGLES) never touched in this unit
    unsigned char m_activeTextureDirty;     // 0x08 (offsets below are relative to 0x36c)
    GLenum m_activeTexture;                 // 0x04
    unsigned char m_blendDirty;             // 0x08 GL_BLEND
    unsigned char m_blendEnabled;           // 0x09
    unsigned char m_blendFuncDirty;         // 0x0a
    GLenum m_blendSource;                   // 0x0c
    GLenum m_blendDestination;              // 0x10
    unsigned char m_bindTextureDirty;       // 0x14
    GLenum m_textureTargets[16];            // 0x18 per texture unit
    GLuint m_textures[16];                  // 0x58 per texture unit
    unsigned char m_clearColorDirty;        // 0x98
    GLfloat m_clearColor[4];                // 0x9c
    unsigned char m_clearDepthDirty;        // 0xac
    GLfloat m_clearDepth;                   // 0xb0
    unsigned char m_clearStencilDirty;      // 0xb4 (not set by Invalidate in the original either)
    GLint m_clearStencil;                   // 0xb8
    unsigned char m_colorMaskDirty;         // 0xbc
    GLboolean m_colorMask[4];               // 0xbd
    unsigned char m_cullFaceDirty;          // 0xc1
    GLenum m_cullFace;                      // 0xc4
    unsigned char m_cullFaceEnableDirty;    // 0xc8 GL_CULL_FACE
    unsigned char m_cullFaceEnabled;        // 0xc9
    unsigned char m_depthFuncDirty;         // 0xca
    GLenum m_depthFunc;                     // 0xcc
    unsigned char m_depthRangeDirty;        // 0xd0
    GLfloat m_depthNear;                    // 0xd4
    GLfloat m_depthFar;                     // 0xd8
    unsigned char m_depthMaskDirty;         // 0xdc
    GLboolean m_depthMask;                  // 0xdd
    unsigned char m_depthTestDirty;         // 0xde GL_DEPTH_TEST
    unsigned char m_depthTestEnabled;       // 0xdf
    unsigned char m_frontFaceDirty;         // 0xe0
    GLenum m_frontFace;                     // 0xe4
    unsigned char m_polygonOffsetDirty;     // 0xe8
    GLfloat m_polygonOffsetFactor;          // 0xec
    GLfloat m_polygonOffsetUnits;           // 0xf0
    unsigned char m_scissorDirty;           // 0xf4
    GLint m_scissor[4];                     // 0xf8
    unsigned char m_scissorTestDirty;       // 0x108 GL_SCISSOR_TEST
    unsigned char m_scissorTestEnabled;     // 0x109
    unsigned char m_stencilMaskDirty;       // 0x10a
    GLuint m_stencilMask;                   // 0x10c
    unsigned char m_viewportDirty;          // 0x110
    GLint m_viewport[4];                    // 0x114
};

class CGraphics_OGLES : public CGraphics {
public:
    /** Fixed function client arrays (m_arrayRequested / m_arrayEnabled index). */
    enum ClientArray {
        ClientArray_Color = 0,
        ClientArray_Normal = 1,
        ClientArray_Vertex = 2,
        ClientArray_TexCoord0 = 3,      // ClientArray_TexCoord0 + n: texture unit n
        ClientArray_Count = 5
    };

    /** Vertex streams of the device (only stream 0 exists). */
    enum { VertexStreamCount = 1 };

    /** Draw instruction recorded by Draw(Primitive, first, count). */
    struct DrawArraysInstruction {
        CDisplayProgram::Instruction::Header m_header;  // 0x00 opcode VtxStreamTris
        ICBuffer::Stream* m_vertexStream;               // 0x10
        unsigned int m_reserved;                        // 0x14
        unsigned int m_count;                           // 0x18
        unsigned int m_first;                           // 0x1c
    };

    /** Draw instruction recorded by Draw(Primitive, indices, first, count). */
    struct DrawElementsInstruction {
        CDisplayProgram::Instruction::Header m_header;  // 0x00 opcode IdxVtxStreamTriStrip
        ICBuffer::Stream* m_vertexStream;               // 0x10
        unsigned int m_reserved;                        // 0x14
        unsigned int m_vertexCount;                     // 0x18
        ICBuffer::Stream* m_indexStream;                // 0x1c
        unsigned int m_indexType;                       // 0x20
        unsigned int m_count;                           // 0x24
        unsigned int m_first;                           // 0x28
    };

    /** A vertex attribute layout the fixed function pipeline understands. */
    struct IntrinsicAttribute {
        unsigned int m_id;              // 0x00 CVertex::Attribute::Id
        unsigned int m_type;            // 0x04 ComponentType
        unsigned int m_count;           // 0x08 components
        components::CStrWChar m_name;   // 0x0c

        IntrinsicAttribute()
        {
            m_id = 0xffffffff;
        }
    };

    /** State the device keeps while a display program runs, restored afterwards. */
    class DisplayProgramRecoveryCache {
    public:
        /** Take over the pending state slots of the device (they are cleared). */
        void Config(CGraphics_OGLES* graphics);
        /** Send the remembered state to the device again. */
        void Flush(CGraphics_OGLES* graphics);

        unsigned char** m_viewportLevel;            // 0x00
        unsigned char** m_shadersLevel;             // 0x04
        unsigned char** m_texturesLevel;            // 0x08
        unsigned char** m_rasterizerLevels[5];      // 0x0c per ICRasterizerState::State
        ICBuffer::Stream* m_vertexStream;           // 0x20
        unsigned char m_vertexStreamEnabled;        // 0x24
        unsigned char m_vertexStreamEnable;         // 0x25
    };

    explicit CGraphics_OGLES(EventHandler eventHandler);

    virtual ~CGraphics_OGLES()
    {
    }

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const
    {
        return ICGraphicsAbstract::Abstraction_1;
    }

    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    virtual ICDisplayProgram* Begin(ICDisplayProgram::Mode& mode, unsigned char keepState, unsigned char runOnEnd);
    virtual void Begin(ICDisplayProgram& program, ICDisplayProgram::Mode& mode, unsigned char keepState,
                       unsigned char runOnEnd);
    virtual void End(ICDisplayProgram& program);
    virtual void Run(ICDisplayProgram& program);

    virtual ICRasterizerState* GetRasterizerState() const
    {
        return m_rasterizerState;
    }

    virtual void SetTargetSurface(ICRenderSurface& surface);
    virtual ICRenderSurface* GetTargetSurface() const;
    virtual void ClearBuffers(unsigned int flags);
    virtual void SwapBuffers();
    virtual void EnableClearScissor(unsigned char enable);
    virtual void SetClearScissor(short x, short y, short width, short height);
    virtual void SetClearColor(float red, float green, float blue, float alpha);
    virtual void SetClearColor(const components::Color_RGBA& color);
    virtual void SetClearColorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
    virtual void SetClearDepth(float depth);
    virtual void SetClearStencil(int stencil);
    virtual void SetViewport(short x, short y, short width, short height);
    virtual void SetShaderProgram(ICShaderProgram& program);
    virtual ICShaderProgram* GetShaderProgram();
    virtual void SetTextureSamplerHint(unsigned int unit, TextureSampleHint hint);
    virtual void SetTexture2d(unsigned int unit, ICRenderSurface& surface);
    virtual void SetVertexStream(unsigned int index, ICBuffer::Stream& stream);
    virtual void EnableVertexStream(unsigned int index, unsigned char enable);
    virtual void Draw(Primitive primitive);
    virtual void Draw(Primitive primitive, unsigned int first, unsigned int count);
    virtual void Draw(Primitive primitive, ICBuffer::Stream& indices);
    virtual void Draw(Primitive primitive, ICBuffer::Stream& indices, unsigned int first, unsigned int count);

    virtual const Description& GetDescription() const
    {
        return m_description;
    }

    virtual const components::CStrWChar& GetDeviceVersion() const
    {
        return m_versionString;
    }

    virtual void GetDeviceVersion(unsigned int& major, unsigned int& minor) const
    {
        major = m_versionMajor;
        minor = m_versionMinor;
    }

    virtual void Sync();
    /** Load the intrinsic shader programs (`reload`: bypass the resource manager). Returns 1. */
    virtual unsigned char InitializeIntrinsicHardwareShaderManagement(unsigned char reload);
    virtual void DestroyIntrinsicSoftwareShaderManagement();
    virtual void SetCallBackInstructionExecutionShaders();
    /** Push the cached device state to OpenGL again. */
    virtual void SyncCore();
    /** Platform part of Destroy(): release the context. */
    virtual void ReleaseDevice() = 0;

    /** Bind texture `level` of `surface` to `unit`. */
    void SetTexture2d(unsigned int unit, ICRenderSurface& surface, unsigned int level);
    /** The parameters of `program` changed: refresh its "ShaderParams" instruction. */
    void NotifyOfChangeInShaderProgramTableData(ICShaderProgram& program);

    /** Mark `array` as used by the current draw and enable it. */
    inline void EnableClientArray(unsigned int array)
    {
        if (array == ClientArray_Color) {
            m_arrayRequested[ClientArray_Color] = 1;
            if (!m_arrayEnabled[ClientArray_Color]) {
                m_arrayEnabled[ClientArray_Color] = 1;
                if (m_fixedFunction) {
                    glEnableClientState(GL_COLOR_ARRAY);
                }
            }
        } else if (array == ClientArray_TexCoord0) {
            m_arrayRequested[array] = 1;
            if (!m_arrayEnabled[array]) {
                m_arrayEnabled[array] = 1;
                if (m_fixedFunction) {
                    glActiveTexture(GL_TEXTURE0 + array - ClientArray_TexCoord0);
                    if (m_fixedFunction) {
                        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    }
                }
            }
        } else if (array == ClientArray_Vertex) {
            m_arrayRequested[ClientArray_Vertex] = 1;
            if (!m_arrayEnabled[ClientArray_Vertex]) {
                m_arrayEnabled[ClientArray_Vertex] = 1;
                if (m_fixedFunction) {
                    glEnableClientState(GL_VERTEX_ARRAY);
                }
            }
        }
    }

    /** Disable `array` (no longer used). */
    inline void DisableClientArray(unsigned int array)
    {
        if (array == ClientArray_Color) {
            m_arrayEnabled[ClientArray_Color] = 0;
            glDisableClientState(GL_COLOR_ARRAY);
        } else if (array - ClientArray_TexCoord0 < 4) {
            m_arrayEnabled[array] = 0;
            if (m_fixedFunction) {
                glActiveTexture(GL_TEXTURE0 + array - ClientArray_TexCoord0);
                if (m_fixedFunction) {
                    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                }
            }
        } else if (array == ClientArray_Vertex) {
            m_arrayEnabled[ClientArray_Vertex] = 0;
            glDisableClientState(GL_VERTEX_ARRAY);
        }
    }

    /** Reset the members Destroy() and the constructor share. */
    inline void ResetFixedFunctionState()
    {
        m_alphaTest = 0;
        m_textureMatrixOverride = 0;
        m_texCoordType = ComponentType_UnsignedShort;
        m_textureMatrixType = 0;
        m_hasTextureNpot = 0;
        m_hasLimitedTextureNpot = 0;
        m_hasDiscardFramebuffer = 0;
        m_npotTextures = 0;
        m_npotRenderTargets = 0;
    }

    ICShaderProgram::ParameterTable m_shaderParameters;     // 0x260 parameters of the intrinsic programs
    IntrinsicAttribute* m_intrinsicAttributes;              // 0x2d8 the 43 fixed function layouts
    ICBuffer::Stream* m_vertexStreams[VertexStreamCount];   // 0x2dc SetVertexStream()
    unsigned char m_vertexStreamDirty[VertexStreamCount];   // 0x2e0 attributes must be set up again
    unsigned char m_vertexStreamEnabled[VertexStreamCount]; // 0x2e1 client arrays are enabled
    unsigned char m_vertexStreamEnable[VertexStreamCount];  // 0x2e2 EnableVertexStream() request
    unsigned char m_vertexStreamFlag;                       // 0x2e3 (set by Initialize)
    core::CVector<unsigned int> m_streamArrays[VertexStreamCount];  // 0x2e4 ClientArray values a stream feeds
    int* m_arrayRequested;                                  // 0x2fc per ClientArray: used by the draw
    unsigned char* m_arrayEnabled;                          // 0x300 per ClientArray: enabled in OpenGL
    unsigned int m_state304;                                // 0x304 (only cleared)
    Description m_description;                              // 0x308
    components::CStrWChar m_versionString;                  // 0x33c GL_VERSION
    components::CStrWChar m_rendererString;                 // 0x34c GL_RENDERER
    unsigned int m_versionMajor;                            // 0x35c
    unsigned int m_versionMinor;                            // 0x360
    CGraphics_OGLES_GLState m_glState;                      // 0x364 cached OpenGL state
    unsigned int m_state490[3];                             // 0x490 (unused)
    CProxy_OGLES* m_proxy;                                  // 0x49c platform context glue
    unsigned char m_state4a0[5];                            // 0x4a0 (unused)
    unsigned char m_hasTextureNpot;                         // 0x4a5 OES_texture_npot
    unsigned char m_hasLimitedTextureNpot;                  // 0x4a6 APPLE_texture_2D_limited_npot
    unsigned char m_hasDiscardFramebuffer;                  // 0x4a7 GL_EXT_discard_framebuffer
    unsigned char m_npotTextures;                           // 0x4a8
    unsigned char m_npotRenderTargets;                      // 0x4a9
    unsigned char m_state4aa[2];                            // 0x4aa (unused)
    int m_maxTextureUnits;                                  // 0x4ac
    int m_maxTextureSize;                                   // 0x4b0
    int m_maxTextureWidth;                                  // 0x4b4
    int m_maxTextureHeight;                                 // 0x4b8
    int m_maxRenderTargetSize;                              // 0x4bc
    unsigned int m_multisampleSamples;                      // 0x4c0 init parameter 6
    unsigned char m_alphaTest;                              // 0x4c4 GL_ALPHA_TEST with alpha > 0.1
    unsigned int m_textureMatrixOverride;                   // 0x4c8 the texture matrix is set elsewhere
    unsigned int m_texCoordType;                            // 0x4cc ComponentType of the texture coordinates
    unsigned int m_textureMatrixType;                       // 0x4d0 m_texCoordType the texture matrix is set for
    components::CArrayInputStream m_shaderSource;           // 0x4d4 built-in program source being loaded
    components::CArrayInputStream m_shaderBinary;           // 0x4f4
    components::CIdToObjectRouter* m_resourceRouter;        // 0x514 resolves the built-in program names
    unsigned int m_fixedFunction;                           // 0x518 OpenGL ES 1 calls are made
    unsigned char* m_drawInstruction;                       // 0x51c draw instruction being recorded
    DisplayProgramRecoveryCache m_recoveryCache;            // 0x520
};

} } } }

#endif
