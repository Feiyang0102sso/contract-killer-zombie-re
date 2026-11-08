// CScriptedObject: original Lua script inheritance, deferred commands, timers and script parameters.
#include "CScriptedObject.h"
#include "App.h"
#include "ResMap.h"
#include "CDH_SoundManager.h"
#include "CStdUtil_Cocoa.h"
#include "lua.h"
#include <string.h>

namespace {
enum LuaValueType {
    LuaNil = 0,
    LuaFunction = 6
};
}

CScriptedObject::CScriptedObject(const XString& scriptName)
    : m_parameters(8)
{
    m_param1 = 0;
    m_scriptNames.addElement(scriptName);
    // original f5ee0 calls libc _memset directly; HashTable memory operations still use the prefix np_*.
    __builtin_memset(m_timers, 0xff, sizeof(m_timers));
    m_lastUpdateMs = CStdUtil_Cocoa::GetUpTimeMS();
}

void CScriptedObject::Update(int)
{
    // original f5f02 converts each command name when the queue is >=6; the conversion result is unused, but the allocation/free still exists.
    if ((unsigned int)m_pendingCommands.size() >= 6) {
        for (int index = 0; index < m_pendingCommands.size(); index++) {
            XString::AnsiString name(m_pendingCommands[index].m_name.c_str());
        }
    }
    if (m_pendingCommands.size() > 0) {
        Vector<SLuaCommand> commands(m_pendingCommands);
        m_pendingCommands.removeAllElements();
        for (int index = 0; index < commands.size(); index++) {
            RunLuaCommand(commands[index]);
        }
    }

    // original f5fdc/f5fe8/f5ff0 each read the time separately; the unsigned wraparound path does not compute a negative time.
    int elapsed = 0;
    if ((unsigned int)CStdUtil_Cocoa::GetUpTimeMS() > m_lastUpdateMs) {
        elapsed = CStdUtil_Cocoa::GetUpTimeMS() - m_lastUpdateMs;
    }
    m_lastUpdateMs = CStdUtil_Cocoa::GetUpTimeMS();
    for (int index = 0; index < 16; index++) {
        if (m_timers[index] >= 0) {
            m_timers[index] -= elapsed;
            if (m_timers[index] <= 0) {
                OnTimer(index);
            }
        }
    }
}

void CScriptedObject::RunLuaCommand(SLuaCommand command)
{
    if (IsRunCommandBlocked()) {
        return;
    }
    if (m_scriptNames.size() <= command.m_scriptIndex) {
        return;
    }
    lua_State* state = ((App*)WindowApp::m_instance)->m_lua;
    lua_gettop(state);
    {
        XString::AnsiString scriptName(m_scriptNames[command.m_scriptIndex].c_str());
        lua_getfield(state, LUA_GLOBALSINDEX, scriptName);
    }
    if (lua_type(state, -1) != LuaNil) {
        {
            XString::AnsiString commandName(command.m_name.c_str());
            lua_pushstring(state, commandName);
        }
        lua_gettable(state, -2);
        if (lua_type(state, -1) == LuaFunction) {
            PushInstanceInLuaState(state);
            int argumentCount = 1;
            if (command.m_parameter.GetLength() > 0) {
                argumentCount = 2;
                lua_pushstring(state, command.m_parameter.GetCharPtr());
            }
            m_lastCommand = command;
            // The original does not check the pcall return code, and the pop order of the error object and table is not additionally corrected.
            lua_pcall(state, argumentCount, 0, 0);
        }
        else {
            RunLuaCommand(SLuaCommand(command, 1));
            lua_pop(state, 1);
        }
    }
    lua_pop(state, 1);
    lua_gettop(state);
}

void CScriptedObject::RunLuaCommand(XString name, CStrChar parameter, int scriptIndex)
{
    SLuaCommand command;
    command.m_name = name;
    command.m_parameter = parameter;
    command.m_scriptIndex = scriptIndex;
    RunLuaCommand(command);
}

