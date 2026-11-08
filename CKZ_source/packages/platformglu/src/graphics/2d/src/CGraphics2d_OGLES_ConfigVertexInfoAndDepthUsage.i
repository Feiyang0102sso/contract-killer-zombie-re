// CGraphics2d_OGLES_ConfigVertexInfoAndDepthUsage.i - vertex layout and depth write set-up of the
// CGraphics2d_OGLES draws (b035). With a depth test the vertices get a depth coordinate; blended
// draws do not write depth.
// The includer provides:
//   unsigned char blendEnabled                    result of CGraphics2d_OGLES_ConfigBlending.i
//   CONFIG_VERTEX_FORMAT_2D, CONFIG_VERTEX_FORMAT_3D   VertexFormat without / with depth
{
    CRasterizerState_v1* pDepthState = m_rasterizerState;
    float* pDepth = (float*)pDepthState->m_table.GetEntryData(pDepthState->m_depthIndex);
    unsigned int depthFunc = (unsigned int)pDepth[0];
    int depthWrite = (int)pDepth[1];
    if (depthFunc == 0) {
        if (m_vertexFormat != CONFIG_VERTEX_FORMAT_2D) {
            ConfigureVertexInfo(CONFIG_VERTEX_FORMAT_2D);
        }
    } else {
        if (m_vertexFormat != CONFIG_VERTEX_FORMAT_3D) {
            ConfigureVertexInfo(CONFIG_VERTEX_FORMAT_3D);
        }
        if (blendEnabled) {
            if (depthWrite != 0) {
                m_rasterizerState->EnableDepthWrite(0);
            }
        } else {
            if (depthWrite == 0) {
                m_rasterizerState->EnableDepthWrite(1);
            }
        }
    }
}
