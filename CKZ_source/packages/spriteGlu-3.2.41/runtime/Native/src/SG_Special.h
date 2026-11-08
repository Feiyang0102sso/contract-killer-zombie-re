// SG_Special.h - hooks called by SG_Presenter::TraverseCurrentFrame around frames, layers and
// sprites; a Pre hook returning non zero skips that element. (created by b042)
#ifndef SG_SPECIAL_H
#define SG_SPECIAL_H

#include "SG_Defines.h"

class SG_Presenter;

/** Default hooks (static: only make sure SG_Home exists). */
class SG_Special {
public:
    static unsigned char DrawSpecialFramePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame);
    static void DrawSpecialFramePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame);
    static unsigned char DrawSpecialLayerPre(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer);
    static void DrawSpecialLayerPost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer);
    static unsigned char DrawSpecialSpritePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer,
                                              int sprite, int x, int y);
    static void DrawSpecialSpritePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer,
                                      int sprite, int x, int y);
};

/** Hook interface of a presenter (see SG_SpecialImpl). */
class SG_SpecialHandler {
public:
    virtual unsigned char DrawSpecialFramePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame) = 0;
    virtual void DrawSpecialFramePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame) = 0;
    virtual unsigned char DrawSpecialLayerPre(SG_TraverseMode mode, SG_Presenter& presenter, int frame,
                                              int layer) = 0;
    virtual void DrawSpecialLayerPost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer) = 0;
    virtual unsigned char DrawSpecialSpritePre(SG_TraverseMode mode, SG_Presenter& presenter, int frame,
                                               int layer, int sprite, int x, int y) = 0;
    virtual void DrawSpecialSpritePost(SG_TraverseMode mode, SG_Presenter& presenter, int frame, int layer,
                                       int sprite, int x, int y) = 0;
    virtual SG_SpecialHandler* Clone() = 0;
};

#endif
