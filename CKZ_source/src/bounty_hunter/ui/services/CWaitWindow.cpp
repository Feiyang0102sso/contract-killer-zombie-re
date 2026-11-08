// b081j: keeps the original unused uptime query, the delay field, and the uninitialized percent.
#include "CWaitWindow.h"
#include "App.h"
#include "DGHelper.h"
#include "CDrawUtil.h"
#include "SScreenAdaptingConstant.h"
#include "CStdUtil_Cocoa.h"
#include "MathLib.h"

CWaitWindow::CWaitWindow() : Window(false), m_angleStep(45), m_fillColor(0)
{
    CStdUtil_Cocoa::GetUpTimeMS();
    for (int delay = 0; delay <= 875; delay += 125) {
        CDot* dot = new CDot;
        dot->m_radius = SScreenAdaptingConstant(1, 2)();
        dot->m_durationMS = 1000;
        dot->m_delayMS = delay;
        dot->m_elapsedMS = 0;
        m_dots.addElement(dot);
    }
}

CWaitWindow::~CWaitWindow()
{
    SAFE_DELETE_VECTOR(m_dots);
}

void CWaitWindow::Update()
{
    if (m_dots.m_size < 1) {
        return;
    }
    int elapsedMS = WindowApp::m_instance->m_updateFrameTime;
    for (int index = 0; index < m_dots.m_size; ++index) {
        m_dots.m_items[index]->Update(elapsedMS);
    }
}

void CWaitWindow::CDot::Update(int elapsedMS)
{
    m_elapsedMS += elapsedMS;
    m_percent = (float)m_elapsedMS / (float)m_durationMS;
    if (m_percent > 2.0f) {
        m_elapsedMS -= m_durationMS;
        m_percent = 0.0f;
    } else if (m_percent > 1.0f) {
        m_percent = 1.0f - (m_percent - 1.0f);
    }
}

void CWaitWindow::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    if (m_fillColor != 0) {
        CDrawUtil::Fill(m_x, m_y, m_width, m_height, m_fillColor,
                       com::glu::platform::graphics::ICGraphics2d::Primitive_Rectangles);
    }
    int angle = 0;
    for (int index = 0; index < m_dots.m_size; ++index) {
        int width = m_width;
        int height = m_height;
        int distance = SScreenAdaptingConstant(10, 20)();
        CDot* dot = m_dots.m_items[index];
        float cosine = MathLib::Cos((float)angle);
        float sine = MathLib::Sin((float)angle);
        int alpha = (int)(dot->m_percent * 255.0f);
        int radius = dot->m_radius;
        int x = (int)((float)(width / 2) + cosine * (float)distance);
        int y = (int)((float)(height / 2) - (float)distance * sine);
        unsigned int color = alpha | (alpha << 16) | (alpha << 8) | 0xff000000;
        ((App*)WindowApp::m_instance)->m_helper->DrawCircle(x, y, radius, color, true);
        angle += m_angleStep;
    }
}
