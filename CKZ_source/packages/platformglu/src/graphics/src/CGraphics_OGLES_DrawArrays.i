// CGraphics_OGLES_DrawArrays.i - glDrawArrays of the vertex stream (created by b037x).
// Uses: this, primitive, first, count.
{
    ICBuffer::Stream* pDrawStream = m_vertexStreams[0];
    ICBuffer* pDrawBuffer = NULL;
    if (pDrawStream != NULL) {
        pDrawBuffer = pDrawStream->m_buffer;
    }
    if (pDrawStream != NULL && pDrawBuffer != NULL && pDrawBuffer->GetType() == ICBuffer::Type_Vertex) {
        if (count == 0x7fffffff) {
            count = m_vertexStreams[0]->m_count;
        }
        GLenum mode = GetGLPrimitive(primitive);
        if (m_fixedFunction) {
            glDrawArrays(mode, first, count);
        }
    }
}
