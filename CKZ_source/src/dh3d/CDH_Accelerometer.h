// CDH_Accelerometer.h - cursor movement driven by the device tilt.
#ifndef CDH_ACCELEROMETER_H
#define CDH_ACCELEROMETER_H

#include <stddef.h>

class CDH_Accelerometer {
public:
    /** Number of samples kept per axis. */
    enum { SAMPLE_COUNT = 20 };

    CDH_Accelerometer();
    virtual ~CDH_Accelerometer();

    /** Move the position by the current tilt for `elapsedMs` milliseconds. */
    void update(int elapsedMs);

    float* m_samplesX;      // 0x04 SAMPLE_COUNT floats
    float* m_samplesY;      // 0x08 SAMPLE_COUNT floats
    int m_field0c;          // 0x0c
    int m_field10;          // 0x10
    int m_field14;          // 0x14
    float m_positionX;      // 0x18
    float m_positionY;      // 0x1c
    float m_tiltX;          // 0x20
    float m_tiltY;          // 0x24
};

#endif
