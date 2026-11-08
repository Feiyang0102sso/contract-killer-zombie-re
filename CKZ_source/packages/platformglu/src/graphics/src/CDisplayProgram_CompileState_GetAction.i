// CDisplayProgram_CompileState_GetAction.i - included by CDisplayProgram_Add.i (created by b037x).
// Decides what to do with a pending STATE instruction (any level but the last one) that differs
// from the recorded candidate at the same level. Sets `addAction` to one of the AddAction_ values
// (see CDisplayProgram_Add.i). Uses: pDisplayProgram, pAddInstruction, pAddCandidate,
// addSortedByLayer, addSearchLastSibling, addLayerShared.
// fx1: the original dispatches on the opcode with a switch (binary compare tree on
// Blending / ClearBuffers / Layer / Target / Viewport).
{
    unsigned int addStateAction;
    unsigned int addOpcode = pAddInstruction->m_opcode;

    if (addSortedByLayer) {
        addStateAction = pDisplayProgram->m_stateActionSorted;
    } else {
        addStateAction = pDisplayProgram->m_stateAction;
    }

    switch (addOpcode) {
    case CDisplayProgram::Instruction::Opcode::Layer:
        // layers are kept sorted: a lower layer goes in front of the candidate
        addAction = AddAction_NextSibling;
        if (((CDisplayProgram::Instruction::LayerInstruction*)pAddInstruction)->m_layer <
            ((CDisplayProgram::Instruction::LayerInstruction*)pAddCandidate)->m_layer) {
            addAction = AddAction_Insert;
        }
        if (!pDisplayProgram->m_blendBiased && addStateAction == 0) {
            pDisplayProgram->m_layerMatched = 1;
            addLayerShared = 1;
        }
        break;

    case CDisplayProgram::Instruction::Opcode::ClearBuffers:
    case CDisplayProgram::Instruction::Opcode::Target:
    case CDisplayProgram::Instruction::Opcode::Viewport:
        // target commands follow the target action of the mode
        addAction = AddAction_Insert;
        if (pDisplayProgram->m_targetAction == 2) {
            addAction = AddAction_NextSibling;
        } else if (pDisplayProgram->m_targetAction == 0) {
            if (pAddCandidate->m_sibling != NULL) {
                addSearchLastSibling = 1;
                pDisplayProgram->m_searchLastSibling = 1;
                addAction = AddAction_LastSibling;
            } else {
                addAction = AddAction_NextSibling;
            }
        }
        break;

    case CDisplayProgram::Instruction::Opcode::Blending:
        addAction = AddAction_Insert;
        if (!pDisplayProgram->m_blendBiased) {
            // not blend biased: like a draw, the last sibling is taken without switching the
            // search mode (the original does not set m_searchLastSibling here)
            if (addStateAction == 2) {
                addAction = AddAction_NextSibling;
            } else if (addStateAction == 0) {
                if (pAddCandidate->m_sibling != NULL) {
                    addAction = AddAction_LastSibling;
                } else {
                    addAction = AddAction_NextSibling;
                }
            }
        } else if (((unsigned int*)pAddCandidate)[4] == 0 || addSortedByLayer) {
            // blend biased: an unblended candidate is shared like any other state
            if (addStateAction == 2) {
                addAction = AddAction_NextSibling;
            } else if (addStateAction == 0) {
                if (pAddCandidate->m_sibling != NULL) {
                    addSearchLastSibling = 1;
                    pDisplayProgram->m_searchLastSibling = 1;
                    addAction = AddAction_LastSibling;
                } else {
                    addAction = AddAction_NextSibling;
                }
            }
        }
        break;

    default:
        addAction = AddAction_Insert;
        if (addStateAction == 2) {
            addAction = AddAction_NextSibling;
        } else if (addStateAction == 0) {
            if (pAddCandidate->m_sibling == NULL) {
                addAction = AddAction_NextSibling;
            } else {
                addSearchLastSibling = 1;
                pDisplayProgram->m_searchLastSibling = 1;
                addAction = AddAction_LastSibling;
            }
        }
        break;
    }
}
