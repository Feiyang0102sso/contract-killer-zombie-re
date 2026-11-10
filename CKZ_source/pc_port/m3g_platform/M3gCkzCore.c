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

/*
 * IndexBuffer triangle list in Swerve's vertex order.
 *
 * m3gGetBatchIndices lists triangle i of a strip s as (s[i], s[i+1], s[i+2]) when i is even and as
 * (s[i], s[i+2], s[i+1]) when i is odd. Swerve lists the odd one as (s[i+1], s[i], s[i+2]): the
 * same triangle with the same winding, starting at another corner. CCollision::BuildMesh takes the
 * first corner as the base of its edge vectors and plane products, so the collision mesh only
 * matches the original with Swerve's order. Compared on every M3G file of the game against the ARM
 * backend (tests/M3gBackendTest.cpp, "firstIndices"). `indices` holds m3gGetBatchSize entries.
 */
M3G_API void ckzM3gGetSwerveTriangleIndices(M3GIndexBuffer handle, M3Gint *indices)
{
    const IndexBuffer *buffer = (const IndexBuffer *) handle;
    M3Gint strip;
    M3Gint corner;
    M3Gint triangle = 0;
    M3Gint triangleIndices[4] = {0, 0, 0, 0};

    for (strip = 0; strip < buffer->stripCount; strip++) {
        for (corner = 0; corner < buffer->lengths[strip] - 2; corner++) {
            m3gGetIndices(buffer, triangle, triangleIndices);
            if (triangleIndices[3] == 0) {
                *indices++ = triangleIndices[0];
                *indices++ = triangleIndices[1];
                *indices++ = triangleIndices[2];
            }
            else {
                *indices++ = triangleIndices[1];
                *indices++ = triangleIndices[0];
                *indices++ = triangleIndices[2];
            }
            triangle++;
        }
        /* Skip the degenerate triangles that stitch the strips together (as m3gGetBatchIndices). */
        if (buffer->lengths[strip] & 1) {
            triangle += 5;
        }
        else {
            triangle += 4;
        }
    }
}
