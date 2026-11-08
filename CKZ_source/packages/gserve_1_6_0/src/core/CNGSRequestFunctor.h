// CNGSRequestFunctor.h - request functor that forwards the reply to a member function of its owner.
// Header-only template (created by b028x for offers/CNGSOfferManager.cpp, the first user; the original
// N_SOL names this file). Other gServe managers may instantiate it for their own classes.
#ifndef CNGSREQUESTFUNCTOR_H
#define CNGSREQUESTFUNCTOR_H

#include <stddef.h>
#include "CNGSServerRequest.h"

/** Calls (m_object->*m_callback)(response, this) when the reply arrives (or on timeout / cancel). */
template <class T>
class CNGSServerRequestCompleteFunctor : public CNGSServerRequestFunctor {
public:
    typedef void (T::*Callback)(CObjectMap* response, CNGSServerRequestFunctor* functor);

    CNGSServerRequestCompleteFunctor(T* object, Callback callback)
        : m_object(object)
        , m_callback(callback)
    {
    }

    virtual ~CNGSServerRequestCompleteFunctor() {}

    virtual void requestCompleteCallback(CObjectMap* response)
    {
        if (m_callback) {
            (m_object->*m_callback)(response, this);
        }
    }

    T* m_object;            // +0x08
    Callback m_callback;    // +0x0c pointer to member (function, this adjustment)
};

#endif
