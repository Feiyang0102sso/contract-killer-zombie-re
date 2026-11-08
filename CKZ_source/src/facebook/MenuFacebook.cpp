// MenuFacebook.cpp - the Facebook menu of the game.
// Reconstructed (dead-stripped in the shipped binary): only weak copies of three empty
// SimpleDialog handlers (SelectedItemChanged, SelectedSubitemChanged, ClickSubitem) survive in
// MenuFacebook.o, so the file defined a SimpleDialog subclass whose vtable referenced them and
// which overrode ClickItem. The class name, the override and its (empty) body are guesses.
#include "SimpleDialog.h"

class MenuFacebook : public SimpleDialog {
public:
    /** An item of the menu was chosen (body not known). */
    virtual void ClickItem(int id, bool byPointer);
};

void MenuFacebook::ClickItem(int id, bool byPointer)
{
}
