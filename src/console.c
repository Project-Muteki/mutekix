#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <muteki/common.h>
#include <muteki/threading.h>
#include <muteki/utils.h>
#include <muteki/ui/canvas.h>
#include <muteki/ui/event.h>

#include <mutekix/console.h>

static unsigned int FLAG_SHIFT = 1;
static unsigned int FLAG_CAPS = (1 << 1);
static unsigned int FLAG_SYMBOL = (1 << 2);

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

int mutekix_console_getchar() {
    static ui_event_t uievent = {0};
    static unsigned int flags;
    register bool is_shift = false, is_symbol=false;

    while (true) {
        if ((TestPendEvent(&uievent) || TestKeyEvent(&uievent)) && GetEvent(&uievent)) {
            if (uievent.key_code0 != KEY_SHIFT && uievent.key_code0 != KEY_SYMBOL) {
                // These 2 are stackable so symbol shift <key> will work
                // Note that shift symbol will not work because that will be search on some systems
                if (flags & FLAG_SHIFT) {
                    is_shift = true;
                    flags &= ~(FLAG_SHIFT);
                }
                if (flags & FLAG_SYMBOL) {
                    is_symbol = true;
                    flags &= ~(FLAG_SYMBOL);
                }
            }

            switch (uievent.key_code0) {
            // Exceptions
            case KEY_ESC:
                return 4; // ^D
            case KEY_ENTER:
                return '\r';
            case KEY_SHIFT:
                flags ^= FLAG_SHIFT;
                break;
            case KEY_SYMBOL:
                flags ^= FLAG_SYMBOL;
                break;
            case KEY_CAPS:
                flags ^= FLAG_CAPS;
                break;
            case KEY_TAB:
                return '\t';
            case KEY_DEL:
                return 8; // ^H aka backspace
            // Symbol keys
            case KEY_1:
                // Symbol + 1 = `
                return is_symbol ? '`' : '1';
            case KEY_EXCL:
                // Symbol + ! (Symbol + Shift + 1) = ~
                return is_symbol ? '~' : '!';
            case KEY_6:
                // Symbol + 6 = ^
                return is_symbol ? '^' : '6';
            case KEY_QUESTION:
                // Symbol + ? (Symbol + Shift + 7) = /
                return is_symbol ? '/' : '?';
            case KEY_7:
                // Symbol + 7 = &
                return is_symbol ? '&' : '7';
            case KEY_COMMA:
                // Symbol + , (Symbol + Shift + 7) = <
                return is_symbol ? '<' : ',';
            case KEY_0:
                // Symbol + 0 = =
                return is_symbol ? '=' : '0';
            case KEY_RPAREN:
                // Symbol + ) = +
                return is_symbol ? '+' : ')';
            case KEY_MENU:
                // Symbol + Menu = _
                if (is_symbol) {
                    return '_';
                }
                break;
            case KEY_DASH:
                // Symbol + - (Symbol + Shift + Menu) = _
                return is_symbol ? '_' : '-';
            case KEY_FONT:
                // Symbol + Font = >
                if (is_symbol) {
                    return '>';
                }
                break;
            case KEY_DOT:
                // Symbol + . (Symbol + Shift + Font) = >
                return is_symbol ? '>' : '.';
            case KEY_I:
                // Symbol + Shift + I = {
                if (is_symbol && is_shift) {
                    return '{';
                }
                // Symbol + I = [
                if (is_symbol) {
                    return '[';
                }
                if (is_shift) {
                    return 'I';
                }
                return 'i';
            case KEY_O:
                // Symbol + Shift + O = }
                if (is_symbol && is_shift) {
                    return '}';
                }
                // Symbol + O = ]
                if (is_symbol) {
                    return ']';
                }
                if (is_shift) {
                    return 'O';
                }
                return 'o';
            case KEY_P:
                // Symbol + Shift + P = |
                if (is_symbol && is_shift) {
                    return '|';
                }
                // Symbol + P = \\ (a single backslash)
                if (is_symbol) {
                    return '\\';
                }
                if (is_shift) {
                    return 'P';
                }
                return 'p';
            case KEY_K:
                // Symbol + Shift + K = :
                if (is_symbol && is_shift) {
                    return ':';
                }
                // Symbol + K = ;
                if (is_symbol) {
                    return ';';
                }
                if (is_shift) {
                    return 'K';
                }
                return 'k';
            case KEY_L:
                // Symbol + Shift + L = "
                if (is_symbol && is_shift) {
                    return '"';
                }
                // Symbol + L = '
                if (is_symbol) {
                    return '\'';
                }
                if (is_shift) {
                    return 'L';
                }
                return 'l';
            default:
                if (uievent.key_code0 >= KEY_A && uievent.key_code0 <= KEY_Z) {
                    return uievent.key_code0 + ((((flags & FLAG_CAPS) != 0) || is_shift) ? 0 : 0x20);
                }
                if (uievent.key_code0 >= 0x20 && uievent.key_code0 < 0x80) {
                    return uievent.key_code0;
                }
            }
        }
    }
    //return EOF;
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
