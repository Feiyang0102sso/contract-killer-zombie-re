// LuaBridge stack conversions visible in CScriptManager.o (0x13653c onward).
#ifndef CKZ_LUABRIDGE_STACK_HPP
#define CKZ_LUABRIDGE_STACK_HPP

#include "lua.h"
#include "lauxlib.h"
#include "CStrChar.h"
#include "xString.h"

namespace luabridge {
using com::glu::platform::components::CStrChar;

// The non-template bridge runtime remains an external dependency until its owner is restored.
extern const char* classname_unknown;
class DataHolder {
public:
    void* getData();
    int m_type;
    void* m_data;
};
DataHolder* checkclass(lua_State* state, int index, const char* name, bool exact);
int __index(lua_State* state);
int __newindex(lua_State* state);
int m_newindexer(lua_State* state);

template<class T> struct classname {
    static const char* name_;
};
template<class T> const char* classname<T>::name_ = classname_unknown;

/** The nested template name and pointer layout come from the original symbols/instructions. */
struct TypeTable {
    template<class T> struct tdstack {
        /** Original reference wrapper at 0xf8374; ownership stays with the caller. */
        static void push(lua_State* state, const T& value, int type = 0)
        {
            DataHolder* holder = (DataHolder*)lua_newuserdata(state, sizeof(DataHolder));
            holder->m_type = type;
            holder->m_data = const_cast<T*>(&value);
            lua_getfield(state, LUA_REGISTRYINDEX, classname<T>::name_);
            lua_setmetatable(state, -2);
        }
    };
};

template<> struct TypeTable::tdstack<int> {
    static int get(lua_State* state, int index) { return luaL_checkinteger(state, index); }
    static void push(lua_State* state, int value) { lua_pushinteger(state, value); }
};
template<> struct TypeTable::tdstack<bool> {
    static bool get(lua_State* state, int index)
    {
        // 0x136684 / 0x1374dc: reject non-boolean Lua arguments before converting them.
        luaL_checktype(state, index, 1);
        return lua_toboolean(state, index) != 0;
    }
    static void push(lua_State* state, bool value) { lua_pushboolean(state, value); }
};
template<> struct TypeTable::tdstack<float> {
    static float get(lua_State* state, int index) { return luaL_checknumber(state, index); }
    static void push(lua_State* state, float value) { lua_pushnumber(state, value); }
};
template<> struct TypeTable::tdstack<const char*> {
    static const char* get(lua_State* state, int index) { return luaL_checklstring(state, index, NULL); }
};
template<> struct TypeTable::tdstack<CStrChar> {
    static CStrChar get(lua_State* state, int index) { return CStrChar(luaL_checklstring(state, index, NULL)); }
    static void push(lua_State* state, const CStrChar& value) { lua_pushstring(state, value.GetCharPtr()); }
};
template<> struct TypeTable::tdstack<XString> {
    static XString get(lua_State* state, int index) { return XString(luaL_checklstring(state, index, NULL)); }
    static void push(lua_State* state, const XString& value)
    {
        XString::AnsiString text(value.c_str());
        lua_pushstring(state, text);
    }
};
template<> struct TypeTable::tdstack<const XString&> {
    static XString get(lua_State* state, int index) { return XString(luaL_checklstring(state, index, NULL)); }
};
template<class T> struct TypeTable::tdstack<T*> {
    static void push(lua_State* state, T* value, int type = 0)
    {
        DataHolder* holder = (DataHolder*)lua_newuserdata(state, sizeof(DataHolder));
        holder->m_type = type;
        holder->m_data = value;
        lua_getfield(state, LUA_REGISTRYINDEX, classname<T>::name_);
        lua_setmetatable(state, -2);
    }
};

} // namespace luabridge
#endif
