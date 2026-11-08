// CNGSJSONParser.h - minimal JSON reader/writer for gServe messages.
// Decoding builds a CObjectMapValue tree (objects, arrays, strings, 64-bit integers, booleans, null);
// encoding writes such a tree back. All members are static.
#ifndef CNGSJSONPARSER_H
#define CNGSJSONPARSER_H

#include "CStrWChar.h"
#include "CVector.h"
#include "CObjectMap.h"

class CWStringBuffer;

class CNGSJSONParser {
public:
    /** Resolve the backslash escapes of a JSON string body. */
    static com::glu::platform::components::CStrWChar decodeString(const com::glu::platform::components::CStrWChar& text);
    /** Escape the characters JSON strings cannot hold literally. */
    static com::glu::platform::components::CStrWChar encodeString(const com::glu::platform::components::CStrWChar& text);
    /** {"payload":[{"version":v, "sequenceId":id, "message":{...}}, ...]} */
    static com::glu::platform::components::CStrWChar encodeMessages(const com::glu::platform::core::CVector<CObjectMap>& messages);
    static void encodeInteger(CWStringBuffer& buffer, long long value);
    static void encodeValue(CWStringBuffer& buffer, const CObjectMapValue* value);
    static com::glu::platform::components::CStrWChar encodeValue(const CObjectMapValue* value);
    /** Append the messages of a payload document to `messages`; returns how many were added. */
    static int decodeMessages(const com::glu::platform::components::CStrWChar& json,
                              com::glu::platform::core::CVector<CObjectMap>& messages);
    /** Parse a JSON object or array (the caller owns the result); NULL on a syntax error. */
    static CObjectMapValue* decodeValue(const com::glu::platform::components::CStrWChar& json);
    /** Split `json` into tokens; 0 when the brackets do not match. */
    static int tokenize(com::glu::platform::core::CVector<com::glu::platform::components::CStrWChar>& tokens,
                        com::glu::platform::components::CStrWChar json);
    static long long decodeInteger(const com::glu::platform::components::CStrWChar& text);
};

#endif
