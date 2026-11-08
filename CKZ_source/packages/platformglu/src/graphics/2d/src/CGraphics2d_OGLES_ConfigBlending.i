// CGraphics2d_OGLES_ConfigBlending.i - blending set-up of the CGraphics2d_OGLES draws (b035).
// Blending stays enabled when the draw needs it; otherwise the blending state is pushed and
// disabled (the includer pops it again when blendPushed is set).
// The includer provides:
//   const components::Color_RGBA* pBlendColor     constant color of the draw (NULL: none)
//   unsigned char blendPushed, blendEnabled       results
//   unsigned char blendNeeded                     scratch (declared by the includer)
//   CONFIG_BLENDING_TEXTURE_NEEDS_BLEND           condition used for the default sampler hint
{
    CRasterizerState_v1* pBlendState = m_rasterizerState;
    unsigned int* pBlending = (unsigned int*)pBlendState->m_table.GetEntryData(pBlendState->m_blendingIndex);
    blendPushed = 0;
    blendEnabled = 0;
    if ((int)pBlending[0] < 0) {
        // the standard alpha blend with the add equation can be dropped for opaque draws
        if (pBlending[1] == 0x07070505 && (pBlending[0] & 0xffff) == 0 &&
            // The original less-than test keeps unordered alpha values on the opaque path.
            (pBlendColor == NULL || !(pBlendColor->a < 0.98f))) {
            switch (m_graphics->m_textureSampleHints[0]) {
            case 0:
                blendNeeded = CONFIG_BLENDING_TEXTURE_NEEDS_BLEND;
                break;
            case 1:
                blendNeeded = 0;
                break;
            case 2:
                blendNeeded = m_graphics->m_alphaTest == 0;
                break;
            }
            blendEnabled = blendNeeded;
            if (!blendEnabled) {
                blendPushed = 1;
                m_rasterizerState->PushState(ICRasterizerState::State_Blending);
                m_rasterizerState->EnableBlending(0);
            }
        } else {
            blendEnabled = 1;
        }
    }
}
