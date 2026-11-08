// CDisplayProgram.cpp - recording modes and the device independent display program.
#include "CDisplayProgram.h"
#include "CStdUtil_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// entries of the opcode -> level table, and the level value of opcodes the mode ignores
static const unsigned int DisplayProgram_OpcodeLevelCount = 0x20;
static const unsigned int DisplayProgram_IgnoredLevel = 0x7fff;

/** Opcode table index of an intrinsic opcode. */
#define OPCODE_INDEX(opcode) ((opcode) & 0xff)

void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_BlendBiased(void* context, ICDisplayProgram& displayProgram)
{
    // one level per state kind; blending is high up so that blended draws stay in order
    static const unsigned int instrLUTIndex[] = {
        CDisplayProgram::Instruction::Opcode::Target,
        CDisplayProgram::Instruction::Opcode::ClearBuffers,
        CDisplayProgram::Instruction::Opcode::Viewport,
        CDisplayProgram::Instruction::Opcode::Layer,
        CDisplayProgram::Instruction::Opcode::Blending,
        CDisplayProgram::Instruction::Opcode::Shaders,
        CDisplayProgram::Instruction::Opcode::Textures,
        CDisplayProgram::Instruction::Opcode::ColorMask,
        CDisplayProgram::Instruction::Opcode::BackfaceCulling,
        CDisplayProgram::Instruction::Opcode::Depth,
        CDisplayProgram::Instruction::Opcode::Scissor,
        CDisplayProgram::Instruction::Opcode::ShaderParams
    };
    ICDisplayProgram::Mode* mode = (ICDisplayProgram::Mode*)context;
    CDisplayProgram& program = static_cast<CDisplayProgram&>(displayProgram);

    if (program.m_opcodeLevelCount != DisplayProgram_OpcodeLevelCount) {
        if (program.m_opcodeLevels != NULL) {
            delete[] program.m_opcodeLevels;
            program.m_opcodeLevels = NULL;
        }
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    } else if (program.m_opcodeLevels == NULL) {
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    }
    program.m_opcodeLevelCount = DisplayProgram_OpcodeLevelCount;

    program.InitializeDepthAndAllocLevel(13);
    for (unsigned int level = 0; level < 12; level++) {
        program.m_opcodeLevels[OPCODE_INDEX(instrLUTIndex[level])] = level;
    }

    // every draw opcode goes to the last level
    program.m_opcodeLevels[2] = program.m_levelCount - 1;
    program.m_opcodeLevels[6] = program.m_levelCount - 1;
    program.m_opcodeLevels[7] = program.m_levelCount - 1;
    program.m_opcodeLevels[8] = program.m_levelCount - 1;
    program.m_opcodeLevels[9] = program.m_levelCount - 1;
    program.m_opcodeLevels[10] = program.m_levelCount - 1;
    program.m_opcodeLevels[11] = program.m_levelCount - 1;
    program.m_opcodeLevels[12] = program.m_levelCount - 1;
    program.m_opcodeLevels[13] = program.m_levelCount - 1;
    program.m_opcodeLevels[24] = program.m_levelCount - 1;
    program.m_opcodeLevels[25] = program.m_levelCount - 1;
    program.m_opcodeLevels[26] = program.m_levelCount - 1;
    program.m_opcodeLevels[27] = program.m_levelCount - 1;
    program.m_opcodeLevels[28] = program.m_levelCount - 1;
    program.m_opcodeLevels[29] = program.m_levelCount - 1;
    program.m_opcodeLevels[30] = program.m_levelCount - 1;
    program.m_opcodeLevels[31] = program.m_levelCount - 1;

    program.m_opcodeLevels[15] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[19] = DisplayProgram_IgnoredLevel;

    program.m_levelInstructions[program.m_opcodeLevels[14]] = (unsigned char*)program.m_layerInstruction;
    program.m_layerLevel = program.m_opcodeLevels[14];
    program.m_blendBiased = mode->m_blendBiased;
    program.m_targetAction = mode->m_targetAction;
    program.m_stateActionSorted = mode->m_stateActionSorted;
    program.m_stateAction = mode->m_stateAction;
    program.m_drawActionSorted = mode->m_drawActionSorted;
    program.m_drawAction = mode->m_drawAction;
}

