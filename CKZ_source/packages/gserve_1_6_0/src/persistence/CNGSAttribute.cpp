// CNGSAttribute.cpp - typed profile attribute stored as a JSON object (b025x).
#include "CNGSAttribute.h"
#include "CNGSTokens.h"
#include "CNGSUtil.h"
#include "CStrChar.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CStrChar;

CNGSAttribute::CNGSAttribute(CStrWChar& name, unsigned char value, int collectionType)
{
    init(name, DATA_TYPE_UINT8, collectionType);
    m_object.addEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN), new CObjectMapInt(value));
}

void CNGSAttribute::init(CStrWChar& name, eDataType type, int collectionType)
{
    m_object.addEntry(CStrWChar(NGS_NAME_TOKEN), new CObjectMapString(name));
    m_object.addEntry(CStrWChar(NGS_TYPE_TOKEN), new CObjectMapInt(type));
    m_object.addEntry(CStrWChar(NGS_COLLECTION_TOKEN), new CObjectMapInt(collectionType));
    m_lastUpdatedInMS = 0;
}

CNGSAttribute::CNGSAttribute(CStrWChar& name, unsigned short value, int collectionType)
{
    init(name, DATA_TYPE_UINT16, collectionType);
    m_object.addEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN), new CObjectMapInt(value));
}

CNGSAttribute::CNGSAttribute(CStrWChar& name, unsigned int value, int collectionType)
{
    init(name, DATA_TYPE_UINT32, collectionType);
    m_object.addEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN), new CObjectMapInt(value));
}

CNGSAttribute::CNGSAttribute(CStrWChar& name, unsigned long long value, int collectionType)
{
    init(name, DATA_TYPE_UINT64, collectionType);
    m_object.addEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN), new CObjectMapInt(value));
}

CNGSAttribute::CNGSAttribute(CStrWChar& name, signed char value, int collectionType)
{
    init(name, DATA_TYPE_INT8, collectionType);
    m_object.addEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN), new CObjectMapInt(value));
}

CNGSAttribute::CNGSAttribute(CStrWChar& name, short value, int collectionType)
{
    init(name, DATA_TYPE_INT16, collectionType);
    m_object.addEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN), new CObjectMapInt(value));
}

CNGSAttribute::CNGSAttribute(CStrWChar& name, int value, int collectionType)
{
    init(name, DATA_TYPE_INT32, collectionType);
    m_object.addEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN), new CObjectMapInt(value));
}

CNGSAttribute::CNGSAttribute(CStrWChar& name, long long value, int collectionType)
{
    init(name, DATA_TYPE_INT64, collectionType);
    m_object.addEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN), new CObjectMapInt(value));
}

CNGSAttribute::CNGSAttribute(CStrWChar& name, CStrWChar& value, bool isBase64, int collectionType)
{
    eDataType type = DATA_TYPE_STRING;
    if (isBase64) {
        type = DATA_TYPE_BASE64;
    }
    init(name, type, collectionType);
    m_object.addEntry(CStrWChar(NGS_STRING_VALUE_TOKEN), new CObjectMapString(value));
}

CStrWChar CNGSAttribute::getName()
{
    return ((CObjectMapString*)m_object.getEntry(CStrWChar("name")))->m_value;
}

int CNGSAttribute::getDataType()
{
    return (int)((CObjectMapInt*)m_object.getEntry(CStrWChar("type")))->m_value;
}

long long CNGSAttribute::getLastUpdatedInMS()
{
    return m_lastUpdatedInMS;
}

int CNGSAttribute::getVal_int32()
{
    return (int)((CObjectMapInt*)m_object.getEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN)))->m_value;
}

CStrWChar CNGSAttribute::getVal_string()
{
    return ((CObjectMapString*)m_object.getEntry(CStrWChar(NGS_STRING_VALUE_TOKEN)))->m_value;
}

void CNGSAttribute::debugPrint()
{
    CStrWChar text = getName();
    CStrChar name = CNGSUtil::WStrToCStr(text);
    if (getDataType() == DATA_TYPE_STRING || getDataType() == DATA_TYPE_BASE64) {
        if (getDataType() == DATA_TYPE_STRING) {
            text = getVal_string();
        } else {
            text = "<base64 data>";
        }
        // The original logs name / value here; the log call is compiled out.
        CNGSUtil::WStrToCStr(text);
    } else {
        // The original logs the integer value here; only the lookup is left.
        m_object.getEntry(CStrWChar(NGS_INTEGER_VALUE_TOKEN));
    }
}
