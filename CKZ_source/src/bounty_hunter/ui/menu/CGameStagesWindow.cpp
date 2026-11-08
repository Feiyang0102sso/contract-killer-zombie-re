// j: original 156890 complete Stages, with only two resource child windows; unwritten tail fields are not additionally initialized.
#include "CGameStagesWindow.h"
#include "CGameResourceWindow.h"
#include "SScreenAdaptingConstant.h"

CGameStagesWindow::CGameStagesWindow()
    : m_field140(0), m_field144(0), m_field148(0), m_field14c(0),
      m_field15c(0), m_field160(0)
{
    SetArchetypeCharacter(1, 0);
    SetAnimation(52, 1);
    SetInsetSpacing(SScreenAdaptingConstant(5)());
    m_command = 0xd9305e1e;
    m_commandParameter1 = 0;
    m_commandParameter2 = 0;
    m_commandParameter3 = 0;
    m_flag150 = false;
    m_field154 = 0;
    m_field158 = -1;
    SScreenAdaptingConstant horizontal(5);
    SScreenAdaptingConstant vertical(10);
    CPlayerGameResourceWindow* provision = new CPlayerGameResourceWindow((CGameResourceWindow::EType)6, true);
    provision->SetCellPos(0, 0, 1, 1);
    int xSpacing = horizontal();
    int ySpacing = vertical();
    provision->SetOutsetSpacing(xSpacing, ySpacing);
    provision->ClearFlags(0x2000);
    AddToFront(provision);
    m_resources.addElement(provision);
    CPlayerGameResourceWindow* currency = new CPlayerGameResourceWindow((CGameResourceWindow::EType)4, true);
    currency->SetCellPos(1, 0, 1, 1);
    xSpacing = horizontal();
    ySpacing = vertical();
    currency->SetOutsetSpacing(xSpacing, ySpacing);
    currency->ClearFlags(0x2000);
    AddToFront(currency);
    m_resources.addElement(currency);
}

void CGameStagesWindow::OnLayout()
{
    // Original 156a20 is truly empty.
}

void CGameStagesWindow::Update()
{
    SGImageWindowCommand::Update();
}
