// CNGSSessionConfig.h - persistent part of a gServe session: the session id issued by the server.
// Stored as a small JSON object ({"sessionid": ...}) in an application data file.
#ifndef CNGSSESSIONCONFIG_H
#define CNGSSESSIONCONFIG_H

#include "CStrWChar.h"

class CObjectMapObject;

class CNGSSessionConfig {
public:
    CNGSSessionConfig();
    /** Initialized from the "sessionid" entry of `object`. */
    CNGSSessionConfig(CObjectMapObject* object);

    /** Forget the session id (empty string). */
    void reset();
    CNGSSessionConfig& operator=(const CNGSSessionConfig& other);
    bool operator==(const CNGSSessionConfig& other);
    /** {"sessionid": <id>} (the caller owns the result). */
    CObjectMapObject* CreateObjectRepresentation();
    /** Take the "sessionid" of `object`; 1 when a non-empty id was found. */
    unsigned char InitFromObjectRepresentation(CObjectMapObject* object);
    /** Save to the application data file `fileName` (obfuscated with `key` when `useKey`). */
    unsigned char writeToFile(com::glu::platform::components::CStrWChar& fileName,
                              com::glu::platform::components::CStrWChar& key, bool useKey);
    unsigned char readFromFile(com::glu::platform::components::CStrWChar& fileName,
                               com::glu::platform::components::CStrWChar& key, bool useKey);

    com::glu::platform::components::CStrWChar m_sessionId;    // +0x00
};

#endif
