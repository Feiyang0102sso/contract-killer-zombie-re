// CBountyGameFactory.h - gServe factory of the game. (created by b057x, completed by b047x)
// It only overrides createRemoteUser (the game does not create remote users); the destructor is
// the implicit one (weak copies in CBountyGameFactory.o).
#ifndef CBOUNTYGAMEFACTORY_H
#define CBOUNTYGAMEFACTORY_H

#include "CNGSFactory.h"

class CBountyGameFactory : public CNGSFactory {
public:
    /** Remote users are not created by the game: always NULL. */
    virtual CNGSRemoteUser* createRemoteUser(const CNGSUserCredentials& credentials);
};

#endif
