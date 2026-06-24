/*
rewritten in cpp (original from Isaac, gomehome@qq.com)
*/

#ifndef DRIVER_DISPLAY_H
#define DRIVER_DISPLAY_H

#include <hardware/spi.h>
#include <stdint.h>

#include "gui/fonts/font.h"
#include "qrcode.h"
const uint32_t BYTES_PER_PIXEL = 3;

// https://github.com/Bodmer/TFT_eSPI/blob/5793878d24161c1ed23ccb136f8564f332506d53/TFT_eSPI.h#L984
template <typename T> static inline void transpose(T &a, T &b) {
    T t = a;
    a = b;
    b = t;
}

class Display {
public:
    enum class COLOR : uint32_t {
        ALICEBLUE = 0xF0F8FF,
        ANTIQUEWHITE = 0xFAEBD7,
        AQUA = 0x00FFFF,
        AQUAMARINE = 0x7FFFD4,
        AZURE = 0xF0FFFF,
        BEIGE = 0xF5F5DC,
        BISQUE = 0xFFE4C4,
        BLACK = 0x000000,
        BLANCHEDALMOND = 0xFFEBCD,
        BLUE = 0x0000FF,
        BLUEVIOLET = 0x8A2BE2,
        BROWN = 0xA52A2A,
        BURLYWOOD = 0xDEB887,
        CADETBLUE = 0x5F9EA0,
        CHARTREUSE = 0x7FFF00,
        CHOCOLATE = 0xD2691E,
        CORAL = 0xFF7F50,
        CORNFLOWERBLUE = 0x6495ED,
        CORNSILK = 0xFFF8DC,
        CRIMSON = 0xDC143C,
        CYAN = 0x00FFFF,
        DARKBLUE = 0x00008B,
        DARKCYAN = 0x008B8B,
        DARKGOLDENROD = 0xB8860B,
        DARKGRAY = 0xA9A9A9,
        DARKGREY = 0xA9A9A9,
        DARKGREEN = 0x006400,
        DARKKHAKI = 0xBDB76B,
        DARKMAGENTA = 0x8B008B,
        DARKOLIVEGREEN = 0x556B2F,
        DARKORANGE = 0xFF8C00,
        DARKORCHID = 0x9932CC,
        DARKRED = 0x8B0000,
        DARKSALMON = 0xE9967A,
        DARKSEAGREEN = 0x8FBC8F,
        DARKSLATEBLUE = 0x483D8B,
        DARKSLATEGRAY = 0x2F4F4F,
        DARKSLATEGREY = 0x2F4F4F,
        DARKTURQUOISE = 0x00CED1,
        DARKVIOLET = 0x9400D3,
        DEEPPINK = 0xFF1493,
        DEEPSKYBLUE = 0x00BFFF,
        DIMGRAY = 0x696969,
        DIMGREY = 0x696969,
        DODGERBLUE = 0x1E90FF,
        FIREBRICK = 0xB22222,
        FLORALWHITE = 0xFFFAF0,
        FORESTGREEN = 0x228B22,
        FUCHSIA = 0xFF00FF,
        GAINSBORO = 0xDCDCDC,
        GHOSTWHITE = 0xF8F8FF,
        GOLD = 0xFFD700,
        GOLDENROD = 0xDAA520,
        GRAY = 0x808080,
        GREY = 0x808080,
        GREEN = 0x008000,
        GREENYELLOW = 0xADFF2F,
        HONEYDEW = 0xF0FFF0,
        HOTPINK = 0xFF69B4,
        INDIANRED = 0xCD5C5C,
        INDIGO = 0x4B0082,
        IVORY = 0xFFFFF0,
        KHAKI = 0xF0E68C,
        LAVENDER = 0xE6E6FA,
        LAVENDERBLUSH = 0xFFF0F5,
        LAWNGREEN = 0x7CFC00,
        LEMONCHIFFON = 0xFFFACD,
        LIGHTBLUE = 0xADD8E6,
        LIGHTCORAL = 0xF08080,
        LIGHTCYAN = 0xE0FFFF,
        LIGHTGOLDENRODYELLOW = 0xFAFAD2,
        LIGHTGRAY = 0xD3D3D3,
        LIGHTGREY = 0xD3D3D3,
        LIGHTGREEN = 0x90EE90,
        LIGHTPINK = 0xFFB6C1,
        LIGHTSALMON = 0xFFA07A,
        LIGHTSEAGREEN = 0x20B2AA,
        LIGHTSKYBLUE = 0x87CEFA,
        LIGHTSLATEGRAY = 0x778899,
        LIGHTSLATEGREY = 0x778899,
        LIGHTSTEELBLUE = 0xB0C4DE,
        LIGHTYELLOW = 0xFFFFE0,
        LIME = 0x00FF00,
        LIMEGREEN = 0x32CD32,
        LINEN = 0xFAF0E6,
        MAGENTA = 0xFF00FF,
        MAROON = 0x800000,
        MEDIUMAQUAMARINE = 0x66CDAA,
        MEDIUMBLUE = 0x0000CD,
        MEDIUMORCHID = 0xBA55D3,
        MEDIUMPURPLE = 0x9370DB,
        MEDIUMSEAGREEN = 0x3CB371,
        MEDIUMSLATEBLUE = 0x7B68EE,
        MEDIUMSPRINGGREEN = 0x00FA9A,
        MEDIUMTURQUOISE = 0x48D1CC,
        MEDIUMVIOLETRED = 0xC71585,
        MIDNIGHTBLUE = 0x191970,
        MINTCREAM = 0xF5FFFA,
        MISTYROSE = 0xFFE4E1,
        MOCCASIN = 0xFFE4B5,
        NAVAJOWHITE = 0xFFDEAD,
        NAVY = 0x000080,
        OLDLACE = 0xFDF5E6,
        OLIVE = 0x808000,
        OLIVEDRAB = 0x6B8E23,
        ORANGE = 0xFFA500,
        ORANGERED = 0xFF4500,
        ORCHID = 0xDA70D6,
        PALEGOLDENROD = 0xEEE8AA,
        PALEGREEN = 0x98FB98,
        PALETURQUOISE = 0xAFEEEE,
        PALEVIOLETRED = 0xDB7093,
        PAPAYAWHIP = 0xFFEFD5,
        PEACHPUFF = 0xFFDAB9,
        PERU = 0xCD853F,
        PINK = 0xFFC0CB,
        PLUM = 0xDDA0DD,
        POWDERBLUE = 0xB0E0E6,
        PURPLE = 0x800080,
        RED = 0xFF0000,
        ROSYBROWN = 0xBC8F8F,
        ROYALBLUE = 0x4169E1,
        SADDLEBROWN = 0x8B4513,
        SALMON = 0xFA8072,
        SANDYBROWN = 0xF4A460,
        SEAGREEN = 0x2E8B57,
        SEASHELL = 0xFFF5EE,
        SIENNA = 0xA0522D,
        SILVER = 0xC0C0C0,
        SKYBLUE = 0x87CEEB,
        SLATEBLUE = 0x6A5ACD,
        SLATEGRAY = 0x708090,
        SLATEGREY = 0x708090,
        SNOW = 0xFFFAFA,
        SPRINGGREEN = 0x00FF7F,
        STEELBLUE = 0x4682B4,
        TAN = 0xD2B48C,
        TEAL = 0x008080,
        THISTLE = 0xD8BFD8,
        TOMATO = 0xFF6347,
        TURQUOISE = 0x40E0D0,
        VIOLET = 0xEE82EE,
        WHEAT = 0xF5DEB3,
        WHITE = 0xFFFFFF,
        WHITESMOKE = 0xF5F5F5,
        YELLOW = 0xFFFF00,
        YELLOWGREEN = 0x9ACD32
    };

