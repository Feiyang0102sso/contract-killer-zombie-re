// CGraphics.cpp - device independent part of the graphics devices and the device resources.
#include "CGraphics.h"
#include "ICRasterizerState.h"
#include "ICShader.h"
#include "ICShaderProgram.h"
#include "CRectangle.h"
#include "CEventListener.h"
#include "CRegistryItr.h"
#include "CHandle.h"
#include "CApplet.h"
#include "ICCore.h"
#include "CMathFixed.h"
#include "CVector2dx.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// 1.0 in 16.16 fixed point
static const int Graphics_FixedOne = 0x10000;

// events the device listens to, and their priorities
static const unsigned int Graphics_Event_747ab255 = 0x747ab255;
static const unsigned int Graphics_Event_c0d486fe = 0xc0d486fe;
static const unsigned int Graphics_Event_16625f9a = 0x16625f9a;
static const unsigned int Graphics_Event_97b4dec3 = 0x97b4dec3;
static const unsigned int Graphics_Event_7b7983ee = 0x7b7983ee;
static const unsigned int Graphics_Event_37f8a76d = 0x37f8a76d;
static const int Graphics_EventPriority_Early = 0x73333300;

// opcodes of the clear table entries start here
static const unsigned int Graphics_ClearEntryOpcodeBase = 1000;

/** 16.16 fixed point quotient. */
static inline int Graphics_FixedDivide(int numerator, int denominator)
{
    return (int)(((long long)numerator << math::CMathFixed::FBits) / denominator);
}

ICGraphicsResource::ICGraphicsResource(unsigned int resourceType)
{
    m_resourceType = resourceType;
    static_cast<CGraphics*>(ICGraphics::GetInstance())->AddToResourceRegistry(*this);
}

unsigned char CGraphics::AddToResourceRegistry(ICGraphicsResource& resource)
{
    systems::CSystemElement* element;
    if (GetResourceRegistry().Query(resource.m_resourceType, (unsigned int)&resource, &element)) {
        return 0;
    }
    GetResourceRegistry().Add(*GetResourceRegistry().CreateSystemElement((unsigned int)&resource, &resource,
                                                                         resource.m_resourceType,
                                                                         core::CPriorityClass::DefaultPriority));
    return 1;
}

ICGraphicsResource::~ICGraphicsResource()
{
    static_cast<CGraphics*>(ICGraphics::GetInstance())->RemoveFromResourceRegistry(*this);
}

unsigned char CGraphics::RemoveFromResourceRegistry(ICGraphicsResource& resource)
{
    systems::CSystemElement* element;
    if (!GetResourceRegistry().Query(resource.m_resourceType, (unsigned int)&resource, &element)) {
        return 0;
    }
    if (resource.m_resourceType == ICRenderSurface::ResourceType) {
        // texture units must not keep a surface that goes away
        for (int unit = 0; unit < TextureUnitCount; unit++) {
            if (m_textureUnits[unit].m_surface == element->m_object) {
                m_textureUnits[unit].m_surface = NULL;
                m_textureUnits[unit].m_level = 0;
                m_textureUnits[unit].m_dirty = 1;
            }
        }
    }
    if (!GetResourceRegistry().Remove(*element)) {
        return 0;
    }
    if (element != NULL) {
        delete element;
    }
    return 1;
}

CGraphics::DebugMessagingBridge::~DebugMessagingBridge()
{
    for (int index = 0; index < m_listeners.m_size; index++) {
        m_listeners.m_data[index]->UnregisterAll();
        if (m_listeners.m_data[index] != NULL) {
            delete m_listeners.m_data[index];
        }
    }
}

CGraphics::CGraphics(EventHandler eventHandler) : m_intrinsicShaderPrograms(10, 10)
{
    m_eventHandler = eventHandler;
    m_targetLevel = NULL;
    m_clearLevel = NULL;
    m_viewportLevel = NULL;
    m_shadersLevel = NULL;
    m_texturesLevel = NULL;
    m_shaderParamsLevel = NULL;
    m_shaderProgram = NULL;
    m_displayProgram = NULL;
    m_runOnEnd = 0;
    m_deviceInitialized = 0;
    m_state212 = 0;
    m_initialized = 0;
    m_intrinsicShadersInitialized = 0;
    m_eventListener = NULL;
    m_width = 0;
    m_initializeFailed = 0;
    m_backgroundColor = components::Color_A8R8G8B8_t::Black;
    m_displayScaleX = Graphics_FixedOne;
    m_displayScaleY = Graphics_FixedOne;
    m_state239 = 0;
    m_windowSurface = NULL;
    m_targetSurface = NULL;
    m_defaultDisplayProgram = NULL;
    m_targetInstruction = NULL;
    m_texturesInstruction = NULL;
    m_shadersInstruction = NULL;
    m_viewportInstruction = NULL;
    m_debugMessagingBridge = NULL;
    m_coreInitialized = 0;
}

