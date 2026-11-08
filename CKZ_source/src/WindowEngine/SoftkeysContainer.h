// PROVISIONAL (created by b055x): SoftkeysContainer is defined inside src/WindowEngine/WindowApp.cpp
// SoftkeysContainer.h - declaration for the screens that create a soft key bar (only the constructor
// and the size are needed; the vtable stays in WindowApp.cpp because Init() is its key function).
#ifndef SOFTKEYSCONTAINER_H
#define SOFTKEYSCONTAINER_H

#include "WindowBase.h"

class SoftkeysContainer : public Window {
public:
    SoftkeysContainer(Window* owner);

    virtual void Init();

    Window* m_owner;                        // 0xac
    bool m_active;                          // 0xb0 (see WindowApp.cpp)
};

#endif