    enum class ROTATIONS : uint8_t {
        R0 = 0x88,
        R90 = 0xE8,
        R180 = 0x48,
        R270 = 0x28,
    };
    Display(spi_inst_t *spi, uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led_pwm);
    bool init();

    void setBrightness(uint8_t percent);
    void display_toggle(bool state);
    void cleanup();
    void reset();
    void sleep(bool enable);

    void clear(COLOR color);

    void scroll(uint16_t y);
    void set_scroll(uint16_t top, uint16_t bottom);

    void draw_circle(uint16_t x0, uint16_t y0, uint16_t r, COLOR color);
    void draw_ellipse(uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color);
    void draw_hline(uint16_t x, uint16_t y, uint16_t w, COLOR color);
    void draw_vline(uint16_t x, uint16_t y, uint16_t h, COLOR color);
    void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, COLOR color);
    void draw_lines(uint16_t coords, COLOR color);                                         // dunno
    void draw_pixel(uint16_t x, uint16_t y, COLOR color);
    void draw_polygon(uint16_t sides, uint16_t x0, uint16_t y0, uint16_t r, COLOR color);  // dunno
    void draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color);
    void draw_sprite_from_file(const char *path, uint16_t x, uint16_t y);
    void draw_qrcode(QRCode *qrcode, uint16_t x, uint16_t y, uint8_t scale);
    bool draw_letter(uint16_t x, uint16_t y, const char letter, Font *font, COLOR color, COLOR background);
    void draw_text(uint16_t x, uint16_t y, uint16_t spacing, const char *text, Font *font, COLOR color, COLOR background);

    void fill_circle(uint16_t x0, uint16_t y0, uint16_t r, COLOR color);
    void fill_ellipse(uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color);
    void fill_hrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color);
    void fill_vrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color);
    void fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color);
    void fill_round_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, COLOR color);
    void fill_polygon(uint16_t sides, uint16_t x0, uint16_t y0, uint16_t r, COLOR color);  // dunno

    void draw_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, Display::COLOR color);
    void fill_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, Display::COLOR color);
    uint16_t measure_text(const char *text, Font *font, uint16_t spacing);

