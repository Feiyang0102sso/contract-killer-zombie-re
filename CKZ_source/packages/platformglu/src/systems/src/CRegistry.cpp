// CRegistry.cpp - priority-sorted element list with optional query accelerator.
#include "CRegistry.h"


namespace com { namespace glu { namespace platform { namespace systems {

CRegistry::CRegistry()
{
    m_classId = ClassId;
    m_sentinel.m_isSentinel = 1;
    m_sentinel.m_priority = 0;
    m_head = GetSentinel();
    m_tail = GetSentinel();
    m_accelerator = NULL;
}

void CRegistry::Add(CSystemElement& element)
{
    if (m_accelerator != NULL && !m_accelerator->m_inRegistryCall) {
        m_accelerator->Add(element);
        return;
    }

    CSystemElement* sentinel = GetSentinel();
    if (m_tail != sentinel) {
        // walk from the tail towards the head to the first element whose priority is not
        // lower than the new one
        CSystemElement* current = m_tail;
        while (current->m_priority < element.m_priority) {
            if (current == m_head) {
                // every element has a lower priority: new head
                m_head->m_prev = &element;
                element.m_prev = NULL;
                element.m_next = m_head;
                m_head = &element;
                return;
            }
            if (current->m_prev != NULL) {
                current = current->m_prev;
            }
        }

        // insert after `current`
        element.m_prev = current;
        element.m_next = current->m_next;
        current->m_next = &element;
        if (element.m_next == sentinel) {
            m_tail = &element;
        }
        element.m_next->m_prev = &element;
    } else {
        // first element
        element.m_prev = NULL;
        element.m_next = sentinel;
        m_sentinel.m_prev = &element;
        m_head = &element;
        m_tail = &element;
    }
}

unsigned char CRegistry::Remove(CSystemElement& element)
{
    if (m_accelerator != NULL && !m_accelerator->m_inRegistryCall) {
        return m_accelerator->Remove(element);
    }

    if (element.m_prev != NULL) {
        element.m_prev->m_next = element.m_next;
    } else {
        m_head = element.m_next;
        if (m_head == GetSentinel()) {
            // the list is empty now
            m_tail = GetSentinel();
            return 1;
        }
    }
    if (element.m_next != NULL) {
        element.m_next->m_prev = element.m_prev;
    }
    if (element.m_next == GetSentinel()) {
        // removed the last element
        m_tail = element.m_next->m_prev;
    }
    return 1;
}

unsigned char CRegistry::QueryClassId(unsigned int elementClassId, CRegistryElement** result)
{
    unsigned char found = 0;
    CSystemElement* match = NULL;
    CSystemElement* element = m_head;
    while (element != GetSentinel()) {
        if (element->m_elementClassId == elementClassId) {
            match = element;
            found = 1;
            break;
        }
        if (element->m_next != NULL) {
            element = element->m_next;
        }
    }
    *result = (CRegistryElement*)match;
    return found;
}

unsigned char CRegistry::QueryHandle(unsigned int handle, CRegistryElement** result)
{
    unsigned char found = 0;
    CSystemElement* match = NULL;
    CSystemElement* element = m_head;
    while (element != GetSentinel()) {
        if (element->m_handle == handle) {
            match = element;
            found = 1;
            break;
        }
        if (element->m_next != NULL) {
            element = element->m_next;
        }
    }
    *result = (CRegistryElement*)match;
    return found;
}

unsigned char CRegistry::QueryClassIdAndHandle(unsigned int elementClassId, unsigned int handle,
                                               CRegistryElement** result)
{
    unsigned char found = 0;
    CSystemElement* match = NULL;
    CSystemElement* element = m_head;
    while (element != GetSentinel()) {
        if (element->m_handle == handle && element->m_elementClassId == elementClassId) {
            match = element;
            found = 1;
            break;
        }
        if (element->m_next != NULL) {
            element = element->m_next;
        }
    }
    *result = (CRegistryElement*)match;
    return found;
}

} } } }
