// CDisplayProgram.h - device independent display program: instructions are recorded into a tree
// of levels (one level per instruction kind, the draw instructions at the last level) so that
// equal state instructions are shared, and replayed by Run() through per-level callbacks.
// (first drafted by b002 as PROVISIONAL; completed by b037x, the owner)
//
// Instruction layout (Instruction::Header, followed by the data words):
//   +0x00 Header* m_child    first instruction of the next level (NULL: the one right after this)
//   +0x04 Header* m_sibling  next alternative at the same level
//   +0x08 u16     m_size     bytes including the header
//   +0x0a u16     m_level    level the instruction was recorded at
//   +0x0c u32     m_opcode   Instruction::Opcode (missing in the 12-byte empty level nodes)
#ifndef CDISPLAYPROGRAM_H
#define CDISPLAYPROGRAM_H

#include <stddef.h>
#include "ICDisplayProgram.h"
#include "ICBuffer.h"
#include "CStack.h"
#include "CVector.h"
#include "CVertex.h"
#include "ICIndexBuffer.h"
#include "ICVertexBuffer.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICIndexBuffer;
class ICVertexBuffer;

/** Mode configure callbacks (CDisplayProgram.cpp). */
void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_BlendBiased(void* context, ICDisplayProgram& program);
void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_BlendBiasedIgnoreTargetCommands(void* context,
                                                                                      ICDisplayProgram& program);
void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFront(void* context, ICDisplayProgram& program);
void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFrontIgnoreTargetCommands(void* context,
                                                                                             ICDisplayProgram& program);
void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFrontIgnoreTargetColorMaskBackfaceCullingAndDepthCommands(
    void* context, ICDisplayProgram& program);

class CDisplayProgram : public ICDisplayProgram {
public:
    class Instruction {
    public:
        class Opcode {
        public:
            /**
             * Built-in opcodes. Bits: 0x0000ff index, 0x010000 intrinsic, 0x020000 indexed,
             * 0x040000 reads a vertex stream, 0x080000 batch (streams are allocated by the
             * program and consecutive draws are merged), 0xff000000 primitive.
             */
            enum Intrinsic {
                Undefined_Value = 0,
                BackfaceCulling = 0x10000,
                Blending = 0x10001,
                Callback = 0x10002,
                ClearBuffers = 0x10003,
                ColorMask = 0x10004,
                Depth = 0x10005,
                IdxVtxStreamLines = 0x1070006,
                IdxVtxStreamLinesBatch = 0x10f0007,
                IdxVtxStreamLineStrip = 0x2070008,
                IdxVtxStreamLineStripBatch = 0x20f0009,
                IdxVtxStreamTris = 0x307000a,
                IdxVtxStreamTrisBatch = 0x30f000b,
                IdxVtxStreamTriStrip = 0x407000c,
                IdxVtxStreamTriStripBatch = 0x40f000d,
                Layer = 0x1000e,
                RasterizerState = 0x1000f,
                Scissor = 0x10010,
                Shaders = 0x10011,
                ShaderParams = 0x10012,
                Stencil = 0x10013,
                Target = 0x10014,
                Textures = 0x10015,
                Undefined = 0x10016,
                Viewport = 0x10017,
                VtxStreamLines = 0x1050018,
                VtxStreamLinesBatch = 0x10d0019,
                VtxStreamLineStrip = 0x205001a,
                VtxStreamLineStripBatch = 0x20d001b,
                VtxStreamTris = 0x305001c,
                VtxStreamTrisBatch = 0x30d001d,
                VtxStreamTriStrip = 0x405001e,
                VtxStreamTriStripBatch = 0x40d001f
            };

            /** Opcode flag bits. */
            enum Flag {
                Flag_Indexed = 0x20000,
                Flag_Batch = 0x80000
            };

            /** Opcode of an instruction name ("Blending", ..., case-insensitive); 0 if unknown. */
            static unsigned int GetIntrinsicOpcode(const wchar_t* name);
        };

        /** Bytes of a Header without the opcode (the empty level nodes). */
        enum { LevelNodeSize = 0xc };

        /** Common part of every recorded instruction. */
        struct Header {
            Header* m_child;            // 0x00 next level (NULL: right after this instruction)
            Header* m_sibling;          // 0x04 next alternative at this level
            unsigned short m_size;      // 0x08 bytes including the header
            unsigned short m_level;     // 0x0a
            unsigned int m_opcode;      // 0x0c
        };

        /** Header of the empty level nodes (no opcode). */
        struct LevelNode {
            Header* m_child;            // 0x00
            Header* m_sibling;          // 0x04
            unsigned short m_size;      // 0x08 LevelNodeSize
            unsigned short m_level;     // 0x0a
        };

