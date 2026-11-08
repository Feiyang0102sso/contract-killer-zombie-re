// PROVISIONAL (created by b077): header-only game template, no owning unit (weak Set in src/CGameApp.cpp, Get in src/bounty_hunter/CShopItem.cpp)
// DataProtection.h - values kept xor-scrambled in memory (anti memory-editing).
// Encrypted<T> stores the bytes of T shifted by a key from CRandGen (bodies below, by b055x);
// Set / Get are not written inline, so the users call them out of line as the original does.
#ifndef DATAPROTECTION_H
#define DATAPROTECTION_H

#include <string.h>  // added by b077
#include "CRandGen.h"  // added by b055x

template <class T>
class Encrypted {
public:
    /** Zero (the value is cleared with memset first, as in CItem::CItem). */  // added by b077
    Encrypted()  // added by b077
    {  // added by b077
        T value;  // added by b077
        memset(&value, 0, sizeof(T));  // added by b077
        Set(value);  // added by b077
    }  // added by b077

    Encrypted(const T& value)  // added by b077
    {  // added by b077
        Set(value);  // added by b077
    }  // added by b077

    /** Scramble `value` with a fresh key. */
    void Set(const T& value);
    /** Unscrambled value. */
    T Get() const;

    /** Copy re-scrambles with a fresh key (b145: inlined in CItem::ParseOverridable). */
    Encrypted& operator=(const Encrypted& other) { Set(other.Get()); return *this; }

    T m_scrambled;          // 0x00
    unsigned int m_key;     // 0x04
};

// Bodies reconstructed by b055x from Encrypted<bool>::Set (CDH_Weapon.o), Encrypted<int>::Set (CGameApp.o)  // added by b055x
// and Encrypted<int>::Get (CShopItem.o).  // added by b055x
template <class T>  // added by b055x
void Encrypted<T>::Set(const T& value)  // added by b055x
{  // added by b055x
    unsigned int key = com::glu::platform::core::CRandGen::GetInstance()->Generate();  // added by b055x
    m_key = key;  // added by b055x
    for (int index = 0; index < (int)sizeof(T); index++) {  // added by b055x
        ((unsigned char*)&m_scrambled)[index] = ((const unsigned char*)&value)[index] + key % 255;  // added by b055x
        key = (unsigned int)(((unsigned long long)key * 22695477 + 1) % 0xffffffffULL);  // added by b055x
    }  // added by b055x
}  // added by b055x

template <class T>  // added by b055x
T Encrypted<T>::Get() const  // added by b055x
{  // added by b055x
    T value;  // added by b055x
    unsigned int key = m_key;  // added by b055x
    for (int index = 0; index < (int)sizeof(T); index++) {  // added by b055x
        ((unsigned char*)&value)[index] = ((const unsigned char*)&m_scrambled)[index] - key % 255;  // added by b055x
        key = (unsigned int)(((unsigned long long)key * 22695477 + 1) % 0xffffffffULL);  // added by b055x
    }  // added by b055x
    return value;  // added by b055x
}  // added by b055x

#endif
