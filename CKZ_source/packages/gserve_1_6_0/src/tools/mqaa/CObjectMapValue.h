// PROVISIONAL (created by b016x): header-only value classes, no owning unit; tools/mqaa (b029) not written yet
// CObjectMapValue.h - JSON-like value tree of gServe messages: the abstract value plus the
// leaf types (int, string, bool, null). Objects and arrays live in CObjectMapObject.h / CObjectMapArray.h.
#ifndef COBJECTMAPVALUE_H
#define COBJECTMAPVALUE_H

#include <stddef.h>
#include "CStrWChar.h"

/** Base of every node in a CObjectMap tree. */
class CObjectMapValue {
public:
    /** getType() results. */
    enum eType {
        TYPE_OBJECT = 0,
        TYPE_ARRAY = 1,
        TYPE_BOOL = 2,
        TYPE_INT = 3,
        TYPE_STRING = 4,
        TYPE_NULL = 5
    };

    CObjectMapValue() {}
    virtual ~CObjectMapValue() {}

    /** Deep copy (the caller owns the result). */
    virtual CObjectMapValue* clone() const = 0;
    virtual int getType() const = 0;
};

/** 64-bit integer leaf (long long is 4-byte aligned on iOS ARM, so the value sits at +4). */
class CObjectMapInt : public CObjectMapValue {
public:
    explicit CObjectMapInt(long long value)
        : m_value(value)
    {
    }

    virtual ~CObjectMapInt() {}

    virtual CObjectMapValue* clone() const
    {
        return new CObjectMapInt(*this);
    }

    virtual int getType() const
    {
        return TYPE_INT;
    }

    long long m_value;      // +0x04
};

/** Wide string leaf. */
class CObjectMapString : public CObjectMapValue {
public:
    explicit CObjectMapString(const com::glu::platform::components::CStrWChar& value)
    {
        m_value += value;
    }

    virtual ~CObjectMapString() {}

    virtual CObjectMapValue* clone() const
    {
        return new CObjectMapString(m_value);
    }

    virtual int getType() const
    {
        return TYPE_STRING;
    }

    com::glu::platform::components::CStrWChar m_value;     // +0x04
};

/** Boolean leaf. */
class CObjectMapBool : public CObjectMapValue {
public:
    explicit CObjectMapBool(unsigned char value)
        : m_value(value)
    {
    }

    virtual ~CObjectMapBool() {}

    virtual CObjectMapValue* clone() const
    {
        return new CObjectMapBool(*this);
    }

    virtual int getType() const
    {
        return TYPE_BOOL;
    }

    unsigned char m_value;  // +0x04
};

/** JSON null. */
class CObjectMapNull : public CObjectMapValue {
public:
    CObjectMapNull() {}
    virtual ~CObjectMapNull() {}

    virtual CObjectMapValue* clone() const
    {
        return new CObjectMapNull();
    }

    virtual int getType() const
    {
        return TYPE_NULL;
    }
};

#endif