CGraphics::~CGraphics()
{
    DestroyCore();
    m_eventHandler = NULL;
}

void CGraphics::DestroyCore()
{
    if (m_defaultDisplayProgram != NULL) {
        delete m_defaultDisplayProgram;
        m_defaultDisplayProgram = NULL;
    }
    if (m_texturesInstruction != NULL) {
        delete m_texturesInstruction;
        m_texturesInstruction = NULL;
    }
    if (m_shadersInstruction != NULL) {
        delete m_shadersInstruction;
        m_shadersInstruction = NULL;
    }
    if (m_viewportInstruction != NULL) {
        delete m_viewportInstruction;
        m_viewportInstruction = NULL;
    }
    if (m_targetInstruction != NULL) {
        delete m_targetInstruction;
        m_targetInstruction = NULL;
    }
    if (m_rasterizerState != NULL) {
        delete m_rasterizerState;
        m_rasterizerState = NULL;
    }
    if (m_eventListener != NULL) {
        m_eventListener->UnregisterAll();
        delete m_eventListener;
        m_eventListener = NULL;
    }
    if (m_debugMessagingBridge != NULL) {
        delete m_debugMessagingBridge;
        m_debugMessagingBridge = NULL;
    }

    m_targetLevel = NULL;
    m_clearLevel = NULL;
    m_viewportLevel = NULL;
    m_shadersLevel = NULL;
    m_texturesLevel = NULL;
    m_shaderParamsLevel = NULL;
    m_shaderProgram = NULL;
    m_displayProgram = NULL;
    m_defaultDisplayProgram = NULL;
    m_runOnEnd = 0;
    m_deviceInitialized = 0;
    m_state212 = 0;
    m_initialized = 0;
    m_intrinsicShadersInitialized = 0;
    m_width = 0;
    m_initializeFailed = 0;
    m_backgroundColor = components::Color_A8R8G8B8_t::Black;
    m_displayScaleX = Graphics_FixedOne;
    m_displayScaleY = Graphics_FixedOne;
    m_state239 = 0;
    m_windowSurface = NULL;
    m_targetSurface = NULL;
    m_coreInitialized = 0;
}

