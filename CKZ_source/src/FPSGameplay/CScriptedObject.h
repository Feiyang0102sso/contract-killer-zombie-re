// PROVISIONAL (created by b079): owner src/FPSGameplay/CScriptedObject.cpp not written yet
// Registration-facing declarations only; no allocation or complete vtable claim.
// b065: the two lines above keep the old declaration history; this owner now restores the real 0x90 layout and four virtual slots.
#ifndef CSCRIPTEDOBJECT_H
#define CSCRIPTEDOBJECT_H
#include "CStrChar.h"
#include "xString.h"
#include "HashTable.h"
#include "Vector_ss.h"
using com::glu::platform::components::CStrChar;
struct lua_State;
class CScriptedObject {
public:
    // virtual ~CScriptedObject(); // only establishes the known primary vptr
    // b065: the old virtual destructor was only a placeholder; in the original the first slot is OnTimer, destruction is done by natural member destructors and takes no virtual slot.
    struct SLuaCommand {
        SLuaCommand() : m_scriptIndex(0) {}
        // original f6348 first copies the name argument, then default-constructs the target and assigns it.
        SLuaCommand(XString name)
        {
            m_name = name;
            m_scriptIndex = 0;
        }
        // The original recursion and Super directly copy three members to form the by-value argument.
        SLuaCommand(const SLuaCommand& other, int scriptIndexIncrement)
            : m_name(other.m_name), m_parameter(other.m_parameter),
              m_scriptIndex(other.m_scriptIndex + scriptIndexIncrement) {}

        bool operator==(const SLuaCommand& other) const
        {
            if (!(m_name == other.m_name)) {
                return false;
            }
            if (!(m_parameter == other.m_parameter)) {
                return false;
            }
            return m_scriptIndex == other.m_scriptIndex;
        }

        XString m_name;          // +0
        CStrChar m_parameter;    // +4
        int m_scriptIndex;       // +0x14
    };

    CScriptedObject(const XString& scriptName);
    virtual void OnTimer(int timerId);           // slot +0
    virtual void Update(int elapsedMs);          // slot +4, the original does not use elapsedMs
    virtual bool IsRunCommandBlocked() { return false; } // slot +8, original 0xeb054: movs0/bx lr
    virtual void PushInstanceInLuaState(lua_State* state) = 0; // slot +0xc
    void RunLuaCommand(SLuaCommand command);
    void RunLuaCommand(XString name, CStrChar parameter, int scriptIndex);
    bool HasLuaMethod(const char* name);
    void AddLuaCommand(XString name);
    void AddLuaCommand(SLuaCommand command);
    void RunTimer(int, int); // 0x132b88
    void FlushTimer(int); // 0x132ba4
    bool IsTimerRunning(int); // 0x132bc0
    void SetBaseScript(CStrChar); // 0x132bdc
    void Super(); // 0x132bf8
    void SetParam(CStrChar, int); // 0x132c14
    int GetParam(CStrChar); // 0x132c30
    void SetParamStr(CStrChar, CStrChar); // 0x132c4c
    CStrChar GetParamStr(CStrChar); // 0x132c68
    void PlaySound(CStrChar); // 0x132c86
    void Print(CStrChar); // 0x132ca2
    int m_param1; // +4, property_rw at 0x132cb2
    // unsigned char m_data08[0x90 - 8]; // old unknown field range; real members restored below.
    HashTable<XString, XString> m_parameters; // +8
    int m_timers[16];                        // +0x14
    unsigned int m_lastUpdateMs;             // +0x54
    Vector<XString> m_scriptNames;           // +0x58
    SLuaCommand m_lastCommand;               // +0x68, name68/parameter6c/index7c
    Vector<SLuaCommand> m_pendingCommands;   // +0x80
};
#endif
