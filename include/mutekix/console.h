#ifndef __MUTEKIX_CONSOLE_H__
#define __MUTEKIX_CONSOLE_H__

#ifdef __cplusplus
extern "C" {
#endif

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

extern const mutekix_console_palette_t MUTEKIX_CONSOLE_PALETTE_DEFAULT;
extern const mutekix_console_palette_t MUTEKIX_CONSOLE_PALETTE_GRAY;

extern void mutekix_console_init(const mutekix_console_palette_t *palette);
extern void mutekix_console_fini();
extern void mutekix_console_clear();
extern size_t mutekix_console_write(const char *ptr, size_t len);
extern int mutekix_console_puts(const char *s);
extern int mutekix_console_printf(const char *fmt, ...);
extern int mutekix_console_getchar();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MUTEKIX_CONSOLE_H__
