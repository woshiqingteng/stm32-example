/*
 * jconfig.h - LibJPEG configuration for the bare-metal ARM target.
 *
 * Memory comes from the C library heap provided by newlib (the link uses
 * nosys.specs, so malloc/free resolve to the toolchain runtime).
 */

#ifndef LIBJPEG_JCONFIG_H
#define LIBJPEG_JCONFIG_H

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#define NO_GETENV
#define MAX_ALLOC_CHUNK 0x10000

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H

#undef CHAR_IS_UNSIGNED
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

#ifdef JPEG_INTERNALS
#undef RIGHT_SHIFT_IS_UNSIGNED
#endif

#endif /* LIBJPEG_JCONFIG_H */
