#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <muteki/common.h>
#include <muteki/threading.h>
#include <muteki/utils.h>
#include <muteki/ui/canvas.h>

#include <mutekix/console.h>

typedef struct {
    unsigned short bottom_right_x;
    unsigned short bottom_right_y;
    unsigned short pos_x;
    unsigned short pos_y;
    uint8_t font_type;
    unsigned short font_height;
    const mutekix_console_palette_t *palette;
    bool ready;
    bool scroll_requested;
    critical_section_t mutex;
} fbcon_config_t;

const mutekix_console_palette_t MUTEKIX_CONSOLE_PALETTE_DEFAULT = {
    .black = RGB_FROM_U8(0, 0, 0),
    .red = RGB_FROM_U8(170, 0, 0),
    .green = RGB_FROM_U8(0, 170, 0),
    .yellow = RGB_FROM_U8(170, 85, 0),
    .blue = RGB_FROM_U8(0, 0, 170),
    .magenta = RGB_FROM_U8(170, 0, 170),
    .cyan = RGB_FROM_U8(0, 170, 170),
    .white = RGB_FROM_U8(170, 170, 170),
    .bright_black = RGB_FROM_U8(85, 85, 85),
    .bright_red = RGB_FROM_U8(255, 85, 85),
    .bright_green = RGB_FROM_U8(85, 255, 85),
    .bright_yellow = RGB_FROM_U8(255, 255, 85),
    .bright_blue = RGB_FROM_U8(85, 85, 255),
    .bright_magenta = RGB_FROM_U8(255, 85, 255),
    .bright_cyan = RGB_FROM_U8(85, 255, 255),
    .bright_white = RGB_FROM_U8(255, 255, 255),
};

const mutekix_console_palette_t MUTEKIX_CONSOLE_PALETTE_GRAY = {
    .black = RGB_FROM_U8(255, 255, 255),
    .red = RGB_FROM_U8(127, 127, 127),
    .green = RGB_FROM_U8(191, 191, 191),
    .yellow = RGB_FROM_U8(223, 223, 223),
    .blue = RGB_FROM_U8(63, 63, 63),
    .magenta = RGB_FROM_U8(143, 143, 143),
    .cyan = RGB_FROM_U8(239, 239, 239),
    .white = RGB_FROM_U8(0, 0, 0),
    .bright_black = RGB_FROM_U8(255, 255, 255),
    .bright_red = RGB_FROM_U8(127, 127, 127),
    .bright_green = RGB_FROM_U8(191, 191, 191),
    .bright_yellow = RGB_FROM_U8(223, 223, 223),
    .bright_blue = RGB_FROM_U8(63, 63, 63),
    .bright_magenta = RGB_FROM_U8(143, 143, 143),
    .bright_cyan = RGB_FROM_U8(239, 239, 239),
    .bright_white = RGB_FROM_U8(0, 0, 0),
};

static fbcon_config_t fbcon_config = {
    .bottom_right_x = 0,
    .bottom_right_y = 0,
    .pos_x = 0,
    .pos_y = 0,
    .font_type = MONOSPACE_TINY_NOCJK,
    .font_height = 0,
    .palette = &MUTEKIX_CONSOLE_PALETTE_DEFAULT,
    .ready = false,
    .scroll_requested = false,
    .mutex = {0},
};

