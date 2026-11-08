// CNGSAttribute.h - one typed, named value of the player profile as exchanged with the gServe
// server: a JSON object {"name", "type", "collectiontype", "intValue" | "stringValue"} (b025x).
#ifndef CNGSATTRIBUTE_H
#define CNGSATTRIBUTE_H

#include "CStrWChar.h"
#include "CObjectMapObject.h"

/** "type" values of an attribute. */
enum eDataType {
    DATA_TYPE_STRING = 0,
    DATA_TYPE_UINT8 = 1,
    DATA_TYPE_UINT16 = 2,
    DATA_TYPE_UINT32 = 3,
    DATA_TYPE_UINT64 = 4,
    DATA_TYPE_INT8 = 5,
    DATA_TYPE_INT16 = 6,
    DATA_TYPE_INT32 = 7,
    DATA_TYPE_INT64 = 8,
    DATA_TYPE_BASE64 = 9          // string holding base64 encoded binary data
};

/**
 * The attribute is not polymorphic itself: its JSON object is the first member, so a
 * CNGSAttribute* is handed around as the CObjectMapValue* of that object (the original
 * constructors only store the CObjectMapObject vtable, and `delete` calls ~CObjectMapObject directly).
 */
class CNGSAttribute {
public:
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, unsigned char value, int collectionType);
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, unsigned short value, int collectionType);
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, unsigned int value, int collectionType);
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, unsigned long long value, int collectionType);
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, signed char value, int collectionType);
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, short value, int collectionType);
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, int value, int collectionType);
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, long long value, int collectionType);
    /** String attribute; `isBase64` marks binary data encoded as base64. */
    CNGSAttribute(com::glu::platform::components::CStrWChar& name, com::glu::platform::components::CStrWChar& value,
                  bool isBase64, int collectionType);

    /** Add "name", "type" and "collectiontype"; clears the update time. */
    void init(com::glu::platform::components::CStrWChar& name, eDataType type, int collectionType);

    com::glu::platform::components::CStrWChar getName();
    int getDataType();
    long long getLastUpdatedInMS();
    int getVal_int32();
    com::glu::platform::components::CStrWChar getVal_string();
    /** Log the attribute (the log output is compiled out). */
    void debugPrint();

    CObjectMapObject m_object;          // +0x00 the JSON object sent to / read from the server
    long long m_lastUpdatedInMS;        // +0x38
};

#endif
