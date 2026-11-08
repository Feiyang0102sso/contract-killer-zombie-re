// CObjectMap.cpp - one gServe message: a shared CObjectMapValue tree plus the queue's message id.
// The building helpers keep m_current on the container being filled.
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"

using com::glu::platform::components::CStrWChar;

CObjectMap::CObjectMap(const CObjectMap& other)
    : m_messageId(other.m_messageId)
    , m_root(other.m_root)
    , m_state(other.m_state)
    , m_current(NULL)
{
}

CObjectMap::CObjectMap(const CObjectMapValue* root)
    : m_messageId(0)
    , m_state(1)
    , m_current(NULL)
{
    if (root != NULL) {
        m_root = CSharedPtr_gServe<CObjectMapValue>(root->clone());
        m_current = m_root.get();
    }
}

unsigned char CObjectMap::beginObject()
{
    unsigned char opened;
    if (m_current != NULL) {
        opened = 0;
        if (m_current->getType() == CObjectMapValue::TYPE_ARRAY) {
            // New element of the current array.
            CObjectMapObject* object = new CObjectMapObject();
            opened = 1;
            object->m_parent = m_current;
            m_current = object;
            ((CObjectMapArray*)object->m_parent)->m_values.Add(object);
        }
    } else {
        // First object: it becomes the root.
        m_root = CSharedPtr_gServe<CObjectMapValue>(new CObjectMapObject());
        opened = 1;
        m_current = m_root.get();
    }
    return opened;
}

unsigned char CObjectMap::beginObject(const CStrWChar& key)
{
    unsigned char opened = 0;
    if (m_current != NULL && m_current->getType() == CObjectMapValue::TYPE_OBJECT) {
        CObjectMapObject* object = new CObjectMapObject();
        opened = 1;
        object->m_parent = m_current;
        m_current = object;
        ((CObjectMapObject*)object->m_parent)->addEntry(key, object);
    }
    return opened;
}

bool CObjectMap::endObject()
{
    if (m_current == NULL) {
        return false;
    }
    if (m_current->getType() == CObjectMapValue::TYPE_OBJECT) {
        m_current = ((CObjectMapObject*)m_current)->m_parent;
        return true;
    }
    return false;
}

unsigned char CObjectMap::addEntry(const CStrWChar& key, const CObjectMapInt& value)
{
    unsigned char added = 0;
    if (m_current != NULL && m_current->getType() == CObjectMapValue::TYPE_OBJECT) {
        CObjectMapObject* object = (CObjectMapObject*)m_current;
        object->addEntry(key, new CObjectMapInt(value));
        added = 1;
    }
    return added;
}

CObjectMapValue* CObjectMap::getDataAt(const CStrWChar& path, const CStrWChar& separator) const
{
    CObjectMapValue* result = NULL;
    if (path.GetLength() == 0 || separator.GetLength() == 0) {
        return result;
    }
    CObjectMapValue* node = m_root.get();
    int start = 0;
    do {
        // Next path component: up to the separator, or the rest of the path.
        int end = path.Find(separator.m_string, start) - 1;
        unsigned char isLast;
        if (end < 0) {
            end = path.GetLength() - 1;
            isLast = 1;
        } else {
            isLast = 0;
        }
        CStrWChar component = path.GetSubString(start, end);
        start = end + separator.GetLength() + 1;
        int type = node->getType();
        if (type == CObjectMapValue::TYPE_ARRAY) {
            // Array step: the component is a decimal index.
            result = NULL;
            char* number = new char[component.GetLength() + 1];
            wcstombs(number, component.m_string, wcslen(component.m_string) + 1);
            int index = atoi(number);
            if (index >= 0) {
                CObjectMapArray* array = (CObjectMapArray*)node;
                if (index < array->m_values.Size()) {
                    result = array->m_values[index];
                }
            }
            delete[] number;
        } else if (type == CObjectMapValue::TYPE_OBJECT) {
            result = ((CObjectMapObject*)node)->getEntry(component);
        } else {
            result = NULL;
        }
        if (isLast) {
            break;
        }
        node = result;
    } while (result != NULL);
    return result;
}
