// CGraphics_OGLES_UpdateAttributes.i - point the fixed function client arrays at the vertex stream
// and enable / disable them (created by b037x). Uses: this.
{
    ICBuffer::Stream* pAttributeStream = m_vertexStreams[0];
    if (pAttributeStream != NULL && pAttributeStream->m_dirty) {
        ICVertexBuffer* pAttributeBuffer = (ICVertexBuffer*)pAttributeStream->m_buffer;
        if (pAttributeBuffer != NULL && pAttributeBuffer->GetType() == ICBuffer::Type_Vertex) {
            CVertex::Decl* pDecl = pAttributeBuffer->GetDecl();
            unsigned char* pVertexData;
            int vertexDataSize;
            pAttributeBuffer->Lock(&pVertexData, vertexDataSize,
                                   m_vertexStreams[0]->m_first * pAttributeBuffer->GetStride(), 0x7fffffff,
                                   ICBuffer::LockType_Read);
            m_streamArrays[0].Clear();
            for (int attributeIndex = 0; attributeIndex < pDecl->m_attributes.m_size; attributeIndex++) {
                CVertex::Attribute::Decl* pAttribute = &pDecl->m_attributes.m_data[attributeIndex].m_decl;
                if (pAttribute->m_id == CVertex::Attribute::Id::Color) {
                    if (m_fixedFunction) {
                        glColorPointer((short)pAttribute->m_count, GetGLComponentType(pAttribute->m_type),
                                       (short)pAttribute->m_stride, pVertexData + pAttribute->m_offset);
                    }
                    unsigned int colorArray = ClientArray_Color;
                    m_streamArrays[0].Add(colorArray);
                } else if (pAttribute->m_id == CVertex::Attribute::Id::TexCoord) {
                    if (m_fixedFunction) {
                        glTexCoordPointer((short)pAttribute->m_count, GetGLComponentType(pAttribute->m_type),
                                          (short)pAttribute->m_stride, pVertexData + pAttribute->m_offset);
                    }
                    unsigned int texCoordArray = ClientArray_TexCoord0;
                    m_streamArrays[0].Add(texCoordArray);
                    m_texCoordType = pAttribute->m_type;
                } else if (pAttribute->m_id == CVertex::Attribute::Id::Position) {
                    if (m_fixedFunction) {
                        glVertexPointer((short)pAttribute->m_count, GetGLComponentType(pAttribute->m_type),
                                        (short)pAttribute->m_stride, pVertexData + pAttribute->m_offset);
                    }
                    unsigned int vertexArray = ClientArray_Vertex;
                    m_streamArrays[0].Add(vertexArray);
                }
            }
        }
        m_vertexStreamDirty[0] = 0;
        m_vertexStreams[0]->m_dirty = 0;
    }

    if (m_vertexStreamEnable[0]) {
        if (m_vertexStreams[0] != NULL) {
            for (int arrayIndex = 0; arrayIndex < m_streamArrays[0].m_size; arrayIndex++) {
                unsigned int array = m_streamArrays[0].m_data[arrayIndex];
                m_arrayRequested[array] = 1;
                if (!m_arrayEnabled[array]) {
                    EnableClientArray(array);
                }
            }
        }
        m_vertexStreamEnable[0] = 0;
        m_vertexStreamEnabled[0] = 1;
    }

    for (unsigned int array = 0; array < 4; array++) {
        if (m_arrayRequested[array] == 0 && m_arrayEnabled[array]) {
            m_arrayEnabled[array] = 0;
            DisableClientArray(array);
        }
    }
}
