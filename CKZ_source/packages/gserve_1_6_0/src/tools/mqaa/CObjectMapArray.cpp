// CObjectMapArray.cpp - indexed container node of a CObjectMap tree (a JSON array).
#include "CObjectMapArray.h"

CObjectMapValue* CObjectMapArray::clone() const
{
    CObjectMapArray* copy = new CObjectMapArray();
    for (int index = 0; index < m_values.Size(); index++) {
        copy->m_values.Add(m_values[index]->clone());
    }
    // A clone is a detached tree.
    copy->m_parent = NULL;
    return copy;
}

CObjectMapArray::~CObjectMapArray()
{
    for (int index = m_values.Size() - 1; index >= 0; index--) {
        if (m_values[index] != NULL) {
            delete m_values[index];
        }
    }
}
