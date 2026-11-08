// b074i: one of the three listener bases of original CBH_GPSMap; only the real listener interface is recovered, the Bar body is not counted as done.
#ifndef CGPSBAR_H
#define CGPSBAR_H
class CGPSBar;
class CGPSBarListener {
public:
    // The original single-slot listener base has no virtual destructor; Map's +b0 address point and the -176 thunk.
    virtual void OnBarItemSelected(CGPSBar* bar) = 0;
};
#endif
