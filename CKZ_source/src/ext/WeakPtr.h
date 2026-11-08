// PROVISIONAL (created by b047x): owner src/FPSGameplay/AI/Executors/CAttackExecutor.cpp not written yet
// WeakPtr.h - intrusive weak pointers of the game layer.
// A Referenced<T> object owns a shared ObjectInfo block that points back at the object; the
// destructor clears that back pointer, so every WeakIntrusivePtr holding the block sees the
// object die. Layouts from src/anim3d (AnimationInstance, AnimatableObject, AnimationTransition).
#ifndef WEAKPTR_H
#define WEAKPTR_H

#include <stddef.h>
#include "CSharedPtr.h"

/** Base of objects that can be watched by WeakIntrusivePtr. */
template <class T>
class Referenced {
public:
    /** Shared block: m_object is cleared when the object is destroyed. */
    struct ObjectInfo {
        ObjectInfo()
        {
            m_reserved = 0;
            m_object = NULL;
        }

        int m_reserved;     // 0x00 (never read in the known code)
        T* m_object;        // 0x04 the living object, NULL once destroyed
    };

    Referenced()
        : m_info(new ObjectInfo())
    {
        m_info.m_pointer->m_object = static_cast<T*>(this);
    }

    virtual ~Referenced()
    {
        m_info.m_pointer->m_object = NULL;
    }

    com::glu::platform::core::CSharedPtr<ObjectInfo> m_info;    // 0x04
};

/** Weak pointer to a Referenced<T> object: get() is NULL once the object is gone. */
template <class T>
class WeakIntrusivePtr {
public:
    WeakIntrusivePtr(T* object = NULL)
    {
        m_object = NULL;
        bind(object);
    }

    /** Watch `object` (NULL releases the shared block). */
    void bind(T* object);

    /** The object, NULL when none is bound or it was destroyed. */
    T* get() const
    {
        // select form (?:) as in the original: every failed test yields NULL
        return (m_object != NULL && m_info.m_pointer != NULL && m_info.m_count->m_useCount != 0
                && m_info.m_pointer->m_object != NULL) ? m_object : NULL;
    }

    T* operator->() const
    {
        return get();
    }

    com::glu::platform::core::CSharedPtr<typename Referenced<T>::ObjectInfo> m_info;     // 0x00
    T* m_object;                                                                        // 0x08
};

template <class T>
void WeakIntrusivePtr<T>::bind(T* object)
{
    if (m_object == object) {
        return;
    }
    m_object = object;
    if (object != NULL) {
        m_info = object->m_info;
    } else {
        // drop the shared block: swap it into a temporary that releases it
        com::glu::platform::core::CSharedPtr<typename Referenced<T>::ObjectInfo> released;
        released.m_pointer = m_info.m_pointer;
        m_info.m_pointer = NULL;
        released.m_count = m_info.m_count;
        m_info.m_count = NULL;
    }
}

/** Two weak pointers are equal when they currently reach the same object. */
template <class T>
bool operator==(const WeakIntrusivePtr<T>& left, const WeakIntrusivePtr<T>& right)
{
    return left.get() == right.get();
}

/** Original e9150 resolves the live object on each side separately, then compares the real pointers; it does not compare the shared block. */
template <class T>
bool operator!=(const WeakIntrusivePtr<T>& left, const WeakIntrusivePtr<T>& right)
{
    return left.get() != right.get();
}

#endif