        /** The Layer instruction a program owns (SetLayer / GetLayer). */
        struct LayerInstruction {
            Header m_header;            // 0x00 opcode Opcode::Layer
            int m_layer;                // 0x10
        };

        /** Batch draw instruction (opcodes with Flag_Batch). */
        struct Draw {
            Header m_header;                    // 0x00
            ICBuffer::Stream* m_vertexStream;   // 0x10 NULL: allocate from the program's vertex buffers
            unsigned int m_vertexFormat;        // 0x14 key of the vertex layout
            unsigned int m_vertexCount;         // 0x18
            ICBuffer::Stream* m_indexStream;    // 0x1c NULL: allocate from the program's index buffers
            unsigned int m_indexType;           // 0x20 ComponentType of the indices
            unsigned int m_indexCount;          // 0x24
        };
    };

    /**
     * Growable pool of T: a list of fixed size vectors; Add() moves on to the next chunk (and
     * creates it when needed) instead of reallocating, so added elements never move.
     */
    template <class T>
    class Pool {
    public:
        struct Chunk {
            core::CVector<T> m_elements;    // 0x00
            Chunk* m_next;                  // 0x18

            Chunk(int capacity) : m_elements(capacity, 0)
            {
                m_next = NULL;
            }
        };

        Pool(int capacity)
        {
            m_current = new Chunk(capacity);
            m_head = m_current;
        }

        ~Pool()
        {
            Chunk* chunk = m_head;
            while (chunk != NULL) {
                Chunk* next = chunk->m_next;
                delete chunk;
                chunk = next;
            }
            m_current = NULL;
            m_head = NULL;
        }

        void Add(const T& element)
        {
            if (m_current->m_elements.m_size < m_current->m_elements.m_capacity) {
                m_current->m_elements.Add(element);
            } else {
                if (m_current->m_next == NULL) {
                    m_current->m_next = new Chunk(m_current->m_elements.m_capacity);
                }
                m_current = m_current->m_next;
                m_current->m_elements.Add(element);
            }
        }

        /** The element added last. */
        T& GetLast()
        {
            return m_current->m_elements.m_data[m_current->m_elements.m_size - 1];
        }

        Chunk* m_current;   // 0x00 chunk being filled
        Chunk* m_head;      // 0x04 first chunk
    };

    /** A shared buffer the batch streams are carved from. */
    template <class T>
    struct BufferRef {
        unsigned int m_key;         // 0x00 vertex format / index type the buffer holds
        unsigned int m_used;        // 0x04 elements handed out
        unsigned int m_capacity;    // 0x08 elements in the buffer
        T m_buffer;                 // 0x0c

        // user-declared: CVector copies the references without zero-filling the slot first
        BufferRef() {}
    };

    /** A block of instruction memory (words). */
    struct NodeChunk {
        unsigned int* m_data;       // 0x00
        unsigned int m_used;        // 0x04 words handed out
        unsigned int m_capacity;    // 0x08 words

        // user-declared: CVector copies the chunks without zero-filling the slot first
        NodeChunk() {}
    };

    CDisplayProgram();
    virtual ~CDisplayProgram();

    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    virtual void SetLayer(int layer);
    virtual int GetLayer();
    virtual void SetCallBackInstructionExecution(unsigned int level, void* context,
                                                 void (*callback)(void* context, unsigned char* instruction));
    virtual void Run();
    virtual void Add(unsigned char* instruction);
    /** Default sizes of the instruction memory and of the shared buffers. */
    virtual void ConfigDefaultAllocationInfo() = 0;

    /** Forget all recorded instructions; `releaseMemory` also frees the memory and buffers. */
    void Reset(unsigned char releaseMemory);
    /** (Re)allocate the per-level tables for `levelCount` levels. */
    void InitializeDepthAndAllocLevel(unsigned int levelCount);