void CDisplayProgram::InitializeDepthAndAllocLevel(unsigned int levelCount)
{
    if (m_levelCount != levelCount) {
        if (m_levelSavedInstructions != NULL) {
            delete[] m_levelSavedInstructions;
            m_levelSavedInstructions = NULL;
        }
        if (m_levelIgnored != NULL) {
            delete[] m_levelIgnored;
            m_levelIgnored = NULL;
        }
        if (m_levelInstructions != NULL) {
            delete[] m_levelInstructions;
            m_levelInstructions = NULL;
        }
        if (m_levelNodes != NULL) {
            delete[] m_levelNodes;
            m_levelNodes = NULL;
        }
        if (m_levelLastAdded != NULL) {
            delete[] m_levelLastAdded;
            m_levelLastAdded = NULL;
        }
        if (m_levelEmpty != NULL) {
            delete[] m_levelEmpty;
            m_levelEmpty = NULL;
        }
        if (m_levelCallbackContexts != NULL) {
            delete[] m_levelCallbackContexts;
            m_levelCallbackContexts = NULL;
        }
        if (m_levelCallbacks != NULL) {
            delete[] m_levelCallbacks;
            m_levelCallbacks = NULL;
        }
    }

    if (m_levelInstructions == NULL) {
        m_levelInstructions = new unsigned char*[levelCount];
    }
    if (m_levelNodes == NULL) {
        m_levelNodes = new Instruction::Header*[levelCount];
        for (unsigned int level = 0; level < levelCount; level++) {
            m_levelNodes[level] = (Instruction::Header*)new Instruction::LevelNode;
            Instruction::Header* node = m_levelNodes[level];
            node->m_child = NULL;
            node->m_sibling = NULL;
            node->m_size = Instruction::LevelNodeSize;
            node->m_level = (unsigned char)level;
        }
    }
    if (m_levelLastAdded == NULL) {
        m_levelLastAdded = new Instruction::Header*[levelCount];
    }
    if (m_levelEmpty == NULL) {
        m_levelEmpty = new unsigned int[levelCount];
    }
    if (m_levelCallbackContexts == NULL) {
        m_levelCallbackContexts = new void*[levelCount];
    }
    if (m_levelCallbacks == NULL) {
        m_levelCallbacks = new (void (*[levelCount])(void* context, unsigned char* instruction));
    }
    m_levelCount = levelCount;

    if (m_levelIgnored == NULL) {
        m_levelIgnored = new unsigned int[levelCount];
        for (unsigned int level = 0; level < m_levelCount; level++) {
            m_levelIgnored[level] = 0;
        }
    }
    if (m_levelSavedInstructions == NULL) {
        m_levelSavedInstructions = new unsigned char*[m_levelCount];
    }
}

void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_BlendBiasedIgnoreTargetCommands(void* context,
                                                                                      ICDisplayProgram& displayProgram)
{
    // as BlendBiased without the target levels
    static const unsigned int instrLUTIndex[] = {
        CDisplayProgram::Instruction::Opcode::Layer,
        CDisplayProgram::Instruction::Opcode::Blending,
        CDisplayProgram::Instruction::Opcode::Shaders,
        CDisplayProgram::Instruction::Opcode::Textures,
        CDisplayProgram::Instruction::Opcode::ColorMask,
        CDisplayProgram::Instruction::Opcode::BackfaceCulling,
        CDisplayProgram::Instruction::Opcode::Depth,
        CDisplayProgram::Instruction::Opcode::Scissor,
        CDisplayProgram::Instruction::Opcode::ShaderParams
    };
    ICDisplayProgram::Mode* mode = (ICDisplayProgram::Mode*)context;
    CDisplayProgram& program = static_cast<CDisplayProgram&>(displayProgram);

    if (program.m_opcodeLevelCount != DisplayProgram_OpcodeLevelCount) {
        if (program.m_opcodeLevels != NULL) {
            delete[] program.m_opcodeLevels;
            program.m_opcodeLevels = NULL;
        }
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    } else if (program.m_opcodeLevels == NULL) {
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    }
    program.m_opcodeLevelCount = DisplayProgram_OpcodeLevelCount;

    program.InitializeDepthAndAllocLevel(10);
    for (unsigned int level = 0; level < 9; level++) {
        program.m_opcodeLevels[OPCODE_INDEX(instrLUTIndex[level])] = level;
    }

    program.m_opcodeLevels[2] = program.m_levelCount - 1;
    program.m_opcodeLevels[6] = program.m_levelCount - 1;
    program.m_opcodeLevels[7] = program.m_levelCount - 1;
    program.m_opcodeLevels[8] = program.m_levelCount - 1;
    program.m_opcodeLevels[9] = program.m_levelCount - 1;
    program.m_opcodeLevels[10] = program.m_levelCount - 1;
    program.m_opcodeLevels[11] = program.m_levelCount - 1;
    program.m_opcodeLevels[12] = program.m_levelCount - 1;
    program.m_opcodeLevels[13] = program.m_levelCount - 1;
    program.m_opcodeLevels[24] = program.m_levelCount - 1;
    program.m_opcodeLevels[25] = program.m_levelCount - 1;
    program.m_opcodeLevels[26] = program.m_levelCount - 1;
    program.m_opcodeLevels[27] = program.m_levelCount - 1;
    program.m_opcodeLevels[28] = program.m_levelCount - 1;
    program.m_opcodeLevels[29] = program.m_levelCount - 1;
    program.m_opcodeLevels[30] = program.m_levelCount - 1;
    program.m_opcodeLevels[31] = program.m_levelCount - 1;

    program.m_opcodeLevels[20] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[3] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[23] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[15] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[19] = DisplayProgram_IgnoredLevel;

    program.m_levelInstructions[program.m_opcodeLevels[14]] = (unsigned char*)program.m_layerInstruction;
    program.m_layerLevel = program.m_opcodeLevels[14];
    program.m_blendBiased = mode->m_blendBiased;
    program.m_targetAction = mode->m_targetAction;
    program.m_stateActionSorted = mode->m_stateActionSorted;
    program.m_stateAction = mode->m_stateAction;
    program.m_drawActionSorted = mode->m_drawActionSorted;
    program.m_drawAction = mode->m_drawAction;
}

