// CFlurry.h - Flurry analytics facade used by the game (session start, custom events).
#ifndef CFLURRY_H
#define CFLURRY_H

class CFlurry {
public:
    /** Starts the Flurry session once (later calls do nothing) and logs the start events. */
    static void StartSession(const char* apiKey);

    /** Logs `eventName` with a wide-string info parameter and two integer parameters. */
    static void LogCustomEvent(const char* eventName, const wchar_t* info, int eventValue, int externalRef);
    /** Same with a UTF-8 info parameter. */
    static void LogCustomEvent(const char* eventName, const char* info, int eventValue, int externalRef);

    static void SetSessionReportsOnCloseEnabled(bool enabled);
    static void SetSessionReportsOnPauseEnabled(bool enabled);
};

#endif
