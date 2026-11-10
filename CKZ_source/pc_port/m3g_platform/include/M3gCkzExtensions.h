/* CKZ: m3gcore extensions for Swerve semantics, implemented in M3gCkzCore.c. */
#ifndef CKZ_M3G_EXTENSIONS_H
#define CKZ_M3G_EXTENSIONS_H

#include <m3g_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Group.removeChild as Swerve does it: also removes nodes m3gcore flags as skeleton bones. */
M3G_API void ckzM3gRemoveChild(M3GGroup group, M3GNode child);

#ifdef __cplusplus
}
#endif

#endif