bool CScriptedObject::HasLuaMethod(const char* name)
{
    bool found = false;
    if (m_scriptNames.size() > 0) {
        lua_State* state = ((App*)WindowApp::m_instance)->m_lua;
        int index = 0;
        do {
            lua_gettop(state);
            {
                XString::AnsiString scriptName(m_scriptNames[index].c_str());
                lua_getfield(state, LUA_GLOBALSINDEX, scriptName);
            }
            if (lua_type(state, -1) != LuaNil) {
                lua_pushstring(state, name);
                lua_gettable(state, -2);
                if (lua_type(state, -1) == LuaFunction) {
                    found = true;
                }
            }
            // original f631a..f6320 also pops two elements on the nil path; not changed to restore the starting top.
            lua_pop(state, 2);
            lua_gettop(state);
            index++;
        } while (index < m_scriptNames.size() && !found);
    }
    return found;
}

void CScriptedObject::AddLuaCommand(XString name)
{
    AddLuaCommand(SLuaCommand(name));
}

void CScriptedObject::AddLuaCommand(SLuaCommand command)
{
    if (m_pendingCommands.indexOf(command, 0) == -1) {
        m_pendingCommands.addElement(command);
    }
}

void CScriptedObject::OnTimer(int timerId)
{
    m_timers[timerId] = -1;
    CStrChar parameter(XString::AnsiString(XString(timerId).c_str()));
    RunLuaCommand(XString("OnTimer"), parameter, 0);
}

void CScriptedObject::FlushTimer(int timerId)
{
    m_timers[timerId] = -1;
}

void CScriptedObject::SetBaseScript(CStrChar scriptName)
{
    XString name(scriptName.GetCharPtr());
    int index = m_lastCommand.m_scriptIndex + 1;
    if (m_scriptNames.size() == index) {
        m_scriptNames.addElement(name);
    }
    else if (m_scriptNames.size() > index) {
        m_scriptNames[index] = name;
    }
}

void CScriptedObject::Super()
{
    RunLuaCommand(SLuaCommand(m_lastCommand, 1));
}

void CScriptedObject::PlaySound(CStrChar soundName)
{
    XString name(soundName.GetCharPtr());
    App* app = (App*)WindowApp::m_instance;
    const char* resource = app->m_resMap->getResourceID(name);
    int sound = CDH_SoundManager::GetSoundFromResourceID(resource);
    if (sound != CDH_SoundManager::SOUND_COUNT) {
        CDH_SoundManager* manager = ((App*)WindowApp::m_instance)->m_soundManager;
        if (!manager->IsPlaying((CDH_SoundManager::eSound)sound)) {
            ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)sound, false, false);
        }
    }
}

void CScriptedObject::SetParam(CStrChar parameterName, int value)
{
    XString name(parameterName.GetCharPtr());
    XString text(value);
    m_parameters.Set(name, text);
}

int CScriptedObject::GetParam(CStrChar parameterName)
{
    XString* value;
    {
        XString name(parameterName.GetCharPtr());
        value = m_parameters.Get(name);
    }
    if (value != NULL) {
        return value->ToInt();
    }
    return 0;
}

bool CScriptedObject::IsTimerRunning(int timerId)
{
    if (timerId >= 0 && timerId < sizeof(m_timers) / sizeof(m_timers[0])) {
        return m_timers[timerId] > -1;
    }
    return false;
}

void CScriptedObject::RunTimer(int timerId, int duration)
{
    m_timers[timerId] = duration;
}

void CScriptedObject::Print(CStrChar)
{
    // original 0xf6674..0xf6678 is only bx lr/nop; the real by-value destruction of the arguments is done by the caller.
}

void CScriptedObject::SetParamStr(CStrChar parameterName, CStrChar value)
{
    XString name(parameterName.GetCharPtr());
    XString text(value.GetCharPtr());
    m_parameters.Set(name, text);
}

CStrChar CScriptedObject::GetParamStr(CStrChar parameterName)
{
    XString* value;
    {
        XString name(parameterName.GetCharPtr());
        value = m_parameters.Get(name);
    }
    if (value == NULL) {
        return CStrChar("");
    }
    CStrChar text(XString::AnsiString(value->c_str()));
    return CStrChar(text);
}