void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFront(void* context,
                                                                         ICDisplayProgram& displayProgram)
{
    // blending sorted just above the draws: everything is drawn back to front
    static const unsigned int instrLUTIndex[] = {
        CDisplayProgram::Instruction::Opcode::Target,
        CDisplayProgram::Instruction::Opcode::ClearBuffers,
        CDisplayProgram::Instruction::Opcode::Viewport,
        CDisplayProgram::Instruction::Opcode::Layer,
        CDisplayProgram::Instruction::Opcode::Shaders,
        CDisplayProgram::Instruction::Opcode::Textures,
        CDisplayProgram::Instruction::Opcode::ColorMask,
        CDisplayProgram::Instruction::Opcode::BackfaceCulling,
        CDisplayProgram::Instruction::Opcode::Depth,
        CDisplayProgram::Instruction::Opcode::Scissor,
        CDisplayProgram::Instruction::Opcode::Blending,
        CDisplayProgram::Instruction::Opcode::ShaderParams
    };
    ICDisplayProgram::Mode* mode = (ICDisplayProgram::Mode*)context;
    CDisplayProgram& program = static_cast<CDisplayProgram&>(displayProgram);

    if (program.m_opcodeLevelCount != DisplayProgram_OpcodeLevelCount) {
        if (program.m_opcodeLevels != NULL) {
            delete[] program.m_opcodeLevels;
            program.m_opcodeLevels = NULL;
        }
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    } else if (program.m_opcodeLevels == NULL) {
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    }
    program.m_opcodeLevelCount = DisplayProgram_OpcodeLevelCount;

    program.InitializeDepthAndAllocLevel(13);
    for (unsigned int level = 0; level < 12; level++) {
        program.m_opcodeLevels[OPCODE_INDEX(instrLUTIndex[level])] = level;
    }

    program.m_opcodeLevels[2] = program.m_levelCount - 1;
    program.m_opcodeLevels[6] = program.m_levelCount - 1;
    program.m_opcodeLevels[7] = program.m_levelCount - 1;
    program.m_opcodeLevels[8] = program.m_levelCount - 1;
    program.m_opcodeLevels[9] = program.m_levelCount - 1;
    program.m_opcodeLevels[10] = program.m_levelCount - 1;
    program.m_opcodeLevels[11] = program.m_levelCount - 1;
    program.m_opcodeLevels[12] = program.m_levelCount - 1;
    program.m_opcodeLevels[13] = program.m_levelCount - 1;
    program.m_opcodeLevels[24] = program.m_levelCount - 1;
    program.m_opcodeLevels[25] = program.m_levelCount - 1;
    program.m_opcodeLevels[26] = program.m_levelCount - 1;
    program.m_opcodeLevels[27] = program.m_levelCount - 1;
    program.m_opcodeLevels[28] = program.m_levelCount - 1;
    program.m_opcodeLevels[29] = program.m_levelCount - 1;
    program.m_opcodeLevels[30] = program.m_levelCount - 1;
    program.m_opcodeLevels[31] = program.m_levelCount - 1;

    program.m_opcodeLevels[15] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[19] = DisplayProgram_IgnoredLevel;

    program.m_levelInstructions[program.m_opcodeLevels[14]] = (unsigned char*)program.m_layerInstruction;
    program.m_layerLevel = program.m_opcodeLevels[14];
    program.m_blendBiased = mode->m_blendBiased;
    program.m_targetAction = mode->m_targetAction;
    program.m_stateActionSorted = mode->m_stateActionSorted;
    program.m_stateAction = mode->m_stateAction;
    program.m_drawActionSorted = mode->m_drawActionSorted;
    program.m_drawAction = mode->m_drawAction;
}