void CGraphics::InitializeCore()
{
    if (m_coreInitialized) {
        return;
    }

    m_debugMessagingBridge = new DebugMessagingBridge;

    m_eventListener = new systems::CEventListener;
    m_eventListener->m_object = this;
    m_eventListener->m_handle = core::CHandle::FromName("Graphics");
    m_eventListener->m_callback = m_eventHandler;
    m_eventListener->Register(Graphics_Event_747ab255, Graphics_EventPriority_Early);
    m_eventListener->Register(Graphics_Event_c0d486fe, Graphics_EventPriority_Early);
    m_eventListener->Register(Graphics_Event_16625f9a, Graphics_EventPriority_Early);
    m_eventListener->Register(Graphics_Event_97b4dec3, Graphics_EventPriority_Early);
    m_eventListener->Register(Graphics_Event_7b7983ee, core::CPriorityClass::DefaultPriority);
    m_eventListener->Register(Graphics_Event_37f8a76d, core::CPriorityClass::DefaultPriority);

    m_rasterizerState = NULL;

    TargetInstruction* target = new TargetInstruction;
    m_targetInstruction = target;
    target->m_header.m_child = NULL;
    target->m_header.m_sibling = NULL;
    target->m_header.m_size = sizeof(TargetInstruction);
    target->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::GetIntrinsicOpcode(L"Target");

    ViewportInstruction* viewport = new ViewportInstruction;
    m_viewportInstruction = viewport;
    viewport->m_header.m_child = NULL;
    viewport->m_header.m_sibling = NULL;
    viewport->m_header.m_size = sizeof(ViewportInstruction);
    viewport->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::GetIntrinsicOpcode(L"Viewport");

    // the clear table doubles as the "ClearBuffers" instruction
    m_clearTable.Initialize(10);
    m_clearTable.Add(L"Stamp", components::CTypedVariableTable::Entry::Integer, 1, 1);
    m_clearTable.Add(L"ScissorTest", components::CTypedVariableTable::Entry::Boolean, 1, 1);
    m_clearTable.Add(L"Scissor", components::CTypedVariableTable::Entry::Integer4d, 1, 1);
    m_clearTable.Add(L"Flags", components::CTypedVariableTable::Entry::Integer, 1, 1);
    m_clearTable.Add(L"Color", components::CTypedVariableTable::Entry::Vector4d, 1, 1);
    m_clearTable.Add(L"ColorMask", components::CTypedVariableTable::Entry::Integer, 1, 1);
    m_clearTable.Add(L"Depth", components::CTypedVariableTable::Entry::Float, 1, 1);
    m_clearTable.Add(L"Stencil", components::CTypedVariableTable::Entry::Integer, 1, 1);
    CDisplayProgram::Instruction::Header* header = (CDisplayProgram::Instruction::Header*)m_clearTable.GetTableHeader();
    header->m_child = NULL;
    header->m_sibling = NULL;
    header->m_size = (unsigned short)m_clearTable.GetTableSize();
    header->m_opcode = CDisplayProgram::Instruction::Opcode::GetIntrinsicOpcode(L"ClearBuffers");
    for (unsigned int index = 0; index < m_clearTable.GetNumberOfEntries(); index++) {
        header = (CDisplayProgram::Instruction::Header*)m_clearTable.GetEntryHeader(index);
        header->m_child = NULL;
        header->m_sibling = NULL;
        header->m_size = (unsigned short)(m_clearTable.GetEntryDataSize(index) + m_clearTable.m_entryHeaderSize);
        header->m_opcode = Graphics_ClearEntryOpcodeBase + index;
    }

    ShadersInstruction* shaders = new ShadersInstruction;
    m_shadersInstruction = shaders;
    shaders->m_header.m_child = NULL;
    shaders->m_header.m_sibling = NULL;
    shaders->m_header.m_size = sizeof(ShadersInstruction);
    shaders->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::GetIntrinsicOpcode(L"Shaders");
    shaders->m_program = NULL;

    TexturesInstruction* textures = new TexturesInstruction;
    m_texturesInstruction = textures;
    textures->m_header.m_child = NULL;
    textures->m_header.m_sibling = NULL;
    textures->m_header.m_size = sizeof(TexturesInstruction);
    textures->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::GetIntrinsicOpcode(L"Textures");
    textures->m_count = 0;

    // (the original bounds this loop by the clear table size)
    for (unsigned int unit = 0; unit < m_clearTable.GetNumberOfEntries(); unit++) {
        m_textureSampleHints[unit] = TextureSampleHint_Default;
    }
    for (int unit = 0; unit < TextureUnitCount; unit++) {
        m_textureUnits[unit].m_surface = NULL;
        m_textureUnits[unit].m_level = 0;
    }
    m_coreInitialized = 1;
}

void CGraphics::OrientationReinterpretationYFlipTransform(components::CRectangle<short>& rectangle,
                                                          components::CRectangle<short>& transformed)
{
    unsigned int width;
    unsigned int height;
    m_targetSurface->GetWidthAndHeight(width, height);

    components::CRectangle<short> clipped;
    clipped.m_x = 0;
    clipped.m_y = 0;
    clipped.m_width = (short)width;
    clipped.m_height = (short)height;
    clipped.Clip(rectangle);

    framework::ICCore* core = CApplet::m_pCore;
    if ((m_targetSurface->m_flags & ICRenderSurface::Flag_Targetable) == 0 || !core->m_deviceOrientationSet) {
        transformed.m_x = clipped.m_x;
        transformed.m_y = (short)(height - clipped.m_y - clipped.m_height);
        transformed.m_width = clipped.m_width;
        transformed.m_height = clipped.m_height;
        return;
    }

    switch (core->m_deviceOrientation) {
    case framework::ICCore::DeviceOrientation_Portrait:
        transformed.m_x = clipped.m_x;
        transformed.m_y = (short)(height - clipped.m_y - clipped.m_height);
        transformed.m_width = clipped.m_width;
        transformed.m_height = clipped.m_height;
        break;
    case framework::ICCore::DeviceOrientation_LandscapeRight:
        transformed.m_x = clipped.m_y;
        transformed.m_y = clipped.m_x;
        transformed.m_width = clipped.m_height;
        transformed.m_height = clipped.m_width;
        break;
    case framework::ICCore::DeviceOrientation_PortraitUpsideDown:
        transformed.m_x = (short)(width - clipped.m_x - clipped.m_width);
        transformed.m_y = clipped.m_y;
        transformed.m_width = clipped.m_width;
        transformed.m_height = clipped.m_height;
        break;
    case framework::ICCore::DeviceOrientation_LandscapeLeft:
        transformed.m_x = (short)(height - clipped.m_y - clipped.m_height);
        transformed.m_y = (short)(width - clipped.m_x - clipped.m_width);
        transformed.m_width = clipped.m_height;
        transformed.m_height = clipped.m_width;
        break;
    }
}

