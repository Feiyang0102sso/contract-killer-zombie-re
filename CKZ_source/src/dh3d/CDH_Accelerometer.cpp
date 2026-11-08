// CDH_Accelerometer.cpp - cursor movement driven by the device tilt.
#include "CDH_Accelerometer.h"

/** Cursor speed per unit of tilt (pixels per second). */
static const float TILT_SPEED = 300.0f;

CDH_Accelerometer::CDH_Accelerometer()
{
    m_samplesX = new float[SAMPLE_COUNT];
    m_samplesY = new float[SAMPLE_COUNT];
    for (int index = 0; index < SAMPLE_COUNT; index++) {
        m_samplesY[index] = 0;
        m_samplesX[index] = 0;
    }
    m_field0c = 0;
    m_field10 = 0;
    m_field14 = 0;
    m_positionX = 0;
    m_positionY = 0;
    m_tiltX = 0;
    m_tiltY = 0;
}

CDH_Accelerometer::~CDH_Accelerometer()
{
    if (m_samplesX != NULL) {
        delete[] m_samplesX;
        m_samplesX = NULL;
    }
    if (m_samplesY != NULL) {
        delete[] m_samplesY;
        m_samplesY = NULL;
    }
}

void CDH_Accelerometer::update(int elapsedMs)
{
    float seconds = elapsedMs * 0.001f;
    m_positionX += m_tiltX * TILT_SPEED * seconds * 2.0f;
    m_positionY += m_tiltY * TILT_SPEED * seconds * 2.0f;
}
