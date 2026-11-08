// CGraphics_OGLES_DrawElements.i - glDrawElements of an index stream (created by b037x).
// Uses: this, primitive, indices, first, count.
{
    ICBuffer* pIndexBuffer = indices.m_buffer;
    if (pIndexBuffer == NULL) {
        return;
    }
    if (pIndexBuffer->GetType() != ICBuffer::Type_Index) {
        return;
    }
    if (count == 0x7fffffff) {
        count = indices.m_count;
    }
    int indexStride = pIndexBuffer->GetStride();
    unsigned char* pIndices;
    int indexSize;
    pIndexBuffer->Lock(&pIndices, indexSize, (indices.m_first + first) * indexStride, indexStride * count,
                       ICBuffer::LockType_ReadWrite);
    GLenum mode = GetGLPrimitive(primitive);
    GLenum indexType = GetGLComponentType(static_cast<ICIndexBuffer*>(pIndexBuffer)->GetIndexType());
    if (m_fixedFunction) {
        glDrawElements(mode, count, indexType, pIndices);
    }
}