void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFrontIgnoreTargetCommands(void* context,
                                                                                             ICDisplayProgram& displayProgram)
{
    // as SortAllBackToFront without the target levels
    static const unsigned int instrLUTIndex[] = {
        CDisplayProgram::Instruction::Opcode::Layer,
        CDisplayProgram::Instruction::Opcode::Shaders,
        CDisplayProgram::Instruction::Opcode::Textures,
        CDisplayProgram::Instruction::Opcode::ColorMask,
        CDisplayProgram::Instruction::Opcode::BackfaceCulling,
        CDisplayProgram::Instruction::Opcode::Depth,
        CDisplayProgram::Instruction::Opcode::Scissor,
        CDisplayProgram::Instruction::Opcode::Blending,
        CDisplayProgram::Instruction::Opcode::ShaderParams
    };
    ICDisplayProgram::Mode* mode = (ICDisplayProgram::Mode*)context;
    CDisplayProgram& program = static_cast<CDisplayProgram&>(displayProgram);

    if (program.m_opcodeLevelCount != DisplayProgram_OpcodeLevelCount) {
        if (program.m_opcodeLevels != NULL) {
            delete[] program.m_opcodeLevels;
            program.m_opcodeLevels = NULL;
        }
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    } else if (program.m_opcodeLevels == NULL) {
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    }
    program.m_opcodeLevelCount = DisplayProgram_OpcodeLevelCount;

    program.InitializeDepthAndAllocLevel(10);
    for (unsigned int level = 0; level < 9; level++) {
        program.m_opcodeLevels[OPCODE_INDEX(instrLUTIndex[level])] = level;
    }

    program.m_opcodeLevels[2] = program.m_levelCount - 1;
    program.m_opcodeLevels[6] = program.m_levelCount - 1;
    program.m_opcodeLevels[7] = program.m_levelCount - 1;
    program.m_opcodeLevels[8] = program.m_levelCount - 1;
    program.m_opcodeLevels[9] = program.m_levelCount - 1;
    program.m_opcodeLevels[10] = program.m_levelCount - 1;
    program.m_opcodeLevels[11] = program.m_levelCount - 1;
    program.m_opcodeLevels[12] = program.m_levelCount - 1;
    program.m_opcodeLevels[13] = program.m_levelCount - 1;
    program.m_opcodeLevels[24] = program.m_levelCount - 1;
    program.m_opcodeLevels[25] = program.m_levelCount - 1;
    program.m_opcodeLevels[26] = program.m_levelCount - 1;
    program.m_opcodeLevels[27] = program.m_levelCount - 1;
    program.m_opcodeLevels[28] = program.m_levelCount - 1;
    program.m_opcodeLevels[29] = program.m_levelCount - 1;
    program.m_opcodeLevels[30] = program.m_levelCount - 1;
    program.m_opcodeLevels[31] = program.m_levelCount - 1;

    program.m_opcodeLevels[20] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[3] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[23] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[15] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[19] = DisplayProgram_IgnoredLevel;

    program.m_levelInstructions[program.m_opcodeLevels[14]] = (unsigned char*)program.m_layerInstruction;
    program.m_layerLevel = program.m_opcodeLevels[14];
    program.m_blendBiased = mode->m_blendBiased;
    program.m_targetAction = mode->m_targetAction;
    program.m_stateActionSorted = mode->m_stateActionSorted;
    program.m_stateAction = mode->m_stateAction;
    program.m_drawActionSorted = mode->m_drawActionSorted;
    program.m_drawAction = mode->m_drawAction;
}

void ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFrontIgnoreTargetColorMaskBackfaceCullingAndDepthCommands(
    void* context, ICDisplayProgram& displayProgram)
{
    // only layer, shaders, textures, scissor and blending are recorded
    static const unsigned int instrLUTIndex[] = {
        CDisplayProgram::Instruction::Opcode::Layer,
        CDisplayProgram::Instruction::Opcode::Shaders,
        CDisplayProgram::Instruction::Opcode::Textures,
        CDisplayProgram::Instruction::Opcode::Scissor,
        CDisplayProgram::Instruction::Opcode::Blending,
        CDisplayProgram::Instruction::Opcode::ShaderParams
    };
    ICDisplayProgram::Mode* mode = (ICDisplayProgram::Mode*)context;
    CDisplayProgram& program = static_cast<CDisplayProgram&>(displayProgram);

    if (program.m_opcodeLevelCount != DisplayProgram_OpcodeLevelCount) {
        if (program.m_opcodeLevels != NULL) {
            delete[] program.m_opcodeLevels;
            program.m_opcodeLevels = NULL;
        }
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    } else if (program.m_opcodeLevels == NULL) {
        program.m_opcodeLevels = new unsigned int[DisplayProgram_OpcodeLevelCount];
    }
    program.m_opcodeLevelCount = DisplayProgram_OpcodeLevelCount;

    program.InitializeDepthAndAllocLevel(7);
    for (unsigned int level = 0; level < 6; level++) {
        program.m_opcodeLevels[OPCODE_INDEX(instrLUTIndex[level])] = level;
    }

    program.m_opcodeLevels[2] = program.m_levelCount - 1;
    program.m_opcodeLevels[6] = program.m_levelCount - 1;
    program.m_opcodeLevels[7] = program.m_levelCount - 1;
    program.m_opcodeLevels[8] = program.m_levelCount - 1;
    program.m_opcodeLevels[9] = program.m_levelCount - 1;
    program.m_opcodeLevels[10] = program.m_levelCount - 1;
    program.m_opcodeLevels[11] = program.m_levelCount - 1;
    program.m_opcodeLevels[12] = program.m_levelCount - 1;
    program.m_opcodeLevels[13] = program.m_levelCount - 1;
    program.m_opcodeLevels[24] = program.m_levelCount - 1;
    program.m_opcodeLevels[25] = program.m_levelCount - 1;
    program.m_opcodeLevels[26] = program.m_levelCount - 1;
    program.m_opcodeLevels[27] = program.m_levelCount - 1;
    program.m_opcodeLevels[28] = program.m_levelCount - 1;
    program.m_opcodeLevels[29] = program.m_levelCount - 1;
    program.m_opcodeLevels[30] = program.m_levelCount - 1;
    program.m_opcodeLevels[31] = program.m_levelCount - 1;

    program.m_opcodeLevels[20] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[3] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[23] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[4] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[0] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[5] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[15] = DisplayProgram_IgnoredLevel;
    program.m_opcodeLevels[19] = DisplayProgram_IgnoredLevel;

    program.m_levelInstructions[program.m_opcodeLevels[14]] = (unsigned char*)program.m_layerInstruction;
    program.m_layerLevel = program.m_opcodeLevels[14];
    program.m_blendBiased = mode->m_blendBiased;
    program.m_targetAction = mode->m_targetAction;
    program.m_stateActionSorted = mode->m_stateActionSorted;
    program.m_stateAction = mode->m_stateAction;
    program.m_drawActionSorted = mode->m_drawActionSorted;
    program.m_drawAction = mode->m_drawAction;
}

ICDisplayProgram::Mode::Mode(Intrinsic intrinsic)
{
    Set(intrinsic);
}

void ICDisplayProgram::Mode::Set(Intrinsic intrinsic)
{
    switch (intrinsic) {
    case BlendBiased:
        m_blendBiased = 1;
        m_targetAction = 0;
        m_stateActionSorted = 0;
        m_stateAction = 2;
        m_drawActionSorted = 0;
        m_drawAction = 2;
        m_context = this;
        m_configure = ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_BlendBiased;
        break;
    case BlendBiasedIgnoreTargetCommands:
        m_blendBiased = 1;
        m_targetAction = 0;
        m_stateActionSorted = 0;
        m_stateAction = 2;
        m_drawActionSorted = 0;
        m_drawAction = 2;
        m_context = this;
        m_configure = ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_BlendBiasedIgnoreTargetCommands;
        break;
    case SortAllBackToFront:
        m_blendBiased = 0;
        m_targetAction = 0;
        m_stateActionSorted = 0;
        m_stateAction = 0;
        m_drawActionSorted = 0;
        m_drawAction = 0;
        m_context = this;
        m_configure = ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFront;
        break;
    case SortAllBackToFrontIgnoreTargetCommands:
        m_blendBiased = 0;
        m_targetAction = 0;
        m_stateActionSorted = 0;
        m_stateAction = 0;
        m_drawActionSorted = 0;
        m_drawAction = 0;
        m_context = this;
        m_configure = ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFrontIgnoreTargetCommands;
        break;
    case SortAllBackToFrontIgnoreTargetColorMaskBackfaceCullingAndDepthCommands:
        m_blendBiased = 0;
        m_targetAction = 0;
        m_stateActionSorted = 0;
        m_stateAction = 0;
        m_drawActionSorted = 0;
        m_drawAction = 0;
        m_context = this;
        m_configure = ICDisplayProgram_Mode_ConfigureDisplayProgramLevels_SortAllBackToFrontIgnoreTargetColorMaskBackfaceCullingAndDepthCommands;
        break;
    }
}

