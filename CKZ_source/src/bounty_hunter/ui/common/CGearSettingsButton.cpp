// j: full original Gear; keeps the two float angles and the two independent screen-constant queries.
#include "CGearSettingsButton.h"
#include "SGImageWindow.h"
#include "SScreenAdaptingConstant.h"
#include "WindowApp.h"

CGearSettingsButton::CGearSettingsButton()
    : m_firstAngle(0.0f), m_secondAngle(0.0f), m_flagf4(false)
{
    m_firstGear = new SGImageWindowRotatable;
    m_firstGear->SetArchetypeCharacter(1, 0);
    m_firstGear->SetAnimation(54, 0);
    m_firstGear->SetLayoutType((ELayoutType)0);
    SGImageWindowRotatable* first = m_firstGear;
    first->SetCorner(SScreenAdaptingConstant(17)(), 0);
    AddToFront(m_firstGear);
    m_secondGear = new SGImageWindowRotatable;
    m_secondGear->SetArchetypeCharacter(1, 0);
    m_secondGear->SetAnimation(55, 0);
    m_secondGear->SetLayoutType((ELayoutType)0);
    SGImageWindowRotatable* second = m_secondGear;
    int secondX = SScreenAdaptingConstant(3)();
    int secondY = SScreenAdaptingConstant(14)();
    second->SetCorner(secondX, secondY);
    AddToFront(m_secondGear);
    SetDesiredWidth(SScreenAdaptingConstant(24)());
    SetDesiredHeight(SScreenAdaptingConstant(30)());
    int paddingX = SScreenAdaptingConstant(10)();
    int paddingY = SScreenAdaptingConstant(10)();
    m_hitPaddingX = paddingX;
    m_hitPaddingY = paddingY;
}

CGearSettingsButton::~CGearSettingsButton()
{
    // Both gears are owned by the Window tree.
}

void CGearSettingsButton::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0xed83def9u) {
        m_flagf4 = event.m_param1 != 0;
        event.Clear();
    }
}

void CGearSettingsButton::Update()
{
    if (m_pressed == m_flagf4) {
        m_firstAngle = 0.0f;
        m_secondAngle = 0.0f;
    } else {
        float duration = (float)WindowApp::m_instance->m_updateFrameTime * 0.2f;
        m_firstAngle = m_firstAngle + duration * 0.6666666865348815918f;
        m_secondAngle = m_secondAngle - duration;
    }
    m_firstGear->m_angle = (int)m_firstAngle;
    m_secondGear->m_angle = (int)m_secondAngle;
}

void CGearSettingsButton::Paint(com::glu::platform::graphics::ICGraphics2d&)
{
    // Original 148f3c is truly empty.
}
