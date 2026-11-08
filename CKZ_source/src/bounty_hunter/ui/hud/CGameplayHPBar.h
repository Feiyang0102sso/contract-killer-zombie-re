// k: only the one method original 150a2c; no evidence of constructor/allocation/base class found, so no allocatable full layout is declared.
#ifndef CGAMEPLAYHPBAR_H
#define CGAMEPLAYHPBAR_H
class CGameplayHPBar {
public:
    void SetPercent(float percent);
private:
    CGameplayHPBar(); // View of an existing instance; creating objects from this thin declaration is forbidden.
};
#endif