unsigned int CDisplayProgram::Instruction::Opcode::GetIntrinsicOpcode(const wchar_t* name)
{
    if (CStdUtil_Cocoa::WcsICmp(name, L"BackfaceCulling") == 0) {
        return BackfaceCulling;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Blending") == 0) {
        return Blending;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Callback") == 0) {
        return Callback;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"ClearBuffers") == 0) {
        return ClearBuffers;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"ColorMask") == 0) {
        return ColorMask;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Depth") == 0) {
        return Depth;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"IdxVtxStreamLines") == 0) {
        return IdxVtxStreamLines;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"IdxVtxStreamLinesBatch") == 0) {
        return IdxVtxStreamLinesBatch;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"IdxVtxStreamLineStrip") == 0) {
        return IdxVtxStreamLineStrip;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"IdxVtxStreamLineStripBatch") == 0) {
        return IdxVtxStreamLineStripBatch;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"IdxVtxStreamTris") == 0) {
        return IdxVtxStreamTris;
    }
    // the original maps the batch name to the plain opcode
    if (CStdUtil_Cocoa::WcsICmp(name, L"IdxVtxStreamTrisBatch") == 0) {
        return IdxVtxStreamTris;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"IdxVtxStreamTriStrip") == 0) {
        return IdxVtxStreamTriStrip;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"IdxVtxStreamTriStripBatch") == 0) {
        return IdxVtxStreamTriStripBatch;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Layer") == 0) {
        return Layer;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"RasterizerState") == 0) {
        return RasterizerState;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Scissor") == 0) {
        return Scissor;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Shaders") == 0) {
        return Shaders;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"ShaderParams") == 0) {
        return ShaderParams;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Stencil") == 0) {
        return Stencil;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Target") == 0) {
        return Target;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Textures") == 0) {
        return Textures;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Undefined") == 0) {
        return Undefined;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"Viewport") == 0) {
        return Viewport;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"VtxStreamLines") == 0) {
        return VtxStreamLines;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"VtxStreamLinesBatch") == 0) {
        return VtxStreamLinesBatch;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"VtxStreamLineStrip") == 0) {
        return VtxStreamLineStrip;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"VtxStreamLineStripBatch") == 0) {
        return VtxStreamLineStripBatch;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"VtxStreamTris") == 0) {
        return VtxStreamTris;
    }
    // the original maps the batch name to the plain opcode
    if (CStdUtil_Cocoa::WcsICmp(name, L"VtxStreamTrisBatch") == 0) {
        return VtxStreamTris;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"VtxStreamTriStrip") == 0) {
        return VtxStreamTriStrip;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, L"VtxStreamTriStripBatch") == 0) {
        return VtxStreamTriStripBatch;
    }
    return Undefined_Value;
}

CDisplayProgram::CDisplayProgram()
    : m_nodeChunks(10),
      m_indexBuffers(10),
      m_vertexBuffers(10),
      m_vertexStreams(100),
      m_indexStreams(50)
{
    m_levelCount = 0;
    m_levelInstructions = NULL;
    m_levelNodes = NULL;
    m_opcodeLevelCount = 0;
    m_opcodeLevels = NULL;
    m_levelLastAdded = NULL;
    m_levelEmpty = NULL;
    m_levelCallbackContexts = NULL;
    m_levelCallbacks = NULL;
    m_runStack.Initialize(100);
    m_levelIgnored = NULL;
    m_levelSavedInstructions = NULL;
    m_layerInstruction = NULL;
    m_nodeChunkLength = 0;
    m_indexBufferLength = 0;
    m_vertexBufferLength = 0;
    m_indexStreamLength = 0;
    m_vertexStreamLength = 0;
    m_nodeChunkIndex = 0;
    m_nodeChunk = NULL;
}

CDisplayProgram::~CDisplayProgram()
{
    CDisplayProgram::Destroy();
}

