/* config.h for the Emscripten / WASM build.
 * Replaces the autotools-generated config.h at the repo root.
 * The Makefile passes -I$(EMSCRIPTEN_DIR) so this file is found first.
 */
#ifndef _CONFIG_H
#define _CONFIG_H

#define PACKAGE        "freedroid"
#define PACKAGE_NAME   "freedroid"
#define PACKAGE_STRING "freedroid 1.9.0"
#define PACKAGE_VERSION "1.9.0"
#define VERSION        "1.9.0"
#define PACKAGE_TARNAME "freedroid"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_URL    ""

/* Standard C headers — all available in Emscripten's musl libc */
#define HAVE_DIRENT_H    1
#define HAVE_FCNTL_H     1
#define HAVE_INTTYPES_H  1
#define HAVE_MEMSET      1
#define HAVE_RAND        1
#define HAVE_SQRT        1
#define HAVE_STDINT_H    1
#define HAVE_STDIO_H     1
#define HAVE_STDLIB_H    1
#define HAVE_STRCSPN     1
#define HAVE_STRINGS_H   1
#define HAVE_STRING_H    1
#define HAVE_STRSPN      1
#define HAVE_STRSTR      1
#define HAVE_STRTOK      1
#define HAVE_SYS_STAT_H  1
#define HAVE_SYS_TIME_H  1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME_H      1
#define HAVE_UNISTD_H    1
#define STDC_HEADERS     1

/* Math and image libs provided by Emscripten ports */
#define HAVE_LIBM     1
#define HAVE_LIBPNG   1
#define HAVE_LIBJPEG  1

/* No X11 in a browser */
#define X_DISPLAY_MISSING 1

/* Data path — matches --preload-file target in the Makefile */
#define FD_DATADIR    "/freedroid"
#define LOCAL_DATADIR "/freedroid"

#endif /* _CONFIG_H */
