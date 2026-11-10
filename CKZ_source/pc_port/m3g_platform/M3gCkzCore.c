/*
 * CKZ: the m3gcore library translation unit. m3gcore is a unity build (m3g_core.c includes every
 * module), so this file compiles it unchanged and then adds extensions for Swerve semantics that
 * m3gcore's public API cannot express; they need the internal structures that only exist inside
 * that unit. The m3gcore sources themselves stay unmodified (PORT_PLAN.md P2, M3).
 */
#include "../3rd_party/m3gcore/src/m3g_core.c"
#include "M3gCkzExtensions.h"

/*
 * Group.removeChild without m3gcore's bone check.
 *
 * m3gcore refuses to remove a node flagged hasBones (M3G_INVALID_VALUE), and m3gNodeDuplicate
 * copies that flag to clones. CUnitBody::Behead duplicates the head group of a zombie skeleton,
 * flies it with SwerveBallistics and removes it after 3 seconds; with the check the copy stayed in
 * the world forever. Probed on the ARM backend: Swerve removes such a copy (getParent is null
 * afterwards). The flag is restored after the removal, it only drives transform invalidation.
 */
M3G_API void ckzM3gRemoveChild(M3GGroup group, M3GNode child)
{
    Node *node = (Node *) child;
    M3Guint hadBones;

    if (node == NULL) {
        m3gRemoveChild(group, child);
        return;
    }
    hadBones = node->hasBones;
    node->hasBones = M3G_FALSE;
    m3gRemoveChild(group, child);
    node->hasBones = hadBones;
}
