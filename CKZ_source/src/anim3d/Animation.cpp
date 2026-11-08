// Animation.cpp - event list of an animation (created by b047x).
#include <string.h>
#include "Animation.h"

Animation::Animation()
{
}

Animation::~Animation()
{
    for (int index = 0; index < m_eventNames.m_size; index++) {
        if (m_eventNames.m_items[index] != NULL) {
            delete[] m_eventNames.m_items[index];
            m_eventNames.m_items[index] = NULL;
        }
    }
}

void Animation::setEvents(Vector<float>& times, Vector<const char*>& names, int count)
{
    m_eventTimes.removeAllElements();
    m_eventNames.removeAllElements();
    m_eventTimes.ensureCapacity(count);
    m_eventNames.ensureCapacity(count);
    for (int index = 0; index < count; index++) {
        m_eventTimes.addElement(times.m_items[index]);
        char* name = new char[strlen(names.m_items[index]) + 1];
        strcpy(name, names.m_items[index]);
        const char* copy = name;
        m_eventNames.addElement(copy);
    }
}