private:
    // Command constants from ILI9488 datasheet
    enum class CMDS : uint8_t {
        NOP = 0x00,                       // No - op
        SWRESET = 0x01,                   // Software reset
        RDDID = 0x04,                     // Read display ID info
        RDDST = 0x09,                     // Read display status
        SLPIN = 0x10,                     // Enter sleep mode
        SLPOUT = 0x11,                    // Exit sleep mode
        PTLON = 0x12,                     // Partial mode on
        NORON = 0x13,                     // Normal display mode on
        RDMODE = 0x0A,                    // Read display power mode
        RDMADCTL = 0x0B,                  // Read display MADCTL
        RDPIXFMT = 0x0C,                  // Read display pixel format
        RDIMGFMT = 0x0D,                  // Read display image format
        RDSELFDIAG = 0x0F,                // Read display self - diagnostic
        INVOFF = 0x20,                    // Display inversion off
        INVON = 0x21,                     // Display inversion on
        GAMMASET = 0x26,                  // Gamma set
        DISPLAY_OFF = 0x28,               // Display off
        DISPLAY_ON = 0x29,                // Display on
        SET_COLUMN = 0x2A,                // Column address set
        SET_PAGE = 0x2B,                  // Page address set
        WRITE_RAM = 0x2C,                 // Memory write
        READ_RAM = 0x2E,                  // Memory read
        PTLAR = 0x30,                     // Partial area
        VSCRvoid = 0x33,                  // Vertical scrolling voidinition
        MADCTL = 0x36,                    // Memory access control
        VSCRSADD = 0x37,                  // Vertical scrolling start address
        PIXFMT = 0x3A,                    // COLMOD : Pixel format set
        WRITE_DISPLAY_BRIGHTNESS = 0x51,  // Brightness hardware dependent !
        READ_DISPLAY_BRIGHTNESS = 0x52,
        WRITE_CTRL_DISPLAY = 0x53,
        READ_CTRL_DISPLAY = 0x54,
        WRITE_CABC = 0x55,          // Write Content Adaptive Brightness Control
        READ_CABC = 0x56,           // Read Content Adaptive Brightness Control
        WRITE_CABC_MINIMUM = 0x5E,  // Write CABC Minimum Brightness
        READ_CABC_MINIMUM = 0x5F,   // Read CABC Minimum Brightness
        INTFMODE = 0xB0,            // Interface Mode
        FRMCTR1 = 0xB1,             // Frame rate control(In normal mode / full COLOR)
        FRMCTR2 = 0xB2,             // Frame rate control(In idle mode / 8 COLOR)
        FRMCTR3 = 0xB3,             // Frame rate control(In partial mode / full COLOR)
        INVCTR = 0xB4,              // Display inversion control
        DFUNCTR = 0xB6,             // Display function control
        PWCTR1 = 0xC0,              // Power control 1
        PWCTR2 = 0xC1,              // Power control 2
        PWCTRA = 0xCB,              // Power control A
        PWCTRB = 0xCF,              // Power control B
        VMCTR1 = 0xC5,              // VCOM control 1
        VMCTR2 = 0xC7,              // VCOM control 2
        RDID1 = 0xDA,               // Read ID 1
        RDID2 = 0xDB,               // Read ID 2
        RDID3 = 0xDC,               // Read ID 3
        RDID4 = 0xDD,               // Read ID 4
        GMCTRP1 = 0xE0,             // Positive gamma correction
        GMCTRN1 = 0xE1,             // Negative gamma correction
        DTCA = 0xE8,                // Driver timing control A
        IMGFUNC = 0xE9,             // Image function
        DTCB = 0xEA,                // Driver timing control B
        POSC = 0xED,                // Power on sequence control
        ENABLE3G = 0xF2,            // Enable 3 gamma control
        PUMPRC = 0xF7,              // Pump ratio control
    };

    void m_block(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *data, uint32_t data_len);
    bool m_is_off_grid(uint16_t xmin, uint16_t ymin, uint16_t xmax, uint16_t ymax);
    void m_write_data(uint8_t *data, uint16_t data_len);
    void m_write_cmd(CMDS command);
    void m_write_cmd_with_args(CMDS command, uint8_t *args, uint16_t args_len);
    bool m_create_pixel_buffer(uint8_t **data, uint32_t *data_len, const uint16_t pixel_count, COLOR);
    void m_fill_pixel_buffer(uint8_t **data, const uint32_t data_len, const COLOR color);
    uint16_t m_color_888_to_565(COLOR color);

    spi_inst_t *m_spi;
    uint8_t m_cs;
    uint8_t m_dc;
    uint8_t m_rst;
    uint8_t m_led_pwm_pin;

    uint m_pwm_slice;
    uint m_pwm_channel;

    uint16_t m_height;
    uint16_t m_width;
    ROTATIONS m_rotation;
};

#endif  // DRIVER_DISPLAY_H
