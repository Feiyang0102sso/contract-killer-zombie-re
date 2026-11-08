// Owner: tools/mqaa/CObjectMapArray.cpp (b028x; started as a b016x PROVISIONAL)
// CObjectMapArray.h - indexed container node of a CObjectMap tree (a JSON array); owns its values.
#ifndef COBJECTMAPARRAY_H
#define COBJECTMAPARRAY_H

#include "CObjectMapValue.h"
#include "CVector.h"

class CObjectMapArray : public CObjectMapValue {
public:
    CObjectMapArray()
        : m_parent(NULL)
    {
    }

    /** Deletes every value. */
    virtual ~CObjectMapArray();

    virtual CObjectMapValue* clone() const;

    virtual int getType() const
    {
        return TYPE_ARRAY;
    }

    /** Append `value`; the array takes ownership. */  // added by b025x
    void addEntry(CObjectMapValue* value)  // added by b025x
    {  // added by b025x
        m_values.Add(value);  // added by b025x
    }  // added by b025x

    CObjectMapValue* m_parent;                                      // +0x04 enclosing container while building
    com::glu::platform::core::CVector<CObjectMapValue*> m_values;   // +0x08
};

#endif
