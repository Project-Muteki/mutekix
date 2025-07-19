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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque assets reader context.
 */
typedef struct mutekix_assets_s mutekix_assets_t;

/**
 * @brief Create the loader file descriptor instance of the asset bundle of the current applet.
 * @details The descriptor needs to be closed manually with _CloseFile() after use.
 * @x_void_param
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

/**
 * @brief Close asset bundle.
 * @param ctx Context object.
 * @x_void_return
 */
void mutekix_assets_fini(mutekix_assets_t *ctx);

/**
 * @brief Open an asset file in the bundle by path.
 * @param ctx Context object.
 * @param path Path to the asset file.
 * @return Loader file discriptor of the asset file.
 */
loader_file_descriptor_t *mutekix_assets_open(mutekix_assets_t *ctx, const char *path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MUTEKIX_CONSOLE_H__
