/**
 * @file console.h
 * @brief Simple ANSI console implementation.
 */

#ifndef __MUTEKIX_CONSOLE_H__
#define __MUTEKIX_CONSOLE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Palette data
 */
typedef struct {
    union {
        // by name
        struct {
            int black;
            int red;
            int green;
            int yellow;
            int blue;
            int magenta;
            int cyan;
            int white;
            int bright_black;
            int bright_red;
            int bright_green;
            int bright_yellow;
            int bright_blue;
            int bright_magenta;
            int bright_cyan;
            int bright_white;
        };
        // by index
        int colors[16];
    };
} mutekix_console_palette_t;

/**
 * @brief Default palette for XRGB LCD framebuffer.
 */
extern const mutekix_console_palette_t MUTEKIX_CONSOLE_PALETTE_DEFAULT;

/**
 * @brief Default palette for 4-bit grayscale LCD framebuffer.
 */
extern const mutekix_console_palette_t MUTEKIX_CONSOLE_PALETTE_GRAY;

/**
 * @brief Initialize the console.
 * @param palette Palette to use. If this is `NULL`, default palette will be used.
 * @x_void_return
 */
extern void mutekix_console_init(const mutekix_console_palette_t *palette);

/**
 * @brief Deinitialize the console.
 * @x_void_param
 * @x_void_return
 */
extern void mutekix_console_fini();

/**
 * @brief Clear all text and reset cursor back to the upper left corner of the screen.
 * @x_void_param
 * @x_void_return
 */
extern void mutekix_console_clear();

/**
 * @brief Write buffer to the console.
 * @param ptr Pointer to buffer that holds the string to write.
 * @param len Number of bytes to be written.
 * @return Number of bytes written.
 */
extern size_t mutekix_console_write(const char *ptr, size_t len);

/**
 * @brief Write NUL-terminated string to the console, and terminate the string with a new line.
 * @param ptr Pointer to buffer that holds the string to write.
 * @return Number of bytes written.
 */
extern int mutekix_console_puts(const char *s);

/**
 * @brief Write formatted string to the console.
 * @param fmt The format string.
 * @param ... The parameters.
 * @return Number of bytes written.
 */
extern int mutekix_console_printf(const char *fmt, ...);

/**
 * @brief Read a character from keyboard.
 * @x_void_param
 * @return Character code, or `-1` if there's an error.
 */
extern int mutekix_console_getchar();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MUTEKIX_CONSOLE_H__
