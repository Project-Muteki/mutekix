/**
 * @file assets.h
 * @brief Load files from applet assets bundle.
 * @details
 * There's no standard formatting for Besta RTOS applet asset bundles. Usually the applet would handle asset bundle
 * parsing and assets loading all within their own code base. This component provides a possible implementation of
 * such asset bundle handler, that (currently with bare-minimum code) implements loading an uncompressed ZIP file
 * as an asset bundle and opening arbitrary files within that bundle. Note that it's not (yet) a full filesystem
 * implementation, nor a full ZIP file parser implementation, so features like listing files for example are not
 * implemented. You may need to consider using other libraries like minizip for more involved tasks.
 *
 * The recommended way of making the asset bundle is to use the `zip` command:
 * @code{.sh}
 * zip -0 applet.dat <file> [files...]
 * # Optionally add it to the exe file
 * cat applet.exe applet.dat > applet-with-asset.exe
 * @endcode
 */

#ifndef __MUTEKIX_ASSETS_H__
#define __MUTEKIX_ASSETS_H__

#include <stdbool.h>
#include <sys/types.h>

#include <muteki/loader.h>
#include <muteki/utils.h>

#include "mutekix/mlib/m-dict.h"
#include "mutekix/mlib/m-string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mutekix_assets_index_entry_s {
    size_t offset;
    size_t size;
} mutekix_assets_index_dict_entry_t;

DICT_DEF2(mutekix_assets_index_dict, string_t, M_STRING_OPLIST, mutekix_assets_index_dict_entry_t, M_POD_OPLIST)

typedef struct {
    loader_file_descriptor_t *root;
    mutekix_assets_index_dict_t index;
} mutekix_assets_t;

/**
 * @brief Create the loader file descriptor instance of the asset bundle of the current applet.
 * @details The descriptor needs to be closed manually with _CloseFile() after use.
 * @return The loader file descriptor of the asset bundle.
 */
loader_file_descriptor_t *mutekix_assets_get_root(void);

/**
 * @brief Load asset bundle.
 * 
 * @param ctx Context object.
 * @retval true @x_term ok
 * @retval false @x_term ng
 */
bool mutekix_assets_init_applet(mutekix_assets_t *ctx);

void mutekix_assets_fini(mutekix_assets_t *ctx);

loader_file_descriptor_t *mutekix_assets_open(mutekix_assets_t *ctx, const char *path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MUTEKIX_CONSOLE_H__
