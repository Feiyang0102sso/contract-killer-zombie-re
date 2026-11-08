// CNGSLoginFlow.h - login step sequencer: the game queues CNGSLoginStep objects, Start() runs the
// first one and every NextStep() advances to the following one (singleton, sizeof 0x2c).
// (b023x; created as PROVISIONAL by b016x)
#ifndef CNGSLOGINFLOW_H
#define CNGSLOGINFLOW_H

#include <stddef.h>
#include "CSingleton.h"
#include "CVector.h"

/** Events forwarded from CNGSLocalUser to the running step. Only the values seen in the
    original callers are listed; their names are not known. */
enum eLoginFlowEvent {
    LOGIN_FLOW_EVENT_2 = 2,
    LOGIN_FLOW_EVENT_7 = 7,                 // VersionCheckStep (game) reacts to this one
    LOGIN_FLOW_EVENT_8 = 8,
    LOGIN_FLOW_EVENT_9 = 9,                 // sent by CNGSHandleUserLogin / CNGSHandleFriendListUpdate
    LOGIN_FLOW_EVENT_12 = 12,
    LOGIN_FLOW_EVENT_0 = 0,                 // added by b030x: login succeeded (CNGSHandleUserLogin / CNGSHandleFriendListUpdate)
    LOGIN_FLOW_EVENT_1 = 1,                 // added by b030x: login result 10
    LOGIN_FLOW_EVENT_6 = 6,                 // added by b030x: server version accepted
    LOGIN_FLOW_EVENT_10 = 10,               // added by b030x: server connection error
    LOGIN_FLOW_EVENT_11 = 11                // added by b030x: login result 1010 / 1011
};

/** One step of the login flow. No virtual destructor: the flow frees steps with plain delete. */
class CNGSLoginStep {
public:
    /** Run the step (the default just executes it). */
    virtual void Prepare()
    {
        Excute();
    }

    /** Do the work of the step (the spelling is the original's). */
    virtual void Excute() = 0;
    virtual void OnEvent(eLoginFlowEvent event, const char* caller) = 0;

    virtual unsigned int GetClassId()
    {
        return m_classId;
    }

    virtual int GetSnType()
    {
        return m_snType;
    }

    unsigned int m_classId;     // +0x04
    int m_snType;               // +0x08 social network type
};

/** Step that checks the game version with the server. */
class CNGSLoginStep_CheckVersion : public CNGSLoginStep {
public:
    virtual void Excute();
};

class CNGSLoginFlow : public com::glu::platform::components::CSingleton {
public:
    enum { ClassId = 0x916da8fd };

    CNGSLoginFlow();
    virtual ~CNGSLoginFlow();

    /** The registered instance, created on first use. */
    static inline CNGSLoginFlow* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNGSLoginFlow* instance = (CNGSLoginFlow*)found;
        if (instance == NULL) {
            instance = new CNGSLoginFlow();
        }
        return instance;
    }

    /** Delete every queued step and stop. */
    void Reset();
    /** A step is current (0 <= step < queued steps). */
    unsigned char IsRunning();
    void Queue(CNGSLoginStep* step);
    void Start();
    /** Make `step` current (-1 keeps the current one) and prepare it. */
    void PrepareStep(int step);
    /** Pass `event` to the current step. */
    void OnEvent(eLoginFlowEvent event, const char* caller);
    /** Advance to the next step; finished after the last one. */
    void NextStep();

    int m_currentStep;                                              // +0x08 -1 when not running
    com::glu::platform::core::CVector<CNGSLoginStep*> m_steps;     // +0x0c owned
    unsigned char m_finished;                                       // +0x24
    int m_unused28;                                                 // +0x28 set to -1, never read here
};

#endif
