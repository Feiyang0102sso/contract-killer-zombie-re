// SG_Presenter.cpp - animation playback and frame traversal of one character (see SG_Presenter.h).
// (written by b042)
#include "SG_Presenter.h"
#include "SG_Home.h"
#include "SG_SpecialImpl.h"
#include "CMathFixed.h"
#include "ICGraphics.h"
#include "ICRenderSurface.h"

using com::glu::platform::components::CRectangle;
using com::glu::platform::graphics::ICGraphics;
using com::glu::platform::graphics::ICRenderSurface;
using com::glu::platform::math::CMathFixed;

// ---- constants ----
static const unsigned short DEFAULT_SCALE_PERCENT = 100;
static const int FIXED_ONE = 0x10000;               // 1.0 in 16.16 fixed point
static const int LAYERS_PER_FRAME = 1;              // every frame shows exactly one layer

SG_Presenter::SG_Presenter()
{
    Init();
    m_special = new SG_SpecialImpl();
}

void SG_Presenter::Init()
{
    m_instance = SG_Home::GetInstance()->GetDefaultSGInstance();
    m_archetype = 0;
    m_character = 0;
    m_animation = 0;
    m_transform = 0;
    m_collisionSpritesOnly = 0;
    m_frameDurations = NULL;
    m_unused28 = 0;
    m_loop = 0;
    m_direction = 0;
    m_specialFlags = 0;
    m_y = 0;
    m_x = 0;
    m_time = 0;
    m_pole0 = 0;
    m_pole1 = 0;
    m_frameCount = 0;
    m_frame = 0;
    m_totalDuration = 0;
    m_scalePercent = DEFAULT_SCALE_PERCENT;
    m_scale = FIXED_ONE;
    m_isScaled = 0;
}

void SG_Presenter::SetArchetypeCharacter(int archetype, int character)
{
    m_archetype = archetype;
    m_character = character;
}

SG_Presenter::~SG_Presenter()
{
    if (m_special != NULL) {
        delete m_special;
        m_special = NULL;
    }
}

unsigned char SG_Presenter::IsViable()
{
    if (!m_instance->IsInitialized()) {
        return 0;
    }
    if (!m_instance->IsLoadedArchetypeCharacter(m_archetype, m_character)) {
        return 0;
    }
    return m_frameDurations != NULL;
}

void SG_Presenter::SetPosition(short x, short y)
{
    m_x = x;
    m_y = y;
}

void SG_Presenter::SetAnimation(int animation)
{
    SetAnimation(animation, m_loop, m_transform);
}

void SG_Presenter::SetAnimation(int animation, unsigned char loop)
{
    SetAnimation(animation, loop, m_transform);
}

void SG_Presenter::SetAnimation(int animation, unsigned char loop, unsigned char transform)
{
    SG_Animation& selected = m_instance->m_archetypes[m_archetype]->m_animations[animation];
    m_animation = animation;
    m_frameDurations = selected.m_frameDurations;
    m_frameCount = selected.m_frameCount;
    m_totalDuration = 0;
    for (int frame = 0; frame < m_frameCount; frame++) {
        m_totalDuration += m_frameDurations[frame];
    }
    m_loop = loop;
    m_frame = 0;
    m_pole0 = 0;
    m_pole1 = m_frameDurations[0];
    m_time = 0;
    m_direction = 1;
    m_transform = transform;
}

void SG_Presenter::setScale(unsigned short percent)
{
    m_scalePercent = m_instance->m_archetypes[m_archetype]->m_characters[m_character].GetScaleFactorForCharacter(percent);
    m_isScaled = m_scalePercent != DEFAULT_SCALE_PERCENT;
    if (m_isScaled) {
        m_scale = CMathFixed::Div(m_scalePercent << 16, DEFAULT_SCALE_PERCENT << 16);
    } else {
        m_scale = FIXED_ONE;
    }
}

unsigned char SG_Presenter::Update(int elapsed)
{
    if (HasFinished()) {
        return 0;
    }
    if (elapsed == 0) {
        return 0;
    }
    m_time += m_direction * elapsed;
    // step over every frame boundary (m_pole1) the time has passed
    while ((m_direction >= 1 && m_time >= m_pole1) || (m_direction <= -1 && m_time <= m_pole1)) {
        if (m_frame != GetLastFrame()) {
            unsigned short pole = m_pole1;
            m_frame += m_direction;
            m_pole0 = pole;
            m_pole1 = pole + m_direction * m_frameDurations[m_frame];
        } else if (m_loop) {
            m_time -= m_direction * m_totalDuration;
            ResetPoles(m_direction);
        } else {
            if (m_direction >= 1) {
                m_time = m_totalDuration;
            } else {
                m_time = 0;
            }
            return 1;
        }
    }
    return 0;
}

unsigned char SG_Presenter::HasFinished()
{
    if (m_loop) {
        return 0;
    }
    if (m_frame != GetLastFrame()) {
        return 0;
    }
    return m_time == m_pole1;
}

void SG_Presenter::ResetPoles(int direction)
{
    if (direction == 1) {
        m_frame = 0;
        m_pole0 = 0;
        m_pole1 = m_frameDurations[0];
    } else {
        m_frame = m_frameCount - 1;
        m_pole0 = m_totalDuration;
        m_pole1 = m_totalDuration - m_frameDurations[m_frameCount - 1];
    }
}

void SG_Presenter::Draw()
{
    Draw(0, 0);
}

