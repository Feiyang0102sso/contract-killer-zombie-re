/*
 * luabridge.cpp - non-template runtime of LuaBridge as built into CKZ 3.1.1.
 *
 * Based on LuaBridge 0.2 by Nathan Reed (indexer / newindexer / m_newindexer /
 * checkclass). Glu's changes, all taken from the original machine code:
 *   - the metamethods are named __index / __newindex / m_newindexer;
 *   - checkclass returns a DataHolder, saves the stack top on entry and restores
 *     it before returning, and builds its error text with CStrChar instead of
 *     snprintf into a 256-byte buffer;
 *   - DataHolder::getData returns inline storage when bit 2 of the type is set.
 */
#include <string.h>
#include "luabridge.h"

namespace luabridge {

/** Name used for classes that were never registered. */
const char* classname_unknown = "(unknown type)";

/** Push t[key] of the table at index (raw access, no metamethods). */
static inline void rawgetfield(lua_State* state, int index, const char* key)
{
    lua_pushstring(state, key);
    if (index < 0) {
        index = index - 1;
    }
    lua_rawget(state, index);
}

void* DataHolder::getData()
{
    // Bit 2: the object is stored inline right after the holder header.
    if (m_type & 4) {
        return (char*)this + 8;
    }
    return m_data;
}

/*
 * Class type checker. Given the index of a userdata on the stack, makes sure
 * that it is an object of the given class name or a subclass thereof, and
 * returns its holder. Works like luaL_checkudata.
 */
DataHolder* checkclass(lua_State* state, int index, const char* name, bool exact)
{
    DataHolder* holder = NULL;
    int savedTop = lua_gettop(state);

    // Make a relative index absolute so we can push our own values.
    if (index < 0) {
        index = index + lua_gettop(state) + 1;
    }

    // The value must be a userdata.
    if (!lua_isuserdata(state, index)) {
        luaL_typerror(state, index, name);
    }

    // Registered metatable of the class, then the metatable of the userdata.
    luaL_getmetatable(state, name);
    lua_getmetatable(state, index);

    if (exact) {
        // Ignore "const" for exact tests (used by destructors).
        if (!strncmp(name, "const ", 6)) {
            name = name + 6;
        }
        if (lua_rawequal(state, -1, -2)) {
            holder = (DataHolder*)lua_touserdata(state, index);
        } else {
            rawgetfield(state, -1, "__type");
            const char* actualType = lua_tostring(state, -1);
            CStrChar message("");
            message += name;
            message += " expected, got ";
            message += actualType;
            // luaL_argerror does not return.
            luaL_argerror(state, index, message.GetCharPtr());
        }
        lua_settop(state, savedTop);
        return holder;
    }

    // Walk up the chain of parent metatables.
    while (!lua_rawequal(state, -1, -2)) {
        // The const metatable of the class also matches.
        rawgetfield(state, -1, "__const");
        if (!lua_isnil(state, -1)) {
            if (lua_rawequal(state, -1, -3)) {
                break;
            }
        }
        lua_pop(state, 1);

        // No parent: report the actual type of the userdata.
        rawgetfield(state, -1, "__parent");
        if (lua_isnil(state, -1)) {
            lua_getmetatable(state, index);
            rawgetfield(state, -1, "__type");
            const char* actualType = lua_tostring(state, -1);
            CStrChar message("");
            message += name;
            message += " expected, got ";
            message += actualType;
            // luaL_argerror does not return.
            luaL_argerror(state, index, message.GetCharPtr());
            lua_settop(state, savedTop);
            return holder;
        }

        // Replace the metatable with its parent.
        lua_remove(state, -2);
    }

    holder = (DataHolder*)lua_touserdata(state, index);
    lua_settop(state, savedTop);
    return holder;
}

/*
 * __index metamethod for objects: method, then property getter, then const
 * method, then the same lookup in the parent class.
 */
int __index(lua_State* state)
{
    lua_getmetatable(state, 1);

    while (true) {
        // The key in the metatable itself.
        lua_pushvalue(state, 2);
        lua_rawget(state, -2);
        if (!lua_isnil(state, -1)) {
            return 1;
        }
        lua_pop(state, 1);

        // A property getter: call it with the object.
        rawgetfield(state, -1, "__propget");
        if (!lua_isnil(state, -1)) {
            lua_pushvalue(state, 2);
            lua_rawget(state, -2);
            if (!lua_isnil(state, -1)) {
                lua_pushvalue(state, 1);
                lua_call(state, 1, 1);
                return 1;
            }
            lua_pop(state, 1);
        }
        lua_pop(state, 1);

        // The const metatable.
        rawgetfield(state, -1, "__const");
        if (!lua_isnil(state, -1)) {
            lua_pushvalue(state, 2);
            lua_rawget(state, -2);
            if (!lua_isnil(state, -1)) {
                return 1;
            }
            lua_pop(state, 1);
        }
        lua_pop(state, 1);

        // Repeat on the parent class; no parent returns nil.
        rawgetfield(state, -1, "__parent");
        if (lua_isnil(state, -1)) {
            return 1;
        }
        lua_remove(state, -2);
    }
}

/*
 * __newindex metamethod for static tables: call the property setter with the
 * value only.
 */
int __newindex(lua_State* state)
{
    lua_getmetatable(state, 1);

    while (true) {
        rawgetfield(state, -1, "__propset");
        if (!lua_isnil(state, -1)) {
            lua_pushvalue(state, 2);
            lua_rawget(state, -2);
            if (!lua_isnil(state, -1)) {
                lua_pushvalue(state, 3);
                lua_call(state, 1, 0);
                return 0;
            }
            lua_pop(state, 1);
        }
        lua_pop(state, 1);

        rawgetfield(state, -1, "__parent");
        if (lua_isnil(state, -1)) {
            return luaL_error(state, "attempt to set %s, which isn't a property", lua_tostring(state, 2));
        }
        lua_remove(state, -2);
    }
}

/*
 * __newindex metamethod for objects: call the property setter with the object
 * and the value.
 */
int m_newindexer(lua_State* state)
{
    lua_getmetatable(state, 1);

    while (true) {
        rawgetfield(state, -1, "__propset");
        if (!lua_isnil(state, -1)) {
            lua_pushvalue(state, 2);
            lua_rawget(state, -2);
            if (!lua_isnil(state, -1)) {
                lua_pushvalue(state, 1);
                lua_pushvalue(state, 3);
                lua_call(state, 2, 0);
                return 0;
            }
            lua_pop(state, 1);
        }
        lua_pop(state, 1);

        rawgetfield(state, -1, "__parent");
        if (lua_isnil(state, -1)) {
            return luaL_error(state, "attempt to set %s, which isn't a property", lua_tostring(state, 2));
        }
        lua_remove(state, -2);
    }
}

}
