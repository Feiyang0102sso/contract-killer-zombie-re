// b082: complete moving cloud, 0x14691c..0x146b9c.
#include "CGPSCloud.h"
#include "App.h"
#include "MathLib.h"

void CGPSCloud::Update(long elapsed)
{
    m_x += m_velocityX * (float)elapsed;
    m_y += m_velocityY * (float)elapsed;
    if (m_onScreen) {
        return;
    }
    if (m_x > (float)m_mapWidth) {
        m_x = -50.0f;
    }
    if (m_y > (float)m_mapHeight) {
        m_y = -50.0f;
    }
    if (m_x < -50.0f) {
        m_x = m_mapWidth;
    }
    if (m_y < -50.0f) {
        m_y = m_mapHeight;
    }
}

void CGPSCloud::Save(TiXmlElement*)
{
    // Original 1469bc is return-only.
}

void CGPSCloud::OnClick()
{
    // Original 1469c0 is return-only.
}

void CGPSCloud::Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y)
{
    if (m_visible) {
        int left = GetRect().m_x;
        int top = GetRect().m_y;
        if (m_cloudPresenter.IsViable()) {
            m_cloudPresenter.Draw(left + x, top + y);
        }
        CGPSItem::Paint(graphics, m_offsetX + x, m_offsetY + y);
    }
}

void CGPSCloud::Load(TiXmlElement*)
{
    // Original 146a34 is return-only.
}

CGPSCloud::CGPSCloud(float x, float y, float velocityX, float velocityY)
    : CGPSItem(), m_cloudPresenter()
{
    m_velocityX = velocityX;
    m_velocityY = velocityY;
    m_mapWidth = WindowApp::m_instance->m_screenWidth;
    m_mapHeight = WindowApp::m_instance->m_screenHeight;
    m_offsetX = 0;
    m_offsetY = 0;
    m_x = x;
    m_y = y;
    App::LoadSGArchetype(4, 0);
    m_presenter.SetArchetypeCharacter(4, 0);
    m_cloudPresenter.SetArchetypeCharacter(4, 0);
    int variant = WindowApp::m_instance->GetRandomValue().Int(3);
    m_cloudPresenter.SetAnimation(3 + variant * 2);
    SetSGImage(2 + variant * 2, -1);
}

void CGPSCloud::SetMapSize(int width, int height)
{
    m_mapWidth = width / App::Scale();
    m_mapHeight = height / App::Scale();
}

CGPSCloud::~CGPSCloud()
{
    // The third presenter is destroyed before the CGPSItem base.
}
