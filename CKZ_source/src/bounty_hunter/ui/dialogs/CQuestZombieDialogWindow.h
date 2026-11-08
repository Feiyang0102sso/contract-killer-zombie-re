// PROVISIONAL (created by b079): owner src/bounty_hunter/ui/dialogs/CQuestZombieDialogWindow.cpp not written yet
// Base call @0x14e802; allocation size 0x118 at CTutorialManager @0x13c1b0.
#ifndef CQUESTZOMBIEDIALOGWINDOW_H
#define CQUESTZOMBIEDIALOGWINDOW_H

#include "CZombieDialogWindow.h"
#include "CQuestManager.h"

class CQuestZombieDialogWindow : public CZombieDialogWindow {
public:
    explicit CQuestZombieDialogWindow(const CQuestManager::SDialog& dialog);
    virtual ~CQuestZombieDialogWindow() {} // Original 14e904/14e914 only do Zombie destruct/free.
};

#endif
