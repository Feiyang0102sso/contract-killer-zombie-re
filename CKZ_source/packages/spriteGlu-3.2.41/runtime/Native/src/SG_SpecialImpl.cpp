// SG_SpecialImpl.cpp - default hook handler, forwards every hook to SG_Special. (written by b042)
#include "SG_SpecialImpl.h"

unsigned char SG_SpecialImpl::DrawSpecialFramePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame)
{
    return SG_Special::DrawSpecialFramePre(mode, presenter, frame);
}

void SG_SpecialImpl::DrawSpecialFramePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame)
{
    SG_Special::DrawSpecialFramePost(mode, presenter, frame);
}

unsigned char SG_SpecialImpl::DrawSpecialLayerPre(SG_TraverseMode mode, SG_Presenter& presenter, int frame,
                                                  int layer)
{
    return SG_Special::DrawSpecialLayerPre(mode, presenter, frame, layer);
}

void SG_SpecialImpl::DrawSpecialLayerPost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer)
{
    SG_Special::DrawSpecialLayerPost(mode, presenter, frame, layer);
}

unsigned char SG_SpecialImpl::DrawSpecialSpritePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame,
                                                   int layer, int sprite, int x, int y)
{
    return SG_Special::DrawSpecialSpritePre(mode, presenter, frame, layer, sprite, x, y);
}

void SG_SpecialImpl::DrawSpecialSpritePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer,
                                           int sprite, int x, int y)
{
    SG_Special::DrawSpecialSpritePost(mode, presenter, frame, layer, sprite, x, y);
}

SG_SpecialHandler* SG_SpecialImpl::Clone()
{
    return new SG_SpecialImpl();
}
