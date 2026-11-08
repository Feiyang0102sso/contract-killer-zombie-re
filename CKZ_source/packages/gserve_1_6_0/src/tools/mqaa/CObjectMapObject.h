// Owner: tools/mqaa/CObjectMapObject.cpp (b028x; started as a b016x PROVISIONAL)
// CObjectMapObject.h - keyed container node of a CObjectMap tree (a JSON object).
// Keys and values are kept in two parallel vectors; the object owns its values.
#ifndef COBJECTMAPOBJECT_H
#define COBJECTMAPOBJECT_H

#include "CObjectMapValue.h"
#include "CVector.h"

class CObjectMapObject : public CObjectMapValue {
public:
    CObjectMapObject()
        : m_parent(NULL)
    {
    }

    /** Deletes every value. */
    virtual ~CObjectMapObject();

    virtual CObjectMapValue* clone() const;

    virtual int getType() const
    {
        return TYPE_OBJECT;
    }

    /** Value stored under `key`, NULL when absent. */
    CObjectMapValue* getEntry(const com::glu::platform::components::CStrWChar& key) const;

    /** Append `value` under `key`; the object takes ownership of `value`. */
    void addEntry(const com::glu::platform::components::CStrWChar& key, CObjectMapValue* value)
    {
        m_keys.Add(key);
        m_values.Add(value);
    }

    CObjectMapValue* m_parent;                                              // +0x04 enclosing container while building
    com::glu::platform::core::CVector<CObjectMapValue*> m_values;           // +0x08
    com::glu::platform::core::CVector<com::glu::platform::components::CStrWChar> m_keys; // +0x20
};

#endif
