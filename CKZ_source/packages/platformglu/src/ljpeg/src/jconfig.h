/* jconfig.h for libjpeg 6b as built into CKZ 3.1.1 (armv7 iOS, clang).
 * Hand-written from jconfig.doc (IJG ships no canned config for iOS).
 * boolean is unsigned char: the original reads/writes boolean fields as bytes
 * (b091 investigation: output_width at 0x5c, output_scanline at 0x78, struct size 0x1ac).
 */

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN
/* the original jinit_memory_mgr has no getenv("JPEGMEM") check */
#define NO_GETENV

/* byte-sized boolean (see header comment) */
typedef unsigned char boolean;
#define HAVE_BOOLEAN

#ifdef JPEG_INTERNALS
#undef RIGHT_SHIFT_IS_UNSIGNED
#endif
