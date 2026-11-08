// CShaderProgram_OGLES.cpp - shader program of the OpenGL ES 1.x device.
#include "CShaderProgram_OGLES.h"
#include "CTypedVariableTable.h"
#include "CGraphics_OGLES.h"

namespace com { namespace glu { namespace platform { namespace graphics {

CShaderProgram_OGLES::~CShaderProgram_OGLES()
{
    CShaderProgram_OGLES::Destroy();
}

void CShaderProgram_OGLES::Destroy()
{
    if (m_parameterSemantics != NULL) {
        delete[] m_parameterSemantics;
        m_parameterSemantics = NULL;
    }
    CShaderProgram::Destroy();
}

unsigned char CShaderProgram_OGLES_ParameterTableSyncCallback(void* context, components::CTypedVariableTable* table)
{
    CShaderProgram_OGLES* program = (CShaderProgram_OGLES*)context;
    delete[] program->m_parameterSemantics;
    program->m_parameterSemantics = new unsigned int[program->m_parameterCount];
    for (unsigned int index = 0; index < program->m_parameterCount; index++) {
        // where the parameter lives in the table
        program->m_parameterEntries[index] = table->GetEntryIndex(program->m_parameterNames[index]);

        // the fixed function state it stands for
        if (program->m_parameterNames[index] == L"alphaTest") {
            program->m_parameterSemantics[index] = CShaderProgram_OGLES::ParameterSemantic_AlphaTest;
        } else if (program->m_parameterNames[index] == L"alphaRefVal") {
            program->m_parameterSemantics[index] = CShaderProgram_OGLES::ParameterSemantic_AlphaRefVal;
        } else if (program->m_parameterNames[index] == L"mvp") {
            program->m_parameterSemantics[index] = CShaderProgram_OGLES::ParameterSemantic_Mvp;
        } else if (program->m_parameterNames[index] == L"constColor") {
            program->m_parameterSemantics[index] = CShaderProgram_OGLES::ParameterSemantic_ConstColor;
        } else if (program->m_parameterNames[index] == L"texEnv0") {
            program->m_parameterSemantics[index] = CShaderProgram_OGLES::ParameterSemantic_TexEnv0;
        } else if (program->m_parameterNames[index] == L"texEnv1") {
            program->m_parameterSemantics[index] = CShaderProgram_OGLES::ParameterSemantic_TexEnv1;
        } else {
            program->m_parameterSemantics[index] = CShaderProgram_OGLES::ParameterSemantic_None;
        }
    }
    static_cast<CGraphics_OGLES*>(ICGraphics::GetInstance())->NotifyOfChangeInShaderProgramTableData(*program);
    return 1;
}

// sync handler callback of the parameter tables (returns 1 like the original)
typedef components::CTypedVariableTable::SyncHandler::Callback SyncCallback;

/** Unregister (context, callback) from `table` if it is registered. */
static inline void RemoveSyncHandler(components::CTypedVariableTable* table, void* context, SyncCallback callback)
{
    for (int index = 0; index < table->m_syncHandlers.m_size; index++) {
        components::CTypedVariableTable::SyncHandler& handler = table->m_syncHandlers.m_data[index];
        if (handler.m_context == context && handler.m_callback == callback) {
            table->m_syncHandlers.RemoveAt(index);
            return;
        }
    }
}

/** Register (context, callback) with `table` unless it is registered already. */
static inline void AddSyncHandler(components::CTypedVariableTable* table, void* context, SyncCallback callback)
{
    for (int index = 0; index < table->m_syncHandlers.m_size; index++) {
        components::CTypedVariableTable::SyncHandler& handler = table->m_syncHandlers.m_data[index];
        if (handler.m_context == context && handler.m_callback == callback) {
            return;
        }
    }
    components::CTypedVariableTable::SyncHandler handler(context, callback);
    table->m_syncHandlers.Add(handler);
}

void CShaderProgram_OGLES::SetParameterTable(ParameterTable& table)
{
    if (m_parameterTable != NULL) {
        RemoveSyncHandler(m_parameterTable, this, CShaderProgram_OGLES_ParameterTableSyncCallback);
    }
    CShaderProgram_OGLES_ParameterTableSyncCallback(this, &table);
    AddSyncHandler(&table, this, CShaderProgram_OGLES_ParameterTableSyncCallback);
    m_parameterTable = &table;
}

void CShaderProgram_OGLES::FinalizeInitialization(components::CInputStream** streams, unsigned int* types)
{
    m_finalized = 1;
    // the source shaders are only parsed, no shader objects are made for them
    for (unsigned int index = 0; index < m_sourceShaderCount; index++) {
        m_shaders[index] = NULL;
    }
    ParseCommon(streams, types);
}

} } } }
