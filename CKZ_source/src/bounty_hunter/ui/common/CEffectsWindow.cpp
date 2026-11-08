// j: original 1484a8..148818; keeps the original path that starts timing only after the animation finishes, and the timing of the random queries.
#include "CEffectsWindow.h"
#include "SGImageWindow.h"
#include "WindowApp.h"
#include "MathLib.h"

CEffectsWindow::CEffectsWindow(int archetype, int character, int width, int height)
    : Window(false), m_effectWidth(width), m_effectHeight(height),
      m_archetype(archetype), m_character(character)
{
    SetLayoutType((ELayoutType)0);
    ClearFlags(0x4000);
    ClearFlags(0x2000);
    SetSize(m_effectWidth, m_effectHeight);
}

void CEffectsWindow::CreateEffect(int animation, int periodMS, int x, int y, bool randomPosition)
{
    SGImageWindow* image = new SGImageWindow;
    image->SetArchetypeCharacter(m_archetype, m_character);
    image->SetAnimation(animation, 0);
    image->SetTranslateCoords(false);
    image->SetLayoutType((ELayoutType)0);
    image->SetDesiredWidth(m_effectWidth);
    image->SetDesiredHeight(m_effectHeight);
    image->ClearFlags(1);
    AddToFront(image);
    SEffect effect = { image, x, y, periodMS, 0, randomPosition };
    m_effects.addElement(effect);
}

void CEffectsWindow::StartEffect(SEffect& effect)
{
    int x;
    int y;
    if (effect.m_randomPosition) {
        x = WindowApp::m_instance->GetRandomValue().Int(m_effectWidth);
        y = WindowApp::m_instance->GetRandomValue().Int(m_effectHeight);
    } else {
        x = effect.m_x;
        y = effect.m_y;
    }
    int periodBeforeRandom = effect.m_periodMS;
    float random = WindowApp::m_instance->GetRandomValue().CFloat();
    effect.m_remainingMS = (int)((float)periodBeforeRandom + random * (float)(effect.m_periodMS / 5));
    effect.m_window->SetCorner(x, y);
    effect.m_window->SetRepeatCount(1, false);
    effect.m_window->SetFlags(1);
}

void CEffectsWindow::Update()
{
    for (int index = 0; index < m_effects.m_size; ++index) {
        // The original takes the current element's address before HasFinished and keeps accessing that address after the call.
        SEffect* effect = m_effects.m_items + index;
        if (effect->m_window->m_presenter.HasFinished()) {
            if (effect->m_remainingMS <= 0) {
                StartEffect(*effect);
            } else {
                effect->m_remainingMS -= WindowApp::m_instance->m_updateFrameTime;
            }
        }
    }
}