    unsigned int m_addLevel;                        // 0x08 level Add() is looking at
    unsigned int m_layerLevel;                      // 0x0c level of the Layer instructions
    unsigned char m_blendBiased;                    // 0x10 Mode::m_blendBiased
    unsigned char m_sortedByLayer;                  // 0x11 set by the recorder: use the *Sorted actions
    unsigned int m_targetAction;                    // 0x14 Mode::m_targetAction
    unsigned int m_stateActionSorted;               // 0x18
    unsigned int m_stateAction;                     // 0x1c
    unsigned int m_drawActionSorted;                // 0x20
    unsigned int m_drawAction;                      // 0x24
    unsigned char m_searchLastSibling;              // 0x28 Add(): look at the last sibling only
    unsigned char m_layerMatched;                   // 0x29 Add(): a Layer instruction was shared
    unsigned char m_mergeDraws;                     // 0x2a Add(): draw action of the current mode is 0
    unsigned int m_levelCount;                      // 0x2c
    unsigned char** m_levelInstructions;            // 0x30 pending instruction of each level
    Instruction::Header** m_levelNodes;             // 0x34 empty node of each level
    unsigned int m_opcodeLevelCount;                // 0x38 entries in m_opcodeLevels
    unsigned int* m_opcodeLevels;                   // 0x3c level of each opcode index (0x7fff: ignored)
    Instruction::Header* m_root;                    // 0x40 first instruction of level 0
    Instruction::Header** m_levelLastAdded;         // 0x44 last instruction recorded at each level
    unsigned int* m_levelEmpty;                     // 0x48 Run(): level reached through an empty node
    core::CStack<Instruction::Header*> m_runStack;  // 0x4c Run(): path to the current instruction
    void** m_levelCallbackContexts;                 // 0x58
    void (**m_levelCallbacks)(void* context, unsigned char* instruction);   // 0x5c
    Instruction::LayerInstruction* m_layerInstruction;  // 0x60 owned Layer instruction (SetLayer)
    unsigned int m_nodeChunkLength;                 // 0x64 words per NodeChunk
    unsigned int m_indexBufferLength;               // 0x68 indices per shared index buffer
    unsigned int m_vertexBufferLength;              // 0x6c vertices per shared vertex buffer
    unsigned int m_indexStreamLength;               // 0x70 indices per batch index stream
    unsigned int m_vertexStreamLength;              // 0x74 vertices per batch vertex stream
    unsigned int m_nodeChunkIndex;                  // 0x78 NodeChunk being filled in m_nodeChunk
    Pool<NodeChunk>::Chunk* m_nodeChunk;            // 0x7c
    Pool<NodeChunk> m_nodeChunks;                   // 0x80
    Pool<BufferRef<ICIndexBuffer*> > m_indexBuffers;    // 0x88
    Pool<BufferRef<ICVertexBuffer*> > m_vertexBuffers;  // 0x90
    Pool<ICBuffer::Stream> m_vertexStreams;         // 0x98
    Pool<ICBuffer::Stream> m_indexStreams;          // 0xa0
    short m_batchDepth;                             // 0xa8 depth counter of the recorded 3d batches
                                                    //      (CGraphics2d_OGLES), cleared by the device's Begin()
    unsigned int* m_levelIgnored;                   // 0xac level is skipped by Add()
    unsigned char** m_levelSavedInstructions;       // 0xb0 Add(): pending instructions of ignored levels
};

/** Vertex layouts of the batch draws (Instruction::Draw::m_vertexFormat). */
enum VertexFormat {
    VertexFormat_Vtx2d = 0x13b44,           // 2 short coordinates
    VertexFormat_Vtx3d = 0x13b54,           // 3 short coordinates
    VertexFormat_Vtx2d_t = 0x213b4184,      // 2 short coordinates + 2 short texture coordinates
    VertexFormat_Vtx3d_t = 0x213b5184       // 3 short coordinates + 2 short texture coordinates
};

/** Attribute lists of the batch vertex layouts (vertex buffers created by CDisplayProgram::Add). */
static const CVertex::Attribute::Decl Vtx2d_AttrDecl[] = {
    { CVertex::Attribute::Id::Position, ComponentType_Short, 0, 2, 4 },
    { CVertex::Attribute::Id::End, 0, 0, 0, 0 }
};

static const CVertex::Attribute::Decl Vtx2d_t_AttrDecl[] = {
    { CVertex::Attribute::Id::Position, ComponentType_Short, 0, 2, 8 },
    { CVertex::Attribute::Id::TexCoord, ComponentType_Short, 4, 2, 8 },
    { CVertex::Attribute::Id::End, 0, 0, 0, 0 }
};

static const CVertex::Attribute::Decl Vtx3d_AttrDecl[] = {
    { CVertex::Attribute::Id::Position, ComponentType_Short, 0, 3, 8 },
    { CVertex::Attribute::Id::End, 0, 0, 0, 0 }
};

static const CVertex::Attribute::Decl Vtx3d_t_AttrDecl[] = {
    { CVertex::Attribute::Id::Position, ComponentType_Short, 0, 3, 12 },
    { CVertex::Attribute::Id::TexCoord, ComponentType_Short, 8, 2, 12 },
    { CVertex::Attribute::Id::End, 0, 0, 0, 0 }
};

} } } }

#endif
