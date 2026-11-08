// LuaBridge function pointer adapters: signatures and calls from CScriptManager ref.
#ifndef CKZ_LUABRIDGE_TYPELIST_HPP
#define CKZ_LUABRIDGE_TYPELIST_HPP
#include "stack.hpp"
namespace luabridge {
template<class F> struct fnptr;
template<class R>
struct fnptr<R (*)()> {
    typedef R return_type;
    typedef R (*function_type)();
    static R apply(function_type function, lua_State* state, int first)
    {
        return function();
    }
};
template<class C, class R>
struct fnptr<R (C::*)()> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)();
    static R apply(C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)();
    }
};
template<class C, class R>
struct fnptr<R (C::*)() const> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)() const;
    static R apply(const C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)();
    }
};
template<class R, class A1>
struct fnptr<R (*)(A1)> {
    typedef R return_type;
    typedef R (*function_type)(A1);
    static R apply(function_type function, lua_State* state, int first)
    {
        return function(TypeTable::tdstack<A1>::get(state, first + 0));
    }
};
template<class C, class R, class A1>
struct fnptr<R (C::*)(A1)> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)(A1);
    static R apply(C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)(TypeTable::tdstack<A1>::get(state, first + 0));
    }
};
template<class C, class R, class A1>
struct fnptr<R (C::*)(A1) const> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)(A1) const;
    static R apply(const C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)(TypeTable::tdstack<A1>::get(state, first + 0));
    }
};
template<class R, class A1, class A2>
struct fnptr<R (*)(A1, A2)> {
    typedef R return_type;
    typedef R (*function_type)(A1, A2);
    static R apply(function_type function, lua_State* state, int first)
    {
        return function(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1));
    }
};
template<class C, class R, class A1, class A2>
struct fnptr<R (C::*)(A1, A2)> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)(A1, A2);
    static R apply(C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1));
    }
};
template<class C, class R, class A1, class A2>
struct fnptr<R (C::*)(A1, A2) const> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)(A1, A2) const;
    static R apply(const C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1));
    }
};
template<class R, class A1, class A2, class A3>
struct fnptr<R (*)(A1, A2, A3)> {
    typedef R return_type;
    typedef R (*function_type)(A1, A2, A3);
    static R apply(function_type function, lua_State* state, int first)
    {
        return function(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1), TypeTable::tdstack<A3>::get(state, first + 2));
    }
};
template<class C, class R, class A1, class A2, class A3>
struct fnptr<R (C::*)(A1, A2, A3)> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)(A1, A2, A3);
    static R apply(C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1), TypeTable::tdstack<A3>::get(state, first + 2));
    }
};
template<class C, class R, class A1, class A2, class A3>
struct fnptr<R (C::*)(A1, A2, A3) const> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)(A1, A2, A3) const;
    static R apply(const C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1), TypeTable::tdstack<A3>::get(state, first + 2));
    }
};
template<class R, class A1, class A2, class A3, class A4>
struct fnptr<R (*)(A1, A2, A3, A4)> {
    typedef R return_type;
    typedef R (*function_type)(A1, A2, A3, A4);
    static R apply(function_type function, lua_State* state, int first)
    {
        return function(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1), TypeTable::tdstack<A3>::get(state, first + 2), TypeTable::tdstack<A4>::get(state, first + 3));
    }
};
template<class C, class R, class A1, class A2, class A3, class A4>
struct fnptr<R (C::*)(A1, A2, A3, A4)> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)(A1, A2, A3, A4);
    static R apply(C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1), TypeTable::tdstack<A3>::get(state, first + 2), TypeTable::tdstack<A4>::get(state, first + 3));
    }
};
template<class C, class R, class A1, class A2, class A3, class A4>
struct fnptr<R (C::*)(A1, A2, A3, A4) const> {
    typedef R return_type;
    typedef C class_type;
    typedef R (C::*function_type)(A1, A2, A3, A4) const;
    static R apply(const C* object, function_type function, lua_State* state, int first)
    {
        return (object->*function)(TypeTable::tdstack<A1>::get(state, first + 0), TypeTable::tdstack<A2>::get(state, first + 1), TypeTable::tdstack<A3>::get(state, first + 2), TypeTable::tdstack<A4>::get(state, first + 3));
    }
};
} // namespace luabridge
#endif
