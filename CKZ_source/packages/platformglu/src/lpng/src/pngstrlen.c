/* pngstrlen.c - Glu addition to libpng 1.2.41 in CKZ 3.1.1.
 * pngconf.h maps png_strlen to this function instead of the C library strlen.
 */
#define PNG_INTERNAL
#include "png.h"

png_size_t PNGAPI
png_strlen(png_const_charp str)
{
   png_size_t length = 0;

   while (str[length] != '\0')
      length++;
   return length;
}