void CDisplayProgram::Destroy()
{
    if (m_levelSavedInstructions != NULL) {
        delete[] m_levelSavedInstructions;
        m_levelSavedInstructions = NULL;
    }
    if (m_levelIgnored != NULL) {
        delete[] m_levelIgnored;
        m_levelIgnored = NULL;
    }
    Reset(1);
    m_runStack.Destroy();
    if (m_levelCallbackContexts != NULL) {
        delete[] m_levelCallbackContexts;
        m_levelCallbackContexts = NULL;
    }
    if (m_levelCallbacks != NULL) {
        delete[] m_levelCallbacks;
        m_levelCallbacks = NULL;
    }
    if (m_levelLastAdded != NULL) {
        delete[] m_levelLastAdded;
        m_levelLastAdded = NULL;
    }
    if (m_opcodeLevels != NULL) {
        delete[] m_opcodeLevels;
        m_opcodeLevels = NULL;
    }
    if (m_levelNodes != NULL) {
        for (unsigned int level = 0; level < m_levelCount; level++) {
            if (m_levelNodes[level] != NULL) {
                delete (Instruction::LevelNode*)m_levelNodes[level];
            }
        }
        delete[] m_levelNodes;
        m_levelNodes = NULL;
    }
    if (m_levelInstructions != NULL) {
        delete[] m_levelInstructions;
        m_levelInstructions = NULL;
    }
    if (m_levelEmpty != NULL) {
        delete[] m_levelEmpty;
        m_levelEmpty = NULL;
    }
    if (m_levelCallbackContexts != NULL) {
        delete[] m_levelCallbackContexts;
        m_levelCallbackContexts = NULL;
    }
    if (m_levelCallbacks != NULL) {
        delete[] m_levelCallbacks;
        m_levelCallbacks = NULL;
    }
    if (m_layerInstruction != NULL) {
        delete m_layerInstruction;
        m_layerInstruction = NULL;
    }
    m_levelCount = 0;
    m_opcodeLevelCount = 0;
    m_nodeChunkLength = 0;
    m_indexBufferLength = 0;
    m_vertexBufferLength = 0;
    m_indexStreamLength = 0;
    m_vertexStreamLength = 0;
    m_nodeChunkIndex = 0;
    m_nodeChunk = NULL;
}

unsigned char CDisplayProgram::Initialize(const InitParam* params)
{
    ConfigDefaultAllocationInfo();

    if (params != NULL) {
        for (; params->m_key != InitParam::Key_End; params++) {
            switch (params->m_key) {
            case InitParam::Key_IndexBufferLength:
                if (params->m_value != 0) {
                    m_indexBufferLength = params->m_value;
                }
                break;
            case InitParam::Key_VertexBufferLength:
                if (params->m_value != 0) {
                    m_vertexBufferLength = params->m_value;
                }
                break;
            case InitParam::Key_IndexStreamLength:
                if (params->m_value != 0) {
                    m_indexStreamLength = params->m_value;
                }
                break;
            case InitParam::Key_VertexStreamLength:
                if (params->m_value != 0) {
                    m_vertexStreamLength = params->m_value;
                }
                break;
            }
        }
    }

    Instruction::LayerInstruction* layerInstruction = new Instruction::LayerInstruction;
    m_layerInstruction = layerInstruction;
    layerInstruction->m_header.m_child = NULL;
    layerInstruction->m_header.m_sibling = NULL;
    layerInstruction->m_header.m_size = sizeof(Instruction::LayerInstruction);
    m_layerInstruction->m_header.m_opcode = Instruction::Opcode::Layer;
    SetLayer(0);

    NodeChunk memory;
    // Keep allocation separate from registering the chunk fields.
    unsigned int* data = new unsigned int[m_nodeChunkLength];
    memory.m_capacity = m_nodeChunkLength;
    memory.m_data = data;
    memory.m_used = 0;
    m_nodeChunks.Add(memory);
    m_nodeChunkIndex = 0;
    m_nodeChunk = m_nodeChunks.m_head;
    return 1;
}

