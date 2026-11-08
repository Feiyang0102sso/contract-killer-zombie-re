// LuaBridge registration and dispatch recovered from CScriptManager's complete template ranges.
#ifndef CKZ_LUABRIDGE_MODULE_HPP
#define CKZ_LUABRIDGE_MODULE_HPP
#include <string.h>
#include "typelist.hpp"

namespace luabridge {

template<class F, class R = typename fnptr<F>::return_type> struct function_proxy {
    static int f(lua_State* state)
    {
        F function = (F)lua_touserdata(state, lua_upvalueindex(1));
        TypeTable::tdstack<R>::push(state, fnptr<F>::apply(function, state, 1));
        return 1;
    }
};
template<class F> struct function_proxy<F, void> {
    static int f(lua_State* state)
    {
        F function = (F)lua_touserdata(state, lua_upvalueindex(1));
        fnptr<F>::apply(function, state, 1);
        return 0;
    }
};

template<class F, class R = typename fnptr<F>::return_type> struct method_proxy {
    static int f(lua_State* state)
    {
        const char* name = lua_tostring(state, lua_upvalueindex(1));
        DataHolder* holder = checkclass(state, 1, name, false);
        typename fnptr<F>::class_type* object = (typename fnptr<F>::class_type*)holder->getData();
        F* function = (F*)lua_touserdata(state, lua_upvalueindex(2));
        TypeTable::tdstack<R>::push(state, fnptr<F>::apply(object, *function, state, 2));
        return 1;
    }
};
template<class F> struct method_proxy<F, void> {
    static int f(lua_State* state)
    {
        const char* name = lua_tostring(state, lua_upvalueindex(1));
        DataHolder* holder = checkclass(state, 1, name, false);
        typename fnptr<F>::class_type* object = (typename fnptr<F>::class_type*)holder->getData();
        F* function = (F*)lua_touserdata(state, lua_upvalueindex(2));
        fnptr<F>::apply(object, *function, state, 2);
        return 0;
    }
};

template<class T> int destructor_dispatch(lua_State* state)
{
    // Original dispatch only asks checkclass to handle destruction; it does not directly delete T.
    const char* name = lua_tostring(state, lua_upvalueindex(1));
    checkclass(state, 1, name, true);
    return 0;
}
template<class C, class T> int m_propget_proxy(lua_State* state)
{
    const char* name = lua_tostring(state, lua_upvalueindex(1));
    DataHolder* holder = checkclass(state, 1, name, false);
    C* object = (C*)holder->getData();
    T C::** member = (T C::**)lua_touserdata(state, lua_upvalueindex(2));
    TypeTable::tdstack<T>::push(state, object->**member);
    return 1;
}
template<class C, class T> int m_propset_proxy(lua_State* state)
{
    const char* name = lua_tostring(state, lua_upvalueindex(1));
    DataHolder* holder = checkclass(state, 1, name, false);
    C* object = (C*)holder->getData();
    T C::** member = (T C::**)lua_touserdata(state, lua_upvalueindex(2));
    T C::* destination = *member;
    T value = TypeTable::tdstack<T>::get(state, 2);
    object->*destination = value;
    return 0;
}

/** Set a raw table entry while preserving the original push/insert/rawset stack protocol. */
inline void raw_set_field(lua_State* state, int table, const char* name)
{
    lua_pushstring(state, name);
    lua_insert(state, -2);
    lua_rawset(state, table);
}

inline void raw_get_field(lua_State* state, int table, const char* name)
{
    lua_pushstring(state, name);
    lua_rawget(state, table);
}

template<class T> void create_metatable(lua_State* state, const char* name)
{
    luaL_newmetatable(state, name);
    lua_pushcfunction(state, __index);
    raw_set_field(state, -3, "__index");
    lua_pushcfunction(state, m_newindexer);
    raw_set_field(state, -3, "__newindex");
    lua_pushstring(state, name);
    lua_pushcclosure(state, destructor_dispatch<T>, 1);
    raw_set_field(state, -3, "__gc");
    lua_pushstring(state, name);
    raw_set_field(state, -3, "__type");
    lua_createtable(state, 0, 0);
    raw_set_field(state, -3, "__propget");
    lua_createtable(state, 0, 0);
    raw_set_field(state, -3, "__propset");
}
template<class T> void create_const_metatable(lua_State* state, const char* name)
{
    CStrChar constName = CStrChar("const ") + name;
    luaL_newmetatable(state, constName.GetCharPtr());
    lua_pushcfunction(state, __index);
    raw_set_field(state, -3, "__index");
    lua_pushcfunction(state, m_newindexer);
    raw_set_field(state, -3, "__newindex");
    lua_pushstring(state, constName.GetCharPtr());
    lua_pushcclosure(state, destructor_dispatch<T>, 1);
    raw_set_field(state, -3, "__gc");
    lua_pushstring(state, constName.GetCharPtr());
    raw_set_field(state, -3, "__type");
    lua_createtable(state, 0, 0);
    raw_set_field(state, -3, "__propget");
    // The original const metatable deliberately has no __propset table (0x136b3c).
}
template<class T> void create_static_table(lua_State* state, const char* name)
{
    lua_createtable(state, 0, 0);
    lua_pushvalue(state, -1);
    lua_setmetatable(state, -2);
    lua_pushcfunction(state, __index);
    raw_set_field(state, -3, "__index");
    lua_pushcfunction(state, __newindex);
    raw_set_field(state, -3, "__newindex");
    lua_createtable(state, 0, 0);
    raw_set_field(state, -3, "__propget");
    lua_createtable(state, 0, 0);
    raw_set_field(state, -3, "__propset");
    lua_pushvalue(state, -1);
    lua_setfield(state, LUA_GLOBALSINDEX, name);
}

template<class T> class class__ {
public:
    class__(lua_State* state, const char* name) : m_state(state)
    {
        classname<T>::name_ = name;
        create_metatable<T>(m_state, name);
        create_const_metatable<T>(m_state, name);
        raw_set_field(m_state, -3, "__const");
        lua_pop(m_state, 1);
        create_static_table<T>(m_state, name);
        lua_pop(m_state, 1);
    }
    class__(lua_State* state, const char* name, const char* parent) : m_state(state)
    {
        classname<T>::name_ = name;
        create_metatable<T>(m_state, name);
        lua_getfield(m_state, LUA_REGISTRYINDEX, parent);
        raw_set_field(m_state, -3, "__parent");
        create_const_metatable<T>(m_state, name);
        CStrChar constParent = CStrChar("const ") + parent;
        lua_getfield(m_state, LUA_REGISTRYINDEX, constParent.GetCharPtr());
        raw_set_field(m_state, -3, "__parent");
        raw_set_field(m_state, -3, "__const");
        lua_pop(m_state, 1);
        create_static_table<T>(m_state, name);
        lua_getfield(m_state, LUA_GLOBALSINDEX, parent);
        raw_set_field(m_state, -3, "__parent");
        lua_pop(m_state, 1);
    }
    template<class F> class__& method(const char* name, F function)
    {
        // Mutable member functions are put in the const-named table too, as in 0x133e28.
        CStrChar tableName(classname<T>::name_);
        tableName = "const " + tableName;
        lua_getfield(m_state, LUA_REGISTRYINDEX, tableName.GetCharPtr());
        lua_pushstring(m_state, tableName.GetCharPtr());
        void* stored = lua_newuserdata(m_state, sizeof(F));
        memcpy(stored, &function, sizeof(F));
        lua_pushcclosure(m_state, method_proxy<F>::f, 2);
        raw_set_field(m_state, -3, name);
        lua_pop(m_state, 1);
        return *this;
    }
    template<class R> class__& property_ro(const char* name, R (T::*function)() const)
    {
        lua_getfield(m_state, LUA_REGISTRYINDEX, classname<T>::name_);
        CStrChar constName = CStrChar("const ") + classname<T>::name_;
        lua_getfield(m_state, LUA_REGISTRYINDEX, constName.GetCharPtr());
        raw_get_field(m_state, -3, "__propget");
        raw_get_field(m_state, -3, "__propget");
        lua_pushstring(m_state, constName.GetCharPtr());
        void* stored = lua_newuserdata(m_state, sizeof(function));
        memcpy(stored, &function, sizeof(function));
        lua_pushcclosure(m_state, method_proxy<R (T::*)() const>::f, 2);
        lua_pushvalue(m_state, -1);
        raw_set_field(m_state, -4, name);
        raw_set_field(m_state, -4, name);
        lua_pop(m_state, 4);
        return *this;
    }
    template<class R> class__& property_ro(const char* name, R T::*member)
    {
        lua_getfield(m_state, LUA_REGISTRYINDEX, classname<T>::name_);
        CStrChar constName = CStrChar("const ") + classname<T>::name_;
        lua_getfield(m_state, LUA_REGISTRYINDEX, constName.GetCharPtr());
        raw_get_field(m_state, -3, "__propget");
        raw_get_field(m_state, -3, "__propget");
        lua_pushstring(m_state, constName.GetCharPtr());
        void* stored = lua_newuserdata(m_state, sizeof(member));
        memcpy(stored, &member, sizeof(member));
        lua_pushcclosure(m_state, m_propget_proxy<T, R>, 2);
        lua_pushvalue(m_state, -1);
        raw_set_field(m_state, -4, name);
        raw_set_field(m_state, -4, name);
        lua_pop(m_state, 4);
        return *this;
    }
    template<class R> class__& property_rw(const char* name, R T::*member)
    {
        property_ro(name, member);
        lua_getfield(m_state, LUA_REGISTRYINDEX, classname<T>::name_);
        raw_get_field(m_state, -2, "__propset");
        lua_pushstring(m_state, classname<T>::name_);
        void* stored = lua_newuserdata(m_state, sizeof(member));
        memcpy(stored, &member, sizeof(member));
        lua_pushcclosure(m_state, m_propset_proxy<T, R>, 2);
        raw_set_field(m_state, -3, name);
        lua_pop(m_state, 2);
        return *this;
    }
    lua_State* m_state;
};

class module {
public:
    explicit module(lua_State* state) : m_state(state) {}
    template<class T> class__<T> class_(const char* name) { return class__<T>(m_state, name); }
    template<class T, class Parent> class__<T> class_(const char* name)
    {
        return class__<T>(m_state, name, classname<Parent>::name_);
    }
    template<class F> module& function(const char* name, F function)
    {
        lua_pushlightuserdata(m_state, (void*)function);
        lua_pushcclosure(m_state, function_proxy<F>::f, 1);
        lua_setfield(m_state, LUA_GLOBALSINDEX, name);
        return *this;
    }
    lua_State* m_state;
};

} // namespace luabridge
#endif
