// CGraphics_OGLES_DrawArraysAddToDisplayProgram.i - Draw(Primitive, first, count) while a display
// program is recorded (created by b037x). Uses: this, first, count.
{
    CDisplayProgram* pRecordProgram = static_cast<CDisplayProgram*>(m_displayProgram);
    CDisplayProgram::Instruction::Header* pDrawHeader = (CDisplayProgram::Instruction::Header*)m_drawInstruction;
    pDrawHeader->m_size = sizeof(DrawArraysInstruction);
    pDrawHeader->m_level =
        (unsigned short)pRecordProgram->m_opcodeLevels[CDisplayProgram::Instruction::Opcode::VtxStreamTris & 0xff];
    DrawArraysInstruction* pDraw = (DrawArraysInstruction*)m_drawInstruction;
    pDraw->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::VtxStreamTris;
    pDraw->m_reserved = 0;
    pDraw->m_count = count;
    pDraw->m_first = first;
    if (m_vertexStreams[0] == NULL) {
        return;
    }
    if (!m_vertexStreamEnabled[0] && !m_vertexStreamEnable[0]) {
        return;
    }
    ICBuffer* pVertexBuffer = m_vertexStreams[0]->m_buffer;
    if (pVertexBuffer == NULL) {
        return;
    }
    if (pVertexBuffer->GetType() == ICBuffer::Type_Vertex) {
        pDrawHeader->m_child = NULL;
        pDrawHeader->m_sibling = NULL;
        pDraw->m_vertexStream = m_vertexStreams[0];
        pRecordProgram->Add(m_drawInstruction);
    }
    return;
}
