// CSharedPtr.h - reference-counted smart pointer of the Glu platform (header-only; b000x).
// The pointee is owned by a control block (CShareCountP: delete, CShareCountPD: custom
// deleter). A CSharedPtr holding NULL keeps its control block but does not count as a user.
#ifndef CSHAREDPTR_H
#define CSHAREDPTR_H

#include <stddef.h>

namespace com { namespace glu { namespace platform { namespace core {

/** Control block shared by all CSharedPtr copies of one pointer. */
class CSharedCountBase {
public:
    CSharedCountBase()
    {
        m_useCount = 0;
        m_blockCount = 0;
    }

    /** Destroy the pointee. */
    virtual void Dispose() = 0;
    /** The custom deleter, NULL when the pointee is deleted with delete. */
    virtual void* GetDeleter() const = 0;

    /** Drop one user; true when that was the last one (the pointee is disposed then). */
    unsigned char Release()
    {
        if (m_useCount == 0) {
            return 0;
        }
        m_useCount--;
        if (m_useCount != 0) {
            return 0;
        }
        Dispose();
        return 1;
    }

    int m_useCount;         // 0x04 CSharedPtr copies holding a non-NULL pointer
    int m_blockCount;       // 0x08 references to this control block
};

/** Control block deleting its pointer with delete. */
template <class T>
class CShareCountP : public CSharedCountBase {
public:
    explicit CShareCountP(T* pointer)
    {
        m_pointer = pointer;
    }

    virtual void Dispose()
    {
        delete m_pointer;
        m_pointer = NULL;
    }

    virtual void* GetDeleter() const
    {
        return NULL;
    }

    T* m_pointer;           // 0x0c
};

/** Control block calling a custom deleter. */
template <class T, class D>
class CShareCountPD : public CSharedCountBase {
public:
    CShareCountPD(T* pointer, D deleter)
    {
        m_pointer = pointer;
        m_deleter = deleter;
    }

    virtual void Dispose()
    {
        if (m_pointer != NULL) {
            m_deleter(m_pointer);
            m_pointer = NULL;
        }
    }

    virtual void* GetDeleter() const
    {
        return (void*)m_deleter;
    }

    T* m_pointer;           // 0x0c
    D m_deleter;            // 0x10
};

template <class T>
class CSharedPtr {
public:
    CSharedPtr()
    {
        m_pointer = NULL;
        m_count = NULL;
    }

    /** Take ownership of `pointer` (deleted with delete). */
    template <class Y>
    explicit CSharedPtr(Y* pointer)
    {
        m_pointer = pointer;
        m_count = new CShareCountP<Y>(pointer);
        m_count->m_blockCount++;
        if (m_pointer != NULL) {
            m_count->m_useCount++;
        }
    }

    /** Take ownership of `pointer`, released by `deleter(pointer)`. */
    template <class Y, class D>
    CSharedPtr(Y* pointer, D deleter)
    {
        m_pointer = pointer;
        m_count = new CShareCountPD<Y, D>(pointer, deleter);
        m_count->m_blockCount++;
        if (m_pointer != NULL) {
            m_count->m_useCount++;
        }
    }

    CSharedPtr(const CSharedPtr<T>& other)
    {
        m_pointer = other.m_pointer;
        m_count = other.m_count;
        if (m_pointer != NULL) {
            m_count->m_useCount++;
        }
    }

    ~CSharedPtr()
    {
        if (m_pointer != NULL && m_count != NULL) {
            if (m_count->Release()) {
                if (m_count->m_blockCount != 0) {
                    if (--m_count->m_blockCount == 0) {
                        delete m_count;
                    }
                }
            }
        }
    }

    CSharedPtr<T>& operator=(const CSharedPtr<T>& other)
    {
        // copy, then swap the copy with this one; the copy releases the old pointer
        CSharedPtr<T> copy(other);
        T* pointer = copy.m_pointer;
        copy.m_pointer = m_pointer;
        m_pointer = pointer;
        CSharedCountBase* count = copy.m_count;
        copy.m_count = m_count;
        m_count = count;
        return *this;
    }

    T* Get() const
    {
        return m_pointer;
    }

    T& operator*() const
    {
        return *m_pointer;
    }

    T* operator->() const
    {
        return m_pointer;
    }

    T* m_pointer;                   // 0x00
    CSharedCountBase* m_count;      // 0x04
};

} } } }

#endif