size_t mutekix_console_write(const char *ptr, size_t len) {
    size_t pos = 0;
    short x = fbcon_config.pos_x, y = fbcon_config.pos_y;
    short char_height = fbcon_config.font_height;
    bool scroll_requested = fbcon_config.scroll_requested;

    OSEnterCriticalSection(&(fbcon_config.mutex));

    if (!fbcon_config.ready || len == 0) {
        return 0;
    }

    for (; pos<len; pos++) {
        short char_width;
        if (fbcon_config.font_type == 17) {
            char_width = 6;
        } else {
            char_width = GetCharWidth(ptr[pos], fbcon_config.font_type);
        }
        bool empty = false;
        bool lf = false;

        // Resolve escape sequences and special characters.
        switch (ptr[pos]) {
            case '\n':
                empty = true;
                lf = true;
                break;
            default:
                empty = false;
                lf = false;
        }

        if (scroll_requested) {
            ScrollUp(0, 0, fbcon_config.bottom_right_x, fbcon_config.bottom_right_y, char_height);
            scroll_requested = false;
        }

        // Calculate next placement
        // Last column or explicit LF request.
        if (x + char_width > fbcon_config.bottom_right_x || lf) {
            // Last line on screen. Scroll the whole frame up by char_height and do an implicit CR.
            if (y + char_height > fbcon_config.bottom_right_y) {
                if (lf) {
                    // Scroll immediately if there's an explicit LF
                    ScrollUp(0, 0, fbcon_config.bottom_right_x, fbcon_config.bottom_right_y, char_height);
                } else {
                    // delayed scroll
                    scroll_requested = true;
                }
                x = 0;
            // Last column but not last line on screen. Do an implicit CRLF.
            } else {
                y += fbcon_config.font_height;
                x = 0;
            }
        }
        if (!empty) {
            WriteChar(x, y, ptr[pos], false);
            x += char_width;
        }
    }

    fbcon_config.pos_x = x;
    fbcon_config.pos_y = y;
    fbcon_config.scroll_requested = scroll_requested;

    OSLeaveCriticalSection(&(fbcon_config.mutex));
    return pos + 1;
}

int mutekix_console_puts(const char *s) {
    if (!fbcon_config.ready) {
        return EOF;
    }

    size_t len = strlen(s);
    size_t actual = mutekix_console_write(s, len);
    mutekix_console_write("\n", 1);
    return (int) (actual + 1);
}

int mutekix_console_printf(const char *fmt, ...) {
    char tmp[1024];
    va_list va;

    va_start(va, fmt);

    if (!fbcon_config.ready) {
        return -1;
    }

    int ret = vsnprintf(tmp, sizeof(tmp), fmt, va);
    if (ret < 0) {
        return ret;
    }
    mutekix_console_write(tmp, (size_t) ret);

    va_end(va);
    return ret;
}

void mutekix_console_clear() {
    OSEnterCriticalSection(&(fbcon_config.mutex));
    ClearScreen(false);
    fbcon_config.pos_x = 0;
    fbcon_config.pos_y = 0;
    fbcon_config.scroll_requested = false;
    OSLeaveCriticalSection(&(fbcon_config.mutex));
}

/**
 * @brief Initialize the console.
 *
 * Use @p palette to specify a palette, or set it to NULL to use the default VGA palette on color screen devices and simple black text on white background palette on monochrome/grayscale devices.
 *
 * WARNING: DO NOT put custom palette in stack memory since the console implementation will continue to access it even after the function calling mutekix_console_init() returns. Use heap or static memory instead.
 *
 * @param palette Specify a custom palette, or @p NULL to use the default.
 */
void mutekix_console_init(const mutekix_console_palette_t *palette) {
    OSInitCriticalSection(&(fbcon_config.mutex));

    OSEnterCriticalSection(&(fbcon_config.mutex));
    fbcon_config.bottom_right_x = GetMaxScrX();
    fbcon_config.bottom_right_y = GetMaxScrY();

    fbcon_config.font_height = GetFontHeight(fbcon_config.font_type);
    SetFontType(fbcon_config.font_type);
    if (palette == NULL) {
        vram_descriptor_t *fbdesc = GetActiveVRamAddress();
        // If the color depth is <= 4 bit, assuming it's grayscale.
        if (fbdesc->depth <= 4) {
            fbcon_config.palette = &MUTEKIX_CONSOLE_PALETTE_GRAY;
        } else {
            fbcon_config.palette = &MUTEKIX_CONSOLE_PALETTE_DEFAULT;
        }
    } else {
        fbcon_config.palette = palette;
    }

    rgbSetBkColor(fbcon_config.palette->black);
    rgbSetColor(fbcon_config.palette->white);

    mutekix_console_clear();

    fbcon_config.ready = true;

    OSLeaveCriticalSection(&(fbcon_config.mutex));
}

/**
 * @brief Deinitialize the console.
 *
 * Sets the internal enable flag back to false so no more messages will be printed.
 */
void mutekix_console_fini() {
    OSEnterCriticalSection(&(fbcon_config.mutex));
    fbcon_config.ready = false;
    OSLeaveCriticalSection(&(fbcon_config.mutex));
    OSDeleteCriticalSection(&(fbcon_config.mutex));
}
