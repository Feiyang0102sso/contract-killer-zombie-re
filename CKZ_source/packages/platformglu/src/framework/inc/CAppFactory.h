// Shared declaration of the game-specific resource manager factory (CGameApp.cpp).
#ifndef CAPPFACTORY_H
#define CAPPFACTORY_H
namespace com { namespace glu { namespace platform {
namespace systems { class ICResourceManager; }
namespace framework {
/** Game-specific factories (implemented by the game, src/CGameApp.cpp). */
class CAppFactory {
public:
    static systems::ICResourceManager* CreateResourceManager();
};
}
} } }
#endif
