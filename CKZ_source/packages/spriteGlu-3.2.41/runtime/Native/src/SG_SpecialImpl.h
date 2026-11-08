// SG_SpecialImpl.h - default hook handler of SG_Presenter: forwards to SG_Special. (created by b042)
#ifndef SG_SPECIALIMPL_H
#define SG_SPECIALIMPL_H

#include "SG_Special.h"

class SG_SpecialImpl : public SG_SpecialHandler {
public:
    virtual unsigned char DrawSpecialFramePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame);
    virtual void DrawSpecialFramePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame);
    virtual unsigned char DrawSpecialLayerPre(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer);
    virtual void DrawSpecialLayerPost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer);
    virtual unsigned char DrawSpecialSpritePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer,
                                               int sprite, int x, int y);
    virtual void DrawSpecialSpritePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer,
                                       int sprite, int x, int y);
    virtual SG_SpecialHandler* Clone();
};

#endif
