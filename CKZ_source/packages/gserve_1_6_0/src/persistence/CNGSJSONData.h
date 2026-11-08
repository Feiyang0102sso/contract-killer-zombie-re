// CNGSJSONData.h - a JSON document fetched over HTTP (by default from the gServe S3 bucket),
// kept parsed in memory and cached obfuscated on the device; offers attribute lookups by path (b025x).
#ifndef CNGSJSONDATA_H
#define CNGSJSONDATA_H

#include <stddef.h>
#include <time.h>
#include "CNGSServerObject.h"
#include "CNGSServerRequest.h"
#include "CStrChar.h"
#include "CStrWChar.h"

class CObjectMap;
class CObjectMapValue;
class CObjectMapObject;
class CHttpTransport_gServe;

/**
 * Receiver of a raw HTTP response (CNGSJSONData::HandleUpdate calls the extra slot +0x14).
 * The original name of this intermediate class is unknown.
 */
class CNGSDataRequestFunctor : public CNGSServerRequestFunctor {
public:
    /** `data` / `length` of the body; `notModified` for an HTTP 304 reply. */
    virtual void requestCompleteCallback(char* data, int length, bool notModified) = 0;
};

/** Cache file header written before the obfuscated JSON (SaveToDisk). */
struct _CNGSJSONSaveData {
    int m_time;                 // +0x00 network time of the save
    int m_version;              // +0x04 always 0x100
    int m_length;               // +0x08 obfuscated data length + 1
    unsigned int m_crc;         // +0x0c CRC-32 of the plain JSON
    unsigned int m_paddingSize; // +0x10 random bytes between the header and the data
    void* m_data;               // +0x14 obfuscated data
    unsigned int m_totalSize;   // +0x18 file size
    com::glu::platform::components::CStrWChar m_key;    // +0x1c obfuscation key (not written)
};

class CNGSJSONData : public CNGSServerObject {
public:
    CNGSJSONData();
    virtual ~CNGSJSONData();

    /** Hook for subclasses (does nothing here). */
    virtual void CheckForUpdate();

    /** Replace (and delete) the response callback. */
    void SetRequestCallback(CNGSServerRequestFunctor* callback);
    /** Start the GET of `<base URL><path>` (conditional unless `forceDownload`); the transport's result. */
    int GetContent(const char* path, bool forceDownload);
    /** Default callback: parse the body and cache it on the device. */
    void HandleValidateCNGSJSONDataResponse(char* data, int length, bool notModified);
    /** Pump the transport and deliver the finished response. */
    void HandleUpdate(int elapsedMs);
    void SetLastAccessTime(int time);
    unsigned char IsBusy();
    void LoadFromServer(const com::glu::platform::components::CStrWChar& path, bool forceDownload);
    unsigned long long getCurrentTime();
    /** Obfuscate m_rawJSON and write it to m_saveFileName. */
    unsigned char SaveToDisk();
    unsigned char SaveToDisk(_CNGSJSONSaveData* saveData);
    /** Integer `name` of the object at `path` (its active "sale" overrides it); 0 when absent or expired. */
    unsigned char FindIntAttribute(const com::glu::platform::components::CStrWChar& path,
                                   const com::glu::platform::components::CStrWChar& name, unsigned long long* value);
    /** The entry's "expiration" / "end" date passed, or its "start" date is still ahead. */
    unsigned char entryIsExpired(CObjectMapObject* entry);
    unsigned char entryHasActiveSale(CObjectMapObject* entry);
    /** Seconds from the HTTP date `date` to now (negative for a future date). */
    double convertDateToSecondsFromNow(const com::glu::platform::components::CStrWChar& date);
    /** String `name` of the object at `path`, or the string at `path` itself. */
    unsigned char FindStringAttribute(const com::glu::platform::components::CStrWChar& path,
                                      const com::glu::platform::components::CStrWChar& name,
                                      com::glu::platform::components::CStrWChar* value);

    CNGSServerRequestFunctor* m_requestCallback;                    // +0x18 a CNGSDataRequestFunctor, owned
    com::glu::platform::components::CStrWChar m_saveFileName;       // +0x1c "gbsio.dat"
    com::glu::platform::components::CStrWChar m_overrideFileName;  // +0x2c "override.json"
    com::glu::platform::components::CStrWChar m_overrideName;      // +0x3c "override"
    bool m_noSaveToDisk;                                            // +0x4c
    bool m_busy;                                                    // +0x4d a load is in progress
    CObjectMapValue* m_root;                                        // +0x50 parsed document, owned
    char* m_rawJSON;                                                // +0x54 last downloaded document
    unsigned char* m_responseData;                                  // +0x58 copy of the last response body
    int m_responseLength;                                           // +0x5c
    bool m_hasResponse;                                             // +0x60
    unsigned char* m_buffer64;                                      // +0x64 freed only (meaning unknown)
    int m_field68;                                                  // +0x68 cleared only (meaning unknown)
    int m_field6c;                                                  // +0x6c cleared only (meaning unknown)
    time_t m_lastAccessTime;                                        // +0x70 date of the cached copy (If-Modified-Since)
    com::glu::platform::components::CStrChar m_baseURL;             // +0x74
    CHttpTransport_gServe* m_transport;                             // +0x84 request in progress
};

/** Response callback calling a CNGSJSONData member. */
class CNGSJSONDataRequestFunctor : public CNGSDataRequestFunctor {
public:
    typedef void (CNGSJSONData::*Callback)(char* data, int length, bool notModified);

    CNGSJSONDataRequestFunctor(CNGSJSONData* target, Callback callback)
        : m_target(target)
        , m_callback(callback)
    {
    }

    virtual ~CNGSJSONDataRequestFunctor() {}

    /** Unused (JSON requests deliver raw data). */
    virtual void requestCompleteCallback(CObjectMap* response);
    virtual void requestCompleteCallback(char* data, int length, bool notModified);

    CNGSJSONData* m_target;     // +0x08
    Callback m_callback;        // +0x0c
};

#endif