void SG_Presenter::Draw(int x, int y)
{
    ICRenderSurface* target = ICGraphics::GetInstance()->GetTargetSurface();
    unsigned int width;
    unsigned int height;
    target->GetWidthAndHeight(width, height);
    short clipHeight = height;
    short clipWidth = width;
    CRectangle<short> clip;
    clip.m_x = 0;
    clip.m_y = 0;
    clip.m_width = clipWidth;
    clip.m_height = clipHeight;
    m_instance->drawInit(m_x + x, m_y + y, clip);
    TraverseCurrentFrame(SG_TraverseMode_Draw);
    m_instance->drawResolve(*this);
}

void SG_Presenter::TraverseCurrentFrame(SG_TraverseMode mode)
{
    SG_Archetype* archetype = m_instance->m_archetypes[m_archetype];
    int frame = archetype->m_animations[m_animation].m_frameLayers[m_frame];
    if ((m_specialFlags & SpecialFlag_FramePre) && m_special != NULL) {
        if (m_special->DrawSpecialFramePre(mode, *this, frame)) {
            return;
        }
    }

    for (int layerIndex = LAYERS_PER_FRAME - 1; layerIndex >= 0; layerIndex--) {
        int layer = frame;
        SG_Layer* layers = archetype->m_layers;
        if ((m_specialFlags & SpecialFlag_LayerPre) && m_special != NULL) {
            if (m_special->DrawSpecialLayerPre(mode, *this, frame, layer)) {
                continue;
            }
        }

        // entries from the last to the first
        SG_Layer& layerData = layers[layer];
        for (int entryIndex = layerData.m_count - 1; entryIndex >= 0; entryIndex--) {
            SG_LayerEntry& entry = layerData.m_entries[entryIndex];
            unsigned int infoCount = 1;
            SpriteDrawInfo* infos = NULL;
            m_instance->computeSpriteDrawInfos(infos, infoCount, m_archetype, m_character, entry.m_sprite);
            for (unsigned int infoIndex = 0; infoIndex < infoCount; infoIndex++) {
                SpriteDrawInfo info = infos[infoIndex];
                short entryX = entry.m_x;
                short entryY = entry.m_y;
                int sprite = info.m_sprite;
                int width;
                int height;
                m_instance->GetSize(sprite, width, height);

                // apply the character transform
                int x = info.m_x + entryX;
                if (m_transform & SquareTransform_MirrorHorizontal) {
                    x = -(x + width);
                }
                int y = info.m_y + entryY;
                if (m_transform & SquareTransform_MirrorVertical) {
                    y = -(height + y);
                }
                int drawX;
                int drawY;
                if (m_transform & SquareTransform_Transposed) {
                    drawX = y;
                    drawY = x;
                    int swap = width;
                    width = height;
                    height = swap;
                } else {
                    drawX = x;
                    drawY = y;
                }
                if (m_isScaled) {
                    width = m_scalePercent * width / 100;
                    height = m_scalePercent * height / 100;
                    drawY = m_scalePercent * drawY / 100;
                    drawX = m_scalePercent * drawX / 100;
                }

                if ((m_specialFlags & SpecialFlag_SpritePre) && m_special != NULL) {
                    if (m_special->DrawSpecialSpritePre(mode, *this, frame, layer, sprite, drawX, drawY)) {
                        continue;
                    }
                }

                switch (mode) {
                case SG_TraverseMode_Draw:
                    m_instance->drawAccumulate(drawX, drawY, width, height, sprite, m_transform, m_scalePercent,
                                               m_scale, entry.m_rotation, entry.m_scaleX, entry.m_scaleY);
                    break;
                case SG_TraverseMode_Collide:
                    if ((m_collisionSpritesOnly && !m_instance->IsCollisionSprite(sprite)) ||
                        m_instance->m_collisionTag != -1) {
                        if (m_instance->m_collisionTag != m_instance->GetTag(sprite)) {
                            break;
                        }
                    }
                    m_instance->collisionAccumulate(drawX, drawY, width, height, sprite, entry.m_rotation,
                                                    entry.m_scaleX, entry.m_scaleY);
                    break;
                case SG_TraverseMode_CollisionInit:
                    if ((m_collisionSpritesOnly && !m_instance->IsCollisionSprite(sprite)) ||
                        m_instance->m_collisionTag != -1) {
                        if (m_instance->m_collisionTag != m_instance->GetTag(sprite)) {
                            break;
                        }
                    }
                    // the first matching sprite is the collision rectangle
                    m_instance->collisionInit(drawX, drawY, width, height, m_instance->GetTag(sprite), sprite);
                    return;
                default:
                    m_instance->boundsAccumulate(drawX, drawY, width, height, entry.m_rotation, entry.m_scaleX,
                                                 entry.m_scaleY);
                    break;
                }

                if (m_special != NULL && (m_specialFlags & SpecialFlag_SpritePost)) {
                    m_special->DrawSpecialSpritePost(mode, *this, frame, layer, sprite, drawX, drawY);
                }
            }
        }

        if (m_special != NULL && (m_specialFlags & SpecialFlag_LayerPost)) {
            m_special->DrawSpecialLayerPost(mode, *this, frame, layer);
        }
    }

    if (m_special != NULL && (m_specialFlags & SpecialFlag_FramePost)) {
        m_special->DrawSpecialFramePost(mode, *this, frame);
    }
}

void SG_Presenter::Bounds(CRectangle<short>& bounds)
{
    m_instance->boundsInit();
    TraverseCurrentFrame(SG_TraverseMode_Bounds);
    m_instance->boundsResolve(bounds, *this);
}

void SG_Presenter::Reset()
{
    int direction = m_direction;
    ResetPoles(direction);
    m_time = m_pole0;
    m_direction = direction;
}
