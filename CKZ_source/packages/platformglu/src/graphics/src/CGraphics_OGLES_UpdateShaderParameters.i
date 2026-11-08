// CGraphics_OGLES_UpdateShaderParameters.i - emulate the parameters of the current shader program
// with fixed function state (created by b037x). Uses: this, pShaderProgram.
{
    CShaderProgram_OGLES* pProgramView = (CShaderProgram_OGLES*)pShaderProgram;
    unsigned char* pSavedParameters = NULL;
    if (m_shaderParamsInstruction != NULL) {
        pSavedParameters = pProgramView->m_parameterTable->m_table;
        pProgramView->m_parameterTable->m_table = m_shaderParamsInstruction;
    }
    for (unsigned int parameter = 0; parameter < pProgramView->m_parameterCount; parameter++) {
        unsigned int semantic = pProgramView->m_parameterSemantics[parameter];
        if (semantic == CShaderProgram_OGLES::ParameterSemantic_TexEnv0 ||
            semantic == CShaderProgram_OGLES::ParameterSemantic_TexEnv1) {
            ICShaderProgram::ParameterTable* pTable = pProgramView->m_parameterTable;
            int* pTexEnv = (int*)pTable->GetEntryData(pProgramView->m_parameterEntries[parameter]);
            if (m_fixedFunction) {
                glActiveTexture(GL_TEXTURE0 + pProgramView->m_parameterSemantics[parameter] -
                                CShaderProgram_OGLES::ParameterSemantic_TexEnv0);
            }
            switch (*pTexEnv) {
            case 0:
                if (m_fixedFunction) {
                    glDisable(GL_TEXTURE_2D);
                }
                break;
            case 1:
                if (m_fixedFunction) {
                    glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
                    if (m_fixedFunction) {
                        glEnable(GL_TEXTURE_2D);
                    }
                }
                break;
            case 3:
                if (m_fixedFunction) {
                    glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    if (m_fixedFunction) {
                        glEnable(GL_TEXTURE_2D);
                    }
                }
                break;
            case 4:
                if (m_fixedFunction) {
                    glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                    if (m_fixedFunction) {
                        glEnable(GL_TEXTURE_2D);
                    }
                }
                break;
            }
        } else if (semantic == CShaderProgram_OGLES::ParameterSemantic_Mvp) {
            ICShaderProgram::ParameterTable* pTable = pProgramView->m_parameterTable;
            GLfloat* pMatrix = (GLfloat*)pTable->GetEntryData(pProgramView->m_parameterEntries[parameter]);
            if (m_fixedFunction) {
                glLoadMatrixf(pMatrix);
            }
        } else if (semantic == CShaderProgram_OGLES::ParameterSemantic_ConstColor) {
            ICShaderProgram::ParameterTable* pTable = pProgramView->m_parameterTable;
            GLfloat* pColor = (GLfloat*)pTable->GetEntryData(pProgramView->m_parameterEntries[parameter]);
            if (m_fixedFunction) {
                glColor4f(pColor[0], pColor[1], pColor[2], pColor[3]);
            }
        }
    }
    // Restore only a saved table, as in the original draw paths.
    if (pSavedParameters != NULL) {
        pProgramView->m_parameterTable->m_table = pSavedParameters;
    }

    // short texture coordinates are 4.12 fixed point: scale them through the texture matrix
    if (m_textureMatrixOverride == 0 && m_texCoordType != m_textureMatrixType) {
        if (m_fixedFunction) {
            glMatrixMode(GL_TEXTURE);
            if (m_fixedFunction) {
                glLoadIdentity();
            }
        }
        if ((m_texCoordType | 0x1000000) == ComponentType_UnsignedShort) {
            if (m_fixedFunction) {
                glScalex(0x10, 0x10, math::CMathFixed::One);
            }
        }
        if (m_fixedFunction) {
            glMatrixMode(GL_MODELVIEW);
        }
        m_textureMatrixType = m_texCoordType;
    }
}
