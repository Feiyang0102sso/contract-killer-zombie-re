// CGraphics_OGLES_DrawElementsAddToDisplayProgram.i - Draw(Primitive, indices, first, count) while a
// display program is recorded (created by b037x). Uses: this, indices, first, count.
{
    CDisplayProgram* pRecordProgram = static_cast<CDisplayProgram*>(m_displayProgram);
    if (m_vertexStreams[0] == NULL) {
        return;
    }
    if (!m_vertexStreamEnabled[0] && !m_vertexStreamEnable[0]) {
        return;
    }
    ICBuffer* pIndexBuffer = indices.m_buffer;
    if (pIndexBuffer == NULL) {
        return;
    }
    if (pIndexBuffer->GetType() == ICBuffer::Type_Index) {
        CDisplayProgram::Instruction::Header* pDrawHeader = (CDisplayProgram::Instruction::Header*)m_drawInstruction;
        pDrawHeader->m_child = NULL;
        pDrawHeader->m_sibling = NULL;
        pDrawHeader->m_size = sizeof(DrawElementsInstruction);
        pDrawHeader->m_level = (unsigned short)
            pRecordProgram->m_opcodeLevels[CDisplayProgram::Instruction::Opcode::IdxVtxStreamTriStrip & 0xff];
        DrawElementsInstruction* pDraw = (DrawElementsInstruction*)m_drawInstruction;
        pDraw->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::IdxVtxStreamTriStrip;
        pDraw->m_vertexStream = m_vertexStreams[0];
        pDraw->m_reserved = 0;
        pDraw->m_vertexCount = m_vertexStreams[0]->m_count;
        pDraw->m_indexStream = &indices;
        pDraw->m_indexType = static_cast<ICIndexBuffer*>(pIndexBuffer)->GetIndexType();
        pDraw->m_count = count;
        pDraw->m_first = first;
        pRecordProgram->Add(m_drawInstruction);
    }
    return;
}