unsigned char CGraphics::CreateAndInitializeWindowSurface(ICRenderSurface::Targetability targetability)
{
    m_windowSurface = ICRenderSurface::CreateInstance(ICRenderSurface::Type_Window, targetability);

    framework::ICCore* core = CApplet::m_pCore;
    core->m_windowToDisplayScale.x = m_displayScaleX;
    core->m_windowToDisplayScale.y = m_displayScaleY;
    core->m_displayToWindowScale.y = Graphics_FixedDivide(Graphics_FixedOne, m_displayScaleY);
    core->m_displayToWindowScale.x = Graphics_FixedDivide(Graphics_FixedOne, m_displayScaleX);

    unsigned char initialized = 0;
    if (m_windowSurface != NULL) {
        initialized = InitializeWindowSurface(*m_windowSurface);
    }
    return initialized;
}

unsigned char CGraphics::SaveStateAndReleaseDevice()
{
    // targetable surfaces are saved last
    core::CVector<ICRenderSurface*> targetSurfaces(10, 0);
    unsigned char saved = 1;
    for (systems::CRegistryItr itr(GetResourceRegistry()); !itr.IsDone(); itr++) {
        ICGraphicsResource* resource = (ICGraphicsResource*)(*itr).m_object;
        if (resource->m_resourceType == ICRenderSurface::ResourceType &&
            (static_cast<ICRenderSurface*>(resource)->m_flags & ICRenderSurface::Flag_Targetable) != 0) {
            ICRenderSurface* surface = static_cast<ICRenderSurface*>(resource);
            targetSurfaces.Add(surface);
        } else if (!SaveResourceStateForFutureRecreation(resource)) {
            saved = 0;
        }
    }
    for (int index = 0; index < targetSurfaces.Size(); index++) {
        SaveResourceStateForFutureRecreation(targetSurfaces[index]);
    }
    if (m_windowSurface != NULL) {
        m_windowSurface->Destroy();
    }
    return saved;
}

unsigned char CGraphics::SaveResourceStateForFutureRecreation(ICGraphicsResource* resource)
{
    unsigned int resourceType = resource->m_resourceType;
    // b117-va: a 3-case switch; the original compares Shader, ShaderProgram, RenderSurface
    // and lays the blocks out as RenderSurface, Shader, ShaderProgram (= case order)
    switch (resourceType) {
    case ICRenderSurface::ResourceType:
        return static_cast<ICRenderSurface*>(resource)->SaveStateForFutureRecreation();
    case ICShader::ResourceType:
        return static_cast<ICShader*>(resource)->SaveStateForFutureRecreation();
    case ICShaderProgram::ResourceType:
        return static_cast<ICShaderProgram*>(resource)->SaveStateForFutureRecreation();
    }
    return 0;
}

unsigned char CGraphics::Recreate()
{
    unsigned char recreated = 0;
    if (m_windowSurface != NULL) {
        recreated = InitializeWindowSurface(*m_windowSurface);
    }
    Sync();
    if (recreated) {
        for (systems::CRegistryItr itr(GetResourceRegistry()); !itr.IsDone(); itr++) {
            if (!RecreateResource((ICGraphicsResource*)(*itr).m_object)) {
                recreated = 0;
            }
        }
    }
    Sync();
    return recreated;
}

unsigned char CGraphics::RecreateResource(ICGraphicsResource* resource)
{
    unsigned int resourceType = resource->m_resourceType;
    // b117-va: a 3-case switch; the original compares Shader, ShaderProgram, RenderSurface
    // and lays the blocks out as RenderSurface, Shader, ShaderProgram (= case order)
    switch (resourceType) {
    case ICRenderSurface::ResourceType:
        return static_cast<ICRenderSurface*>(resource)->Recreate();
    case ICShader::ResourceType:
        return static_cast<ICShader*>(resource)->Recreate();
    case ICShaderProgram::ResourceType:
        return static_cast<ICShaderProgram*>(resource)->Recreate();
    }
    return 0;
}

} } } }
