// Platform functions m3gcore expects from its host (m3g_defs.h, m3g_gl.h, m3g_loader_inflate.inl).
// Replaces m3gcore's Symbian/Android platform files: zlib inflation, logging, assertions and
// native bitmap/window queries. The game never renders into native bitmaps or windows through
// m3gcore, so those queries report failure.
#include "m3g_defs.h"
#include "m3g_gl.h"
#include <zlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Inflate one compressed M3G section. The game's zlib only contains the inflate half,
 * so this does by hand what zlib's uncompress() would. Returns the inflated size, 0 on error.
 */
extern "C" M3Gsizei m3gSymbianInflateBlock(M3Gsizei srcLength, const M3Gubyte* src,
                                           M3Gsizei dstLength, M3Gubyte* dst)
{
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = const_cast<Bytef*>(src);
    stream.avail_in = static_cast<uInt>(srcLength);
    stream.next_out = dst;
    stream.avail_out = static_cast<uInt>(dstLength);
    if (inflateInit(&stream) != Z_OK) {
        fprintf(stderr, "M3G inflate: init failed\n");
        return 0;
    }
    const int status = inflate(&stream, Z_FINISH);
    const M3Gsizei produced = static_cast<M3Gsizei>(stream.total_out);
    inflateEnd(&stream);
    if (status != Z_STREAM_END) {
        fprintf(stderr, "M3G inflate: failed with %d after %d bytes\n", status, produced);
        return 0;
    }
    return produced;
}

extern "C" void m3gLogMessage(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    fputs("M3G: ", stderr);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
}

extern "C" void m3gBeginLog(void)
{
    m3gLogMessage("--- M3G event log ---\n");
}

extern "C" void m3gEndLog(void)
{
    m3gLogMessage("--- end of log ---\n");
}

extern "C" void m3gAssertFailed(const char* filename, int line)
{
    fprintf(stderr, "M3G assertion failed: %s, line %d\n", filename, line);
    abort();
}

#if defined(M3G_ENABLE_PROFILING)
extern "C" void m3gCleanupProfile(void)
{
}

extern "C" void m3gBeginProfile(int)
{
}

extern "C" int m3gEndProfile(int)
{
    return 0;
}

extern "C" M3Gint m3gProfileTriggered(void)
{
    return 0;
}
#endif

M3Gbool m3gglLockNativeBitmap(M3GNativeBitmap, M3Gubyte**, M3Gsizei*)
{
    return M3G_FALSE;
}

void m3gglReleaseNativeBitmap(M3GNativeBitmap)
{
}

extern "C" M3Gbool m3gglGetNativeBitmapParams(M3GNativeBitmap, M3GPixelFormat*, M3Gint*, M3Gint*, M3Gint*)
{
    fprintf(stderr, "M3G: native bitmap targets are not supported\n");
    return M3G_FALSE;
}

extern "C" M3Gbool m3gglGetNativeWindowParams(M3GNativeWindow, M3GPixelFormat*, M3Gint*, M3Gint*)
{
    fprintf(stderr, "M3G: native window targets are not supported\n");
    return M3G_FALSE;
}
