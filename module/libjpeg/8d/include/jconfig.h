/*
 * jconfig.h - LibJPEG configuration for the bare-metal ARM target.
 *
 * Memory is taken from the C library heap provided by newlib (the link uses
 * nosys.specs, so malloc/free resolve to the toolchain runtime).
 */

#ifndef LIBJPEG_JCONFIG_H
#define LIBJPEG_JCONFIG_H

#include <stdlib.h>
#include <stddef.h>

/* Memory allocation routines used by the system dependent memory manager. */
#define JMALLOC malloc
#define JFREE   free

#define NO_GETENV
#undef  USE_MSDOS_MEMMGR
#undef  USE_MAC_MEMMGR
#define USE_HEAP_MEM
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
