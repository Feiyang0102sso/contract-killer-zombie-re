// b074i: original 159654 returns Vector<int> via hidden return; this view is only for external member calls.
#ifndef CINCENTIVIZEDWINDOW_H
#define CINCENTIVIZEDWINDOW_H
#include "Vector_ss.h"
class CIncentivizedWindow {
public:
    Vector<int> GetSelectedIds() const;
};
extern const int EventId_TargettingMissionFriendSelected;
#endif
