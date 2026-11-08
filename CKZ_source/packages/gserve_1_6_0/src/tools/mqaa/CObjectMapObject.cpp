// CObjectMapObject.cpp - keyed container node of a CObjectMap tree (a JSON object).
#include "CObjectMapObject.h"

using com::glu::platform::components::CStrWChar;

CObjectMapObject::~CObjectMapObject()
{
    for (int index = m_values.Size() - 1; index >= 0; index--) {
        if (m_values[index] != NULL) {
            delete m_values[index];
        }
    }
}

CObjectMapValue* CObjectMapObject::clone() const
{
    CObjectMapObject* copy = new CObjectMapObject();
    for (int index = 0; index < m_values.Size(); index++) {
        copy->m_values.Add(m_values[index]->clone());
    }
    copy->m_keys = m_keys;
    // A clone is a detached tree.
    copy->m_parent = NULL;
    return copy;
}

CObjectMapValue* CObjectMapObject::getEntry(const CStrWChar& key) const
{
    for (int index = 0; index < m_keys.Size(); index++) {
        if (m_keys[index] == key) {
            return m_values[index];
        }
    }
    return NULL;
}