void CDisplayProgram::Reset(unsigned char releaseMemory)
{
    // instruction memory
    for (Pool<NodeChunk>::Chunk* chunk = m_nodeChunks.m_head; chunk != NULL; chunk = chunk->m_next) {
        for (int index = 0; index < chunk->m_elements.m_size; index++) {
            NodeChunk& memory = chunk->m_elements.m_data[index];
            if (releaseMemory) {
                delete[] memory.m_data;
                memory.m_data = NULL;
                memory.m_used = 0;
                memory.m_capacity = 0;
            } else {
                memory.m_used = 0;
            }
        }
    }
    m_nodeChunkIndex = 0;
    m_nodeChunk = m_nodeChunks.m_head;

    // shared vertex buffers
    for (Pool<BufferRef<ICVertexBuffer*> >::Chunk* chunk = m_vertexBuffers.m_head; chunk != NULL;
         chunk = chunk->m_next) {
        for (int index = 0; index < chunk->m_elements.m_size; index++) {
            BufferRef<ICVertexBuffer*>& ref = chunk->m_elements.m_data[index];
            if (releaseMemory) {
                if (ref.m_buffer != NULL) {
                    delete ref.m_buffer;
                }
                ref.m_key = 0;
                ref.m_buffer = NULL;
                ref.m_used = 0;
                ref.m_capacity = 0;
            } else {
                ref.m_used = 0;
            }
        }
    }

    // shared index buffers
    for (Pool<BufferRef<ICIndexBuffer*> >::Chunk* chunk = m_indexBuffers.m_head; chunk != NULL;
         chunk = chunk->m_next) {
        for (int index = 0; index < chunk->m_elements.m_size; index++) {
            BufferRef<ICIndexBuffer*>& ref = chunk->m_elements.m_data[index];
            if (releaseMemory) {
                if (ref.m_buffer != NULL) {
                    delete ref.m_buffer;
                }
                ref.m_key = 0;
                ref.m_buffer = NULL;
                ref.m_used = 0;
                ref.m_capacity = 0;
            } else {
                ref.m_used = 0;
            }
        }
    }

    // batch streams
    for (Pool<ICBuffer::Stream>::Chunk* chunk = m_vertexStreams.m_head; chunk != NULL; chunk = chunk->m_next) {
        chunk->m_elements.Clear();
    }
    m_vertexStreams.m_current = m_vertexStreams.m_head;
    for (Pool<ICBuffer::Stream>::Chunk* chunk = m_indexStreams.m_head; chunk != NULL; chunk = chunk->m_next) {
        chunk->m_elements.Clear();
    }
    m_indexStreams.m_current = m_indexStreams.m_head;

    // every level starts empty again, the layer instruction stays in place
    for (unsigned int level = 0; level < m_levelCount; level++) {
        m_levelInstructions[level] = (unsigned char*)m_levelNodes[level];
    }
    if (m_levelInstructions != NULL && m_opcodeLevels[14] != DisplayProgram_IgnoredLevel) {
        m_layerInstruction->m_header.m_level = m_opcodeLevels[14];
        m_levelInstructions[m_opcodeLevels[14]] = (unsigned char*)m_layerInstruction;
    }
    m_root = NULL;
}

void CDisplayProgram::SetLayer(int layer)
{
    m_layerInstruction->m_layer = layer;
}

int CDisplayProgram::GetLayer()
{
    return m_layerInstruction->m_layer;
}

void CDisplayProgram::SetCallBackInstructionExecution(unsigned int level, void* context,
                                                      void (*callback)(void* context, unsigned char* instruction))
{
    m_levelCallbackContexts[level] = context;
    m_levelCallbacks[level] = callback;
}

void CDisplayProgram::Run()
{
    m_runStack.Top() = NULL;
    for (unsigned int index = 0; index < m_levelCount; index++) {
        m_levelEmpty[index] = 0;
    }

    Instruction::Header* node = m_root;
    unsigned int level = 0;
    while (node != NULL) {
        // down to the last level, executing every instruction on the way
        while (level < m_levelCount) {
            m_runStack.Push();
            m_runStack.Top() = node;
            if (node->m_size <= Instruction::LevelNodeSize) {
                if (m_levelEmpty[level] == 0) {
                    m_levelEmpty[level] = 1;
                }
            } else {
                m_levelEmpty[level] = 0;
                m_levelCallbacks[level](m_levelCallbackContexts[level], (unsigned char*)node);
            }
            Instruction::Header* child = node->m_child;
            if (child == NULL) {
                child = (Instruction::Header*)((unsigned char*)node + node->m_size);
            }
            level++;
            node = child;
        }

        // back up to the closest instruction with a sibling
        node = NULL;
        do {
            if (level == 0) {
                break;
            }
            level--;
            node = m_runStack.Top()->m_sibling;
            m_runStack.Pop();
        } while (node == NULL);
    }
}

void CDisplayProgram::Add(unsigned char* instruction)
{
    CDisplayProgram* pDisplayProgram = this;
    unsigned char* pInstruction = instruction;
#include "CDisplayProgram_Add.i"
}

} } } }
