// CDisplayProgram_Add.i - body of CDisplayProgram::Add (created by b037x). Also included by
// recorders that add instructions inline (graphics/2d/src/CGraphics2d_OGLES.cpp).
// The includer provides:
//   CDisplayProgram* pDisplayProgram   program being recorded
//   unsigned char* pInstruction        instruction to record (copied, the caller keeps it)
// The pending state instruction of every level (m_levelInstructions) together with the new
// instruction (last level) form a chain. The chain is matched against the recorded tree from the
// root; equal state instructions are shared, the rest of the chain is copied into instruction
// memory and linked in where the mode's actions say.
{
    // actions of the search (CDisplayProgram_CompileState_GetAction.i decides them for states)
    enum {
        AddAction_Share = 0,        // the candidate equals the pending instruction: go one level down
        AddAction_Merged = 1,       // the draw was merged into the candidate batch: nothing to add
        AddAction_Insert = 2,       // link the chain in before the next sibling of addPrevious
        AddAction_NextSibling = 3,  // look at the next sibling (append after the last one)
        AddAction_LastSibling = 5   // look at the last sibling of the candidate only
    };

    unsigned int addLevelCount;
    unsigned int addLastLevel;
    unsigned int addLevel;
    unsigned int addIndex;
    unsigned char addSortedByLayer;
    unsigned char addSearchLastSibling;
    unsigned char addLayerShared;
    unsigned char addShareLayer;
    unsigned char addOnLastAddedPath;
    int addAction;
    CDisplayProgram::Instruction::Header* pAddNode;
    CDisplayProgram::Instruction::Header* pAddParent;
    CDisplayProgram::Instruction::Header* pAddPrevious;
    CDisplayProgram::Instruction::Header* pAddCandidate;
    CDisplayProgram::Instruction::Header* pAddInstruction;
    unsigned int* pAddWords;

    // the new instruction is the pending instruction of the last level
    pDisplayProgram->m_levelInstructions[pDisplayProgram->m_levelCount - 1] = pInstruction;

    // ignored levels take part as their empty node
    addLevelCount = pDisplayProgram->m_levelCount;
    for (addIndex = 0; addIndex < addLevelCount; addIndex++) {
        if (pDisplayProgram->m_levelIgnored[addIndex] != 0) {
            pDisplayProgram->m_levelSavedInstructions[addIndex] = pDisplayProgram->m_levelInstructions[addIndex];
            pDisplayProgram->m_levelInstructions[addIndex] =
                (unsigned char*)pDisplayProgram->m_levelNodes[addIndex];
            addLevelCount = pDisplayProgram->m_levelCount;
        }
    }

    pDisplayProgram->m_searchLastSibling = 0;
    pDisplayProgram->m_layerMatched = 0;
    addSortedByLayer = pDisplayProgram->m_sortedByLayer;
    if (addSortedByLayer) {
        pDisplayProgram->m_mergeDraws = pDisplayProgram->m_drawActionSorted == 0;
    } else {
        pDisplayProgram->m_mergeDraws = pDisplayProgram->m_drawAction == 0;
    }

    addLastLevel = addLevelCount - 1;
    pAddNode = pDisplayProgram->m_root;
    pAddParent = NULL;
    pAddPrevious = NULL;
    pAddCandidate = NULL;
    addSearchLastSibling = 0;
    addShareLayer = 0;
    addOnLastAddedPath = 1;
    addAction = AddAction_Insert;

    // ---- find where the chain leaves the recorded tree ----
    for (addLevel = 0; addLevel < addLevelCount; addLevel++) {
        pAddInstruction = (CDisplayProgram::Instruction::Header*)pDisplayProgram->m_levelInstructions[addLevel];
        if (addLevel == addLastLevel) {
            addSearchLastSibling = pDisplayProgram->m_mergeDraws;
            pDisplayProgram->m_searchLastSibling = addSearchLastSibling;
            addLayerShared = addShareLayer;
        } else {
            addLayerShared = 0;
            if (addShareLayer) {
                addSearchLastSibling = 1;
                pDisplayProgram->m_searchLastSibling = 1;
                pDisplayProgram->m_layerMatched = 0;
            }
        }
        if (pAddNode == NULL) {
            break;
        }

        pAddPrevious = pAddParent;
        pAddCandidate = pAddNode;
        pAddNode = NULL;
        while (pAddCandidate != NULL) {
            if (addSearchLastSibling) {
                if (addOnLastAddedPath) {
                    pAddCandidate = pDisplayProgram->m_levelLastAdded[addLevel];
                } else {
                    while (pAddCandidate->m_sibling != NULL) {
                        pAddCandidate = pAddCandidate->m_sibling;
                    }
                }
            }
            pDisplayProgram->m_addLevel = addLevel;

            // compare the candidate with the pending instruction
            unsigned char addEqual = 0;
            if (pAddCandidate->m_size == pAddInstruction->m_size) {
                unsigned int* pCandidateWords = &pAddCandidate->m_opcode;
                unsigned int* pInstructionWords = &pAddInstruction->m_opcode;
                // the original shifts the word count unsigned (lsr)
                int addWordCount = (pAddCandidate->m_size - (unsigned int)CDisplayProgram::Instruction::LevelNodeSize) >> 2;
                addEqual = 1;
                if (addWordCount == 2) {
                    if (pCandidateWords[0] != pInstructionWords[0] || pCandidateWords[1] != pInstructionWords[1]) {
                        addEqual = 0;
                    }
                } else if (addWordCount == 3) {
                    if (pCandidateWords[0] != pInstructionWords[0] || pCandidateWords[1] != pInstructionWords[1] ||
                        pCandidateWords[2] != pInstructionWords[2]) {
                        addEqual = 0;
                    }
                } else {
                    for (int word = 0; word < addWordCount; word++) {
                        if (pCandidateWords[word] != pInstructionWords[word]) {
                            addEqual = 0;
                            break;
                        }
                    }
                }
            }

            unsigned int addStateAction;
            if (addSortedByLayer) {
                addStateAction = pDisplayProgram->m_stateActionSorted;
            } else {
                addStateAction = pDisplayProgram->m_stateAction;
            }

            if (addEqual) {
                // equal: share it unless the layer / blending rules of the mode say otherwise
                addAction = AddAction_Share;
                if ((int)addLevel <= (int)(pDisplayProgram->m_layerLevel + (pDisplayProgram->m_blendBiased != 0))) {
                    // fx1: a switch (the original compares Layer first and falls into Blending)
                    switch (pAddCandidate->m_opcode) {
                    case CDisplayProgram::Instruction::Opcode::Blending:
                        if (pDisplayProgram->m_blendBiased && addStateAction == 0) {
                            if (pAddCandidate->m_sibling == NULL) {
                                pDisplayProgram->m_layerMatched = 1;
                                addLayerShared = 1;
                            } else {
                                addSearchLastSibling = 1;
                                pDisplayProgram->m_searchLastSibling = 1;
                                addAction = AddAction_LastSibling;
                            }
                        }
                        break;
                    case CDisplayProgram::Instruction::Opcode::Layer:
                        if (!pDisplayProgram->m_blendBiased && addStateAction == 0) {
                            pDisplayProgram->m_layerMatched = 1;
                            addLayerShared = 1;
                        }
                        break;
                    default:
                        if (pDisplayProgram->m_targetAction != 0 || pAddCandidate->m_sibling == NULL) {
                            if (addLevel == pDisplayProgram->m_layerLevel - 1) {
                                addSearchLastSibling = 0;
                                pDisplayProgram->m_searchLastSibling = 0;
                            }
                        } else {
                            addSearchLastSibling = 1;
                            pDisplayProgram->m_searchLastSibling = 1;
                            addAction = AddAction_LastSibling;
                        }
                        break;
                    }
                }
            } else if (addLevel == addLastLevel) {
                // the draw instruction
                unsigned int addDrawAction;
                if (addSortedByLayer) {
                    addDrawAction = pDisplayProgram->m_drawActionSorted;
                } else {
                    addDrawAction = pDisplayProgram->m_drawAction;
                }
                if (addDrawAction == 0) {
                    addAction = AddAction_NextSibling;
                    if (pAddCandidate->m_sibling != NULL) {
                        addAction = AddAction_LastSibling;
                    }
                } else if (addDrawAction == 2) {
                    addAction = AddAction_NextSibling;
                } else {
                    addAction = AddAction_Insert;
                }

                // a batch draw is merged into an equal batch that has room left
                if (addAction != AddAction_LastSibling &&
                    (pAddInstruction->m_opcode & CDisplayProgram::Instruction::Opcode::Flag_Batch) != 0) {
                    CDisplayProgram::Instruction::Draw* pDraw = (CDisplayProgram::Instruction::Draw*)pAddInstruction;
                    CDisplayProgram::Instruction::Draw* pBatch = (CDisplayProgram::Instruction::Draw*)pAddCandidate;
                    if (pDraw->m_vertexStream == NULL && pBatch->m_header.m_opcode == pDraw->m_header.m_opcode &&
                        pBatch->m_vertexFormat == pDraw->m_vertexFormat) {
                        ICBuffer::Stream* pVertexStream = pBatch->m_vertexStream;
                        unsigned int vertexFirst = pBatch->m_vertexCount;
                        if (pDraw->m_vertexCount <= pVertexStream->m_count - vertexFirst) {
                            if ((pDraw->m_header.m_opcode & CDisplayProgram::Instruction::Opcode::Flag_Indexed) == 0) {
                                unsigned int vertexTotal = pDraw->m_vertexCount + vertexFirst;
                                pBatch->m_vertexCount = vertexTotal;
                                pDraw->m_vertexCount = vertexTotal;
                                pDraw->m_vertexStream = pVertexStream;
                                addAction = AddAction_Merged;
                            } else if (pDraw->m_indexStream == NULL && pBatch->m_indexType == pDraw->m_indexType &&
                                       pDraw->m_indexCount <= pBatch->m_indexStream->m_count - pBatch->m_indexCount) {
                                unsigned int vertexTotal = pDraw->m_vertexCount + vertexFirst;
                                pBatch->m_vertexCount = vertexTotal;
                                pDraw->m_vertexCount = vertexTotal;
                                pDraw->m_vertexStream = pVertexStream;
                                unsigned int indexTotal = pDraw->m_indexCount + pBatch->m_indexCount;
                                pBatch->m_indexCount = indexTotal;
                                pDraw->m_indexCount = indexTotal;
                                pDraw->m_indexStream = pBatch->m_indexStream;
                                addAction = AddAction_Merged;
                            }
                        }
                    }
                }
            } else {
#include "CDisplayProgram_CompileState_GetAction.i"
            }

            switch (addAction) {
            case AddAction_Share:
                // go one level down below the shared candidate
                pAddNode = pAddCandidate->m_child;
                if (pAddCandidate->m_sibling != NULL) {
                    addOnLastAddedPath = 0;
                }
                if (pAddNode == NULL) {
                    pAddNode = (CDisplayProgram::Instruction::Header*)((unsigned char*)pAddCandidate + pAddCandidate->m_size);
                }
                addShareLayer = addLayerShared;
                pAddParent = pAddCandidate;
                pAddCandidate = NULL;
                break;
            case AddAction_Merged:
                goto addRestoreIgnoredLevels;
            case AddAction_Insert:
                pAddParent = pAddPrevious;
                addOnLastAddedPath = 0;
                goto addCopyChain;
            case AddAction_NextSibling:
                if (pAddCandidate->m_sibling == NULL) {
                    goto addCopyChain;
                }
                pAddPrevious = pAddCandidate;
                pAddCandidate = pAddCandidate->m_sibling;
                break;
            default:
                // AddAction_LastSibling: look at the candidate again (the last sibling this time)
                break;
            }
        }
    }

addCopyChain:
    {
        // ---- streams of a batch draw come from the program's shared buffers ----
        CDisplayProgram::Instruction::Draw* pDraw =
            (CDisplayProgram::Instruction::Draw*)pDisplayProgram->m_levelInstructions[addLastLevel];
        unsigned int drawOpcode = pDraw->m_header.m_opcode;
        if ((drawOpcode & CDisplayProgram::Instruction::Opcode::Flag_Batch) != 0) {
            ICBuffer::Stream** ppIndexStream = (ICBuffer::Stream**)&pDraw->m_vertexFormat;
            if (pDraw->m_vertexStream == NULL) {
                unsigned int vertexFormat = pDraw->m_vertexFormat;
                CDisplayProgram::BufferRef<ICVertexBuffer*>* pVertexRef = NULL;
                ICVertexBuffer* pVertexBuffer = NULL;
                ppIndexStream = &pDraw->m_indexStream;

                CDisplayProgram::Pool<CDisplayProgram::BufferRef<ICVertexBuffer*> >::Chunk* pVertexChunk =
                    pDisplayProgram->m_vertexBuffers.m_head;
                while (pVertexChunk != NULL && pVertexRef == NULL) {
                    for (int element = 0; element < pVertexChunk->m_elements.m_size; element++) {
                        CDisplayProgram::BufferRef<ICVertexBuffer*>* pRef = &pVertexChunk->m_elements.m_data[element];
                        if (pRef->m_key == vertexFormat &&
                            pDisplayProgram->m_vertexStreamLength <= pRef->m_capacity - pRef->m_used) {
                            pVertexRef = pRef;
                            pVertexBuffer = pRef->m_buffer;
                            break;
                        }
                    }
                    pVertexChunk = pVertexChunk->m_next;
                }
                if (pVertexRef == NULL) {
                    const CVertex::Attribute::Decl* pDecl;
                    pVertexBuffer = ICVertexBuffer::CreateInstance();
                    if (vertexFormat == VertexFormat_Vtx2d) {
                        pDecl = Vtx2d_AttrDecl;
                    } else if (vertexFormat == VertexFormat_Vtx3d) {
                        pDecl = Vtx3d_AttrDecl;
                    } else if (vertexFormat == VertexFormat_Vtx2d_t) {
                        pDecl = Vtx2d_t_AttrDecl;
                    } else if (vertexFormat == VertexFormat_Vtx3d_t) {
                        pDecl = Vtx3d_t_AttrDecl;
                    } else {
                        pDecl = NULL;
                    }
                    pVertexBuffer->Initialize(pDecl, pDisplayProgram->m_vertexBufferLength, 0);
                    CDisplayProgram::BufferRef<ICVertexBuffer*> vertexRef;
                    vertexRef.m_key = vertexFormat;
                    vertexRef.m_used = 0;
                    vertexRef.m_capacity = pDisplayProgram->m_vertexBufferLength;
                    vertexRef.m_buffer = pVertexBuffer;
                    pDisplayProgram->m_vertexBuffers.Add(vertexRef);
                    pVertexRef = &pDisplayProgram->m_vertexBuffers.GetLast();
                }
                ICBuffer::Stream vertexStream;
                vertexStream.m_buffer = pVertexBuffer;
                vertexStream.m_first = pVertexRef->m_used;
                vertexStream.m_count = pDisplayProgram->m_vertexStreamLength;
                vertexStream.m_dirty = 1;
                pDisplayProgram->m_vertexStreams.Add(vertexStream);
                pVertexRef->m_used = pDisplayProgram->m_vertexStreamLength + pVertexRef->m_used;
                pDraw->m_vertexStream = &pDisplayProgram->m_vertexStreams.GetLast();
            }

            if ((drawOpcode & CDisplayProgram::Instruction::Opcode::Flag_Indexed) != 0 && *ppIndexStream == NULL) {
                unsigned int indexType = ((unsigned int*)ppIndexStream)[1];
                CDisplayProgram::BufferRef<ICIndexBuffer*>* pIndexRef = NULL;
                ICIndexBuffer* pIndexBuffer = NULL;

                CDisplayProgram::Pool<CDisplayProgram::BufferRef<ICIndexBuffer*> >::Chunk* pIndexChunk =
                    pDisplayProgram->m_indexBuffers.m_head;
                while (pIndexChunk != NULL && pIndexRef == NULL) {
                    for (int element = 0; element < pIndexChunk->m_elements.m_size; element++) {
                        CDisplayProgram::BufferRef<ICIndexBuffer*>* pRef = &pIndexChunk->m_elements.m_data[element];
                        if (pRef->m_key == indexType &&
                            pDisplayProgram->m_indexStreamLength <= pRef->m_capacity - pRef->m_used) {
                            pIndexRef = pRef;
                            pIndexBuffer = pRef->m_buffer;
                            break;
                        }
                    }
                    pIndexChunk = pIndexChunk->m_next;
                }
                if (pIndexRef == NULL) {
                    pIndexBuffer = ICIndexBuffer::CreateInstance();
                    pIndexBuffer->Initialize(indexType, pDisplayProgram->m_indexBufferLength, 0);
                    CDisplayProgram::BufferRef<ICIndexBuffer*> indexRef;
                    indexRef.m_key = indexType;
                    indexRef.m_used = 0;
                    indexRef.m_capacity = pDisplayProgram->m_indexBufferLength;
                    indexRef.m_buffer = pIndexBuffer;
                    pDisplayProgram->m_indexBuffers.Add(indexRef);
                    pIndexRef = &pDisplayProgram->m_indexBuffers.GetLast();
                }
                ICBuffer::Stream indexStream;
                indexStream.m_buffer = pIndexBuffer;
                indexStream.m_first = pIndexRef->m_used;
                indexStream.m_count = pDisplayProgram->m_indexStreamLength;
                indexStream.m_dirty = 1;
                pDisplayProgram->m_indexStreams.Add(indexStream);
                pIndexRef->m_used = pDisplayProgram->m_indexStreamLength + pIndexRef->m_used;
                *ppIndexStream = &pDisplayProgram->m_indexStreams.GetLast();
            }
        }

        // ---- copy the rest of the chain into instruction memory ----
        unsigned int addSize = 0;
        for (addIndex = addLevel; addIndex < pDisplayProgram->m_levelCount; addIndex++) {
            addSize += ((CDisplayProgram::Instruction::Header*)pDisplayProgram->m_levelInstructions[addIndex])->m_size;
        }

        CDisplayProgram::Pool<CDisplayProgram::NodeChunk>::Chunk* pNodeChunk = pDisplayProgram->m_nodeChunk;
        CDisplayProgram::NodeChunk* pMemory = &pNodeChunk->m_elements.m_data[pDisplayProgram->m_nodeChunkIndex];
        if (pMemory->m_capacity - pMemory->m_used < addSize) {
            unsigned int chunkIndex = pDisplayProgram->m_nodeChunkIndex;
            if (chunkIndex < (unsigned int)pNodeChunk->m_elements.m_size - 1) {
                pDisplayProgram->m_nodeChunkIndex = chunkIndex + 1;
            } else if (chunkIndex < (unsigned int)pNodeChunk->m_elements.m_capacity - 1) {
                pDisplayProgram->m_nodeChunkIndex = chunkIndex + 1;
                CDisplayProgram::NodeChunk memory;
                memory.m_data = new unsigned int[pDisplayProgram->m_nodeChunkLength];
                memory.m_used = 0;
                memory.m_capacity = pDisplayProgram->m_nodeChunkLength;
                pDisplayProgram->m_nodeChunks.Add(memory);
                pNodeChunk = pDisplayProgram->m_nodeChunk;
            } else {
                pDisplayProgram->m_nodeChunkIndex = 0;
                pNodeChunk = pNodeChunk->m_next;
                if (pNodeChunk == NULL) {
                    CDisplayProgram::NodeChunk memory;
                    memory.m_data = new unsigned int[pDisplayProgram->m_nodeChunkLength];
                    memory.m_used = 0;
                    memory.m_capacity = pDisplayProgram->m_nodeChunkLength;
                    pDisplayProgram->m_nodeChunks.Add(memory);
                    pNodeChunk = pDisplayProgram->m_nodeChunk->m_next;
                }
                pDisplayProgram->m_nodeChunk = pNodeChunk;
            }
        }
        pMemory = &pNodeChunk->m_elements.m_data[pDisplayProgram->m_nodeChunkIndex];
        unsigned int* pDestination = pMemory->m_data + pMemory->m_used;
        pMemory->m_used = pMemory->m_used + (addSize >> 2);

        pAddWords = pDestination;
        for (addIndex = addLevel; addIndex < pDisplayProgram->m_levelCount; addIndex++) {
            unsigned int* pSource = (unsigned int*)pDisplayProgram->m_levelInstructions[addIndex];
            unsigned short instructionSize = ((CDisplayProgram::Instruction::Header*)pSource)->m_size;
            if (addOnLastAddedPath) {
                pDisplayProgram->m_levelLastAdded[addIndex] = (CDisplayProgram::Instruction::Header*)pAddWords;
            }
            if ((instructionSize >> 2) != 0) {
                unsigned int* pEnd = (unsigned int*)((unsigned char*)pAddWords + (instructionSize & 0xfffc));
                do {
                    *pAddWords = *pSource;
                    pAddWords++;
                    pSource++;
                } while (pAddWords < pEnd);
            }
        }

        // ---- link the copied chain in ----
        CDisplayProgram::Instruction::Header* pNew = (CDisplayProgram::Instruction::Header*)pDestination;
        if (addAction == AddAction_Insert) {
            if (pAddParent == NULL) {
                pNew->m_sibling = pDisplayProgram->m_root;
                pDisplayProgram->m_root = pNew;
            } else if (pAddParent->m_level == pNew->m_level) {
                pNew->m_sibling = pAddParent->m_sibling;
                pAddParent->m_sibling = pNew;
            } else {
                CDisplayProgram::Instruction::Header* pChild = pAddParent->m_child;
                if (pChild == NULL) {
                    pChild = (CDisplayProgram::Instruction::Header*)((unsigned char*)pAddParent + pAddParent->m_size);
                }
                pNew->m_sibling = pChild;
                pAddParent->m_child = pNew;
            }
        } else {
            pAddCandidate->m_sibling = pNew;
        }
    }

addRestoreIgnoredLevels:
    // ignored levels get their pending instruction back
    addLevelCount = pDisplayProgram->m_levelCount;
    for (addIndex = 0; addIndex < addLevelCount; addIndex++) {
        if (pDisplayProgram->m_levelIgnored[addIndex] != 0) {
            pDisplayProgram->m_levelInstructions[addIndex] = pDisplayProgram->m_levelSavedInstructions[addIndex];
            addLevelCount = pDisplayProgram->m_levelCount;
        }
    }
}
