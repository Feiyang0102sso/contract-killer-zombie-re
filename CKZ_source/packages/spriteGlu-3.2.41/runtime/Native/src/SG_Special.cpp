// SG_Special.cpp - default presenter hooks (see SG_Special.h). (written by b042)
#include "SG_Special.h"
#include "SG_Home.h"

unsigned char SG_Special::DrawSpecialFramePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame)
{
    SG_Home::GetInstance();
    return 0;
}

void SG_Special::DrawSpecialFramePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame)
{
}

unsigned char SG_Special::DrawSpecialLayerPre(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer)
{
    return 0;
}

void SG_Special::DrawSpecialLayerPost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer)
{
}

unsigned char SG_Special::DrawSpecialSpritePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer,
                                               int sprite, int x, int y)
{
    return 0;
}

void SG_Special::DrawSpecialSpritePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer,
                                       int sprite, int x, int y)
{
}
