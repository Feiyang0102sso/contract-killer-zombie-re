// CNGSSessionConfig.cpp - persisted gServe session id.
#include "CNGSSessionConfig.h"
#include "CObjectMapObject.h"
#include "CArrayInputStream.h"
#include "CArrayOutputStream.h"
#include "CNGSJSONParser.h"
#include "CFileUtil_gServe.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CArrayInputStream;
using com::glu::platform::components::CArrayOutputStream;

/** Size of the session file. */
static const unsigned int SESSION_FILE_SIZE = 512;
/** Longest JSON text read back from the session file (characters). */
static const unsigned int SESSION_TEXT_CAPACITY = 129;

CNGSSessionConfig::CNGSSessionConfig()
{
    reset();
}

void CNGSSessionConfig::reset()
{
    m_sessionId = L"";
}

CNGSSessionConfig::CNGSSessionConfig(CObjectMapObject* object)
{
    reset();
    CObjectMapValue* entry = object->getEntry(CStrWChar("sessionid"));
    if (entry != NULL && entry->getType() == CObjectMapValue::TYPE_STRING) {
        CStrWChar sessionId = ((CObjectMapString*)entry)->m_value;
        m_sessionId = sessionId;
    }
}

CNGSSessionConfig& CNGSSessionConfig::operator=(const CNGSSessionConfig& other)
{
    m_sessionId = other.m_sessionId;
    return *this;
}

bool CNGSSessionConfig::operator==(const CNGSSessionConfig& other)
{
    return m_sessionId == other.m_sessionId;
}

CObjectMapObject* CNGSSessionConfig::CreateObjectRepresentation()
{
    CObjectMapObject* object = new CObjectMapObject();
    CStrWChar key("sessionid");
    object->addEntry(key, new CObjectMapString(m_sessionId));
    return object;
}

unsigned char CNGSSessionConfig::InitFromObjectRepresentation(CObjectMapObject* object)
{
    unsigned char result = 0;
    if (object != NULL) {
        CObjectMapValue* entry = object->getEntry(CStrWChar("sessionid"));
        if (entry != NULL) {
            result = 0;
            if (entry->getType() == CObjectMapValue::TYPE_STRING) {
                m_sessionId = CStrWChar(((CObjectMapString*)entry)->m_value);
                result = 1;
                if (m_sessionId.GetLength() < 1) {
                    result = 0;
                    m_sessionId = L"";
                }
            }
        }
    }
    return result;
}

unsigned char CNGSSessionConfig::writeToFile(CStrWChar& fileName, CStrWChar& key, bool useKey)
{
    unsigned char buffer[SESSION_FILE_SIZE] = {0};
    unsigned char result = 0;
    CArrayOutputStream stream;
    if (stream.Open(buffer, SESSION_FILE_SIZE)) {
        CObjectMapObject* object = CreateObjectRepresentation();
        stream.WriteJMUtf(CNGSJSONParser::encodeValue(object).GetWCharPtr());
        if (object != NULL) {
            delete object;
        }
        const wchar_t* keyText = NULL;
        if (useKey) {
            keyText = key.GetWCharPtr();
        }
        result = CFileUtil_gServe::WriteApplicationDataFile(fileName.GetWCharPtr(), buffer, SESSION_FILE_SIZE, keyText);
        stream.Close();
    }
    return result;
}

unsigned char CNGSSessionConfig::readFromFile(CStrWChar& fileName, CStrWChar& key, bool useKey)
{
    unsigned char buffer[SESSION_FILE_SIZE] = {0};
    const wchar_t* keyText = NULL;
    if (useKey) {
        keyText = key.GetWCharPtr();
    }
    unsigned char result = 0;
    if (CFileUtil_gServe::ReadApplicationDataFile(fileName.GetWCharPtr(), buffer, SESSION_FILE_SIZE, keyText)) {
        result = 0;
        CArrayInputStream stream;
        if (stream.Open(buffer, SESSION_FILE_SIZE)) {
            wchar_t text[SESSION_TEXT_CAPACITY];
            int length = stream.ReadJMUtf(text, SESSION_TEXT_CAPACITY);
            text[length] = L'\0';
            if (length > 0) {
                CObjectMapObject* object = (CObjectMapObject*)CNGSJSONParser::decodeValue(CStrWChar(text));
                result = InitFromObjectRepresentation(object);
                if (object != NULL) {
                    delete object;
                }
            }
        }
    }
    return result;
}
