// PROVISIONAL (created by b051x): header-only swerve_tw wrapper, no owning unit
// tws_loader.h - typed front end of the M3G loader (first used in src/ext/SwerveHelper.cpp).
#ifndef TWS_LOADER_H
#define TWS_LOADER_H

#include "tws.h"
#include "CVector.h"
#include "CSwerveLoader.h"

namespace swerve {

/** Loads M3G data into Object3D handles. */
template <class ErrorHandler>
class Loader {
public:
    /** Load `size` bytes of `data` from `offset`; the root objects are appended to `objects`. */
    static void load(unsigned int size, const unsigned char* data, int offset,
                     com::glu::platform::core::CVector<Object3D<ErrorHandler> >& objects);
    /** Append a handle for every object of `loaded`; the handles take over the loader's references. */
    static void wrap(com::glu::platform::core::CVector<IObject3D*>& loaded,
                     com::glu::platform::core::CVector<Object3D<ErrorHandler> >& objects);
};

template <class ErrorHandler>
void Loader<ErrorHandler>::load(unsigned int size, const unsigned char* data, int offset,
                                com::glu::platform::core::CVector<Object3D<ErrorHandler> >& objects)
{
    com::glu::platform::core::CVector<IObject3D*> loaded;
    CSwerveLoader::load(size, data, offset, loaded);
    wrap(loaded, objects);
}

template <class ErrorHandler>
void Loader<ErrorHandler>::wrap(com::glu::platform::core::CVector<IObject3D*>& loaded,
                                com::glu::platform::core::CVector<Object3D<ErrorHandler> >& objects)
{
    int count = loaded.Size();
    for (int index = 0; index < count; index++) {
        // IObject3D (SwerveApi.h) is the C view of the same runtime objects as IQueryInterface
        objects.Add(Object3D<ErrorHandler>((IQueryInterface*)loaded[index]));
    }
}

} // namespace swerve

#endif
