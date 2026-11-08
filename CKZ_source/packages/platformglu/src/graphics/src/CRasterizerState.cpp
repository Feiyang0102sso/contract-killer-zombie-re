// CRasterizerState.cpp - device independent rasterizer state.
#include "CRasterizerState.h"
#include "CDisplayProgram.h"
#include "ICGraphics.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// history slots per state entry (PushState depth)
static const unsigned int RasterizerState_HistoryDepth = 0x10;

CRasterizerState_v1::CRasterizerState_v1() : core::CClass(ClassId)
{
    m_graphics = ICGraphics::GetInstance();
    m_table.Initialize(10);
    m_table.Add(L"BackfaceCulling", components::CTypedVariableTable::Entry::Integer, 1, RasterizerState_HistoryDepth);
    m_table.Add(L"Blending", components::CTypedVariableTable::Entry::Integer, 2, RasterizerState_HistoryDepth);
    m_table.Add(L"ColorMask", components::CTypedVariableTable::Entry::Integer, 1, RasterizerState_HistoryDepth);
    m_table.Add(L"Depth", components::CTypedVariableTable::Entry::Float, 6, RasterizerState_HistoryDepth);
    m_table.Add(L"Scissor", components::CTypedVariableTable::Entry::Integer, 5, RasterizerState_HistoryDepth);
    m_backfaceCullingIndex = m_table.GetEntryIndex(L"BackfaceCulling");
    m_blendingIndex = m_table.GetEntryIndex(L"Blending");
    m_colorMaskIndex = m_table.GetEntryIndex(L"ColorMask");
    m_depthIndex = m_table.GetEntryIndex(L"Depth");
    m_scissorIndex = m_table.GetEntryIndex(L"Scissor");

    // the whole table is a "RasterizerState" instruction, each entry an instruction of its own name
    CDisplayProgram::Instruction::Header* header = (CDisplayProgram::Instruction::Header*)m_table.GetTableHeader();
    header->m_child = NULL;
    header->m_sibling = NULL;
    header->m_size = (unsigned short)m_table.GetTableSize();
    header->m_opcode = CDisplayProgram::Instruction::Opcode::GetIntrinsicOpcode(L"RasterizerState");
    for (unsigned int index = 0; index < m_table.GetNumberOfEntries(); index++) {
        header = (CDisplayProgram::Instruction::Header*)m_table.GetEntryHeader(index);
        header->m_child = NULL;
        header->m_sibling = NULL;
        header->m_size = (unsigned short)(m_table.GetEntryDataSize(index) + m_table.m_entryHeaderSize);
        header->m_opcode = CDisplayProgram::Instruction::Opcode::GetIntrinsicOpcode(m_table.GetEntryName(index));
    }

    m_stateLevels[0] = NULL;
    m_stateLevels[1] = NULL;
    m_stateLevels[2] = NULL;
    m_stateLevels[3] = NULL;
    m_stateLevels[4] = NULL;
}

void CRasterizerState_v1::GetColorMaskState(unsigned char& red, unsigned char& green, unsigned char& blue,
                                            unsigned char& alpha)
{
    unsigned int* mask = (unsigned int*)m_table.GetEntryData(m_colorMaskIndex);
    red = (unsigned char)(*mask >> 24);
    green = (unsigned char)(*mask >> 16);
    blue = (unsigned char)(*mask >> 8);
    alpha = (unsigned char)*mask;
}

void CRasterizerState_v1::GetCullState(CullState& state)
{
    state = *(CullState*)m_table.GetEntryData(m_backfaceCullingIndex);
}

void CRasterizerState_v1::GetBlendState(unsigned char& enabled, BlendFactor& source, BlendFactor& destination,
                                        BlendFactor& sourceAlpha, BlendFactor& destinationAlpha, BlendOp& op,
                                        BlendOp& opAlpha)
{
    unsigned int* blending = (unsigned int*)m_table.GetEntryData(m_blendingIndex);
    enabled = (unsigned char)(blending[0] >> 31);
    op = (BlendOp)((blending[0] >> 8) & 0xff);
    opAlpha = (BlendOp)(blending[0] & 0xff);
    source = (BlendFactor)(blending[1] >> 24);
    destination = (BlendFactor)((blending[1] >> 16) & 0xff);
    sourceAlpha = (BlendFactor)((blending[1] >> 8) & 0xff);
    destinationAlpha = (BlendFactor)(blending[1] & 0xff);
}

void CRasterizerState_v1::GetDepthState(DepthTestFunc& func, unsigned char& write, float& bias, float& nearValue,
                                        float& farValue)
{
    float* depth = (float*)m_table.GetEntryData(m_depthIndex);
    func = (DepthTestFunc)(unsigned int)depth[0];
    write = (unsigned char)(int)depth[1];
    bias = depth[2];
    nearValue = depth[4];
    farValue = depth[5];
}

void CRasterizerState_v1::GetScissorState(unsigned char& enabled, short& x, short& y, short& width, short& height)
{
    int* scissor = (int*)m_table.GetEntryData(m_scissorIndex);
    enabled = (unsigned char)scissor[0];
    x = (short)scissor[1];
    y = (short)scissor[2];
    width = (short)scissor[3];
    height = (short)scissor[4];
}

void CRasterizerState_v1::PushState(State state)
{
    switch (state) {
    case State_BackfaceCulling:
        m_table.PushEntry(m_backfaceCullingIndex);
        break;
    case State_Blending:
        m_table.PushEntry(m_blendingIndex);
        break;
    case State_ColorMask:
        m_table.PushEntry(m_colorMaskIndex);
        break;
    case State_Depth:
        m_table.PushEntry(m_depthIndex);
        break;
    case State_Scissor:
        m_table.PushEntry(m_scissorIndex);
        break;
    }
}

} } } }
