// DGMovingEntity.cpp - constrained movement in 20.12 fixed point coordinates.
#include "DGMovingEntity.h"
#include "Fixed.h"
#include <stdlib.h>

MovingEntity::MovingEntity()
{
    m_constraints = 0;
    m_movementMode = -1;
    m_startX = 0;
    m_positionX = 0;
    m_startY = 0;
    m_positionY = 0;
    m_elapsedMs = 0;
}

MovingEntity::~MovingEntity()
{
}

bool MovingEntity::isTargetReached() const
{
    if (m_constraints == 0) {
        if (m_velocityX > 0) {
            if (m_positionX >= m_targetX) {
                return true;
            }
        } else if (m_velocityX < 0) {
            if (m_positionX <= m_targetX) {
                return true;
            }
        }
        bool reached = true;
        if (m_velocityY > 0) {
            if (m_positionY < m_targetY) {
                reached = false;
            }
        } else {
            reached = false;
            if (m_velocityY < 0) {
                reached = m_positionY <= m_targetY;
            }
        }
        return reached;
    }
    if (m_positionX == m_targetX && m_positionY == m_targetY) {
        return true;
    }
    return !checkConstraints(m_positionX, m_positionY);
}

void MovingEntity::update(int elapsedMs)
{
    int mode = m_movementMode;
    if (mode == -1) {
        return;
    }
    bool reached = isTargetReached();
    int deltaX = 0;
    int deltaY = 0;
    switch (mode) {
    case 1:
        if (!reached) {
            deltaY = m_velocityY * elapsedMs / 1000;
            deltaX = m_velocityX * elapsedMs / 1000;
            // The original compares the magnitude before clamping each axis.
            if (abs(deltaX) > abs(m_targetX - m_positionX)) {
                deltaX = m_targetX - m_positionX;
            }
            if (abs(deltaY) > abs(m_targetY - m_positionY)) {
                deltaY = m_targetY - m_positionY;
            }
        }
        break;
    case 0:
        if (!reached) {
            m_elapsedMs += elapsedMs;
            if (m_elapsedMs > m_durationMs) {
                m_elapsedMs = m_durationMs;
                deltaX = m_targetX - m_positionX;
                deltaY = m_targetY - m_positionY;
            } else {
                int positionY = m_positionY;
                int positionX = m_positionX;
                int startX = m_startX;
                int startY = m_startY;
                int distanceX = m_targetX - startX;
                int distanceY = m_targetY - startY;
                int fraction = Fixed::div(m_elapsedMs, m_durationMs);
                deltaX = startX + Fixed::mul(distanceX, fraction) - positionX;
                deltaY = startY + Fixed::mul(distanceY, fraction) - positionY;
            }
        }
        break;
    }
    if (deltaY != 0 || deltaX != 0) {
        m_positionX += deltaX;
        m_positionY += deltaY;
        correctPositionByConstraints();
    }
}

void MovingEntity::correctPositionByConstraints()
{
    int constraints = m_constraints;
    if (constraints == 0) {
        return;
    }
    int reached = 0;
    if ((constraints & 8) != 0 && m_positionX < m_minX) {
        m_positionX = m_minX;
        reached = 8;
        if (m_targetX < m_minX) {
            m_targetX = m_minX;
        }
    }
    if ((constraints & 2) != 0 && m_positionX > m_maxX) {
        m_positionX = m_maxX;
        if (m_targetX > m_maxX) {
            m_targetX = m_maxX;
        }
        reached |= 2;
    }
    if ((constraints & 1) != 0 && m_positionY < m_minY) {
        m_positionY = m_minY;
        if (m_targetY < m_minY) {
            m_targetY = m_minY;
        }
        reached |= 1;
    }
    if ((constraints & 4) != 0 && m_positionY > m_maxY) {
        m_positionY = m_maxY;
        if (m_targetY > m_maxY) {
            m_targetY = m_maxY;
        }
        reached |= 4;
    }
    if (reached != 0) {
        int positionX = m_positionX;
        int positionY = m_positionY;
        m_elapsedMs = 0;
        m_startX = positionX;
        m_startY = positionY;
        m_targetX = positionX;
        m_targetY = positionY;
        onConstraintReached(reached);
    }
}

bool MovingEntity::checkConstraints(int x, int y) const
{
    int constraints = m_constraints;
    if (constraints == 0) {
        return true;
    }
    if ((constraints & 8) != 0 && x < m_minX) {
        return false;
    }
    if ((constraints & 2) != 0 && x > m_maxX) {
        return false;
    }
    if ((constraints & 1) != 0 && y < m_minY) {
        return false;
    }
    if ((constraints & 4) != 0) {
        return y <= m_maxY;
    }
    return true;
}
