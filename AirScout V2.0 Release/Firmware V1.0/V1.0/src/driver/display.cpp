#include "display.h"

#include <ff.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/spi.h>
#include <math.h>
#include <pico/time.h>
#include <qrcode.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ulog.h>

#include "driver/fs.h"
#include "globals.h"

Display::Display(spi_inst_t *spi, uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led_pwm_pin) {
    m_spi = spi;
    m_cs = cs;
    m_dc = dc;
    m_rst = rst;
    m_led_pwm_pin = led_pwm_pin;
    m_height = 480;
    m_width = 320;
    m_rotation = ROTATIONS::R270;

    m_pwm_slice = pwm_gpio_to_slice_num(m_led_pwm_pin);
    m_pwm_channel = pwm_gpio_to_channel(m_led_pwm_pin);

    // Initialize GPIO pins
    gpio_init(m_cs);
    gpio_init(m_dc);
    gpio_init(m_rst);

    gpio_set_dir(m_cs, GPIO_OUT);
    gpio_set_dir(m_dc, GPIO_OUT);
    gpio_set_dir(m_rst, GPIO_OUT);

    gpio_put(m_cs, 1);
    gpio_put(m_dc, 0);
    gpio_put(m_rst, 1);

    gpio_set_function(m_led_pwm_pin, GPIO_FUNC_PWM);

    pwm_set_wrap(m_pwm_slice, 100);
    pwm_set_clkdiv(m_pwm_slice, 16);
    pwm_set_enabled(m_pwm_slice, true);
    setBrightness(0);
}

bool Display::init() {
    reset();
    sleep(false);
    // initialization commands
    m_write_cmd(CMDS::GMCTRP1);  // PGAMCTRL(Positive Gamma Control)
    m_write_data((uint8_t[]){0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F}, 15);

    m_write_cmd(CMDS::GMCTRN1);  // NGAMCTRL(Negative Gamma Control)
    m_write_data((uint8_t[]){0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F}, 15);

    // m_write_cmd(CMDS::PWCTR1);           // Power Control 1
    // m_write_data((uint8_t[]){0x17, 0x15}, 2);

    // m_write_cmd(CMDS::PWCTR2);           // Power Control 2
    // m_write_data((uint8_t[]){0x41}, 1);

    // m_write_cmd(CMDS::VMCTR1);           // VCOM Control
    // m_write_data((uint8_t[]){0x00, 0x12, 0x80}, 3);

    m_write_cmd(CMDS::MADCTL);           // Memory Access
    m_write_data((uint8_t[]){0x48}, 1);  // MX=0, MY=1, BGR=1

    m_write_cmd(CMDS::PIXFMT);           // Pixel format
    m_write_data((uint8_t[]){0x66}, 1);  // 18-bit color

    // m_write_cmd(CMDS::INTFMODE);         // Interface mode
    // m_write_data((uint8_t[]){0x00}, 1);

    // m_write_cmd(CMDS::FRMCTR1);          // Frame rate
    // m_write_data((uint8_t[]){0xA0}, 1);  // 60Hz

    // m_write_cmd(CMDS::INVCTR);           // Display inversion
    // m_write_data((uint8_t[]){0x20}, 1);

    // m_write_cmd(CMDS::DFUNCTR);          // Display function control
    // m_write_data((uint8_t[]){0x02, 0x02, 0x3B}, 3);

    // m_write_cmd(CMDS::IMGFUNC);          // Set image function
    // m_write_data((uint8_t[]){0x00}, 1);

    // m_write_cmd(CMDS::PUMPRC);           // Adjust control
    // m_write_data((uint8_t[]){0xA9, 0x51, 0x2C, 0x82}, 4);

    // sleep(false);
    // sleep_ms(500);

    clear(COLOR::BLACK);
    display_toggle(true);
    sleep_ms(20);
    setBrightness(25);
    return true;
}

void Display::setBrightness(uint8_t percent) {
    if(percent > 100) {
        percent = 100;
    }

    pwm_set_chan_level(m_pwm_slice, m_pwm_channel, percent);
}
void Display::display_toggle(bool state) {
    if(state) {
        m_write_cmd(CMDS::DISPLAY_ON);
    } else {
        m_write_cmd(CMDS::DISPLAY_OFF);
    }
}
void Display::cleanup() {
    display_toggle(false);
    clear(COLOR::BLACK);
}
void Display::reset() {
    gpio_put(m_rst, 0);
    sleep_ms(100);
    gpio_put(m_rst, 1);
    sleep_ms(100);
}
void Display::sleep(bool enable) {
    if(enable) {
        m_write_cmd(CMDS::SLPIN);
    } else {
        m_write_cmd(CMDS::SLPOUT);
        sleep_ms(100);
    }
}
void Display::clear(COLOR color) {
    // write 4 line blocks
    static const uint8_t lines_per_block = 4;

    uint32_t data_len = 0;
    uint8_t *data = NULL;

    if(!m_create_pixel_buffer(&data, &data_len, lines_per_block * m_width, color)) {
        return;
    }

    // write blocks to display
    for(int blocks = 0; blocks < (ceil(m_height / lines_per_block)); blocks++) {
        m_block(0, lines_per_block * blocks, m_width - 1, lines_per_block * (blocks + 1), data, data_len);
    }

    free(data);
}
void Display::scroll(uint16_t y) {
}
void Display::set_scroll(uint16_t top, uint16_t bottom) {
}
void Display::draw_circle(uint16_t x0, uint16_t y0, uint16_t r, COLOR color) {
    int16_t f = 1 - r;
    int16_t dx = 1;
    int16_t dy = -r - r;
    int16_t x = 0;
    int16_t y = r;
    draw_pixel(x0, y0 + r, color);
    draw_pixel(x0, y0 - r, color);
    draw_pixel(x0 + r, y0, color);
    draw_pixel(x0 - r, y0, color);
    while(x < y) {
        if(f >= 0) {
            y -= 1;
            dy += 2;
            f += dy;
        }
        x += 1;
        dx += 2;
        f += dx;
        draw_pixel(x0 + x, y0 + y, color);
        draw_pixel(x0 - x, y0 + y, color);
        draw_pixel(x0 + x, y0 - y, color);
        draw_pixel(x0 - x, y0 - y, color);
        draw_pixel(x0 + y, y0 + x, color);
        draw_pixel(x0 - y, y0 + x, color);
        draw_pixel(x0 + y, y0 - x, color);
        draw_pixel(x0 - y, y0 - x, color);
    }
}
void Display::draw_ellipse(uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color) {
}
void Display::draw_hline(uint16_t x, uint16_t y, uint16_t w, COLOR color) {
    if(m_is_off_grid(x, y, x + w - 1, y)) {
        ulog_warn("Offgrid");
        return;
    }
    uint32_t data_len = 0;
    uint8_t *data = NULL;

    if(!m_create_pixel_buffer(&data, &data_len, w, color)) {
        return;
    }

    m_block(x, y, x + w - 1, y, data, data_len);
    free(data);
}

void Display::draw_vline(uint16_t x, uint16_t y, uint16_t h, COLOR color) {
    if(m_is_off_grid(x, y, x, y + h - 1)) {
        ulog_warn("Offgrid");
        return;
    }
    uint32_t data_len = 0;
    uint8_t *data = NULL;

    if(!m_create_pixel_buffer(&data, &data_len, h, color)) {
        return;
    }

    m_block(x, y, x, y + h - 1, data, data_len);
    free(data);
}
void Display::draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, COLOR color) {
    // Check for horizontal line
    if(y1 == y2) {
        if(x1 > x2) {
            // x1, x2 = x2, x1;
            uint16_t tmp;
            tmp = x1;
            x1 = x2;
            x2 = tmp;
        }
        draw_hline(x1, y1, x2 - x1 + 1, color);
        return;
    }
    // Check for vertical line
    if(x1 == x2) {
        if(y1 > y2) {
            // y1, y2 = y2, y1
            uint16_t tmp;
            tmp = y1;
            y1 = y2;
            y2 = tmp;
        }

        draw_vline(x1, y1, y2 - y1 + 1, color);
        return;
    }

    // Confirm coordinates in boundary
    if(m_is_off_grid(fmin(x1, x2), fmin(y1, y2), fmax(x1, x2), fmax(y1, y2))) {
        ulog_warn("Offgrid");
        return;
    }

    // Changes in x, y
    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    // Determine how steep the line is
    bool is_steep = abs(dy) > abs(dx);
    // Rotate line
    if(is_steep) {
        // x1, y1 = y1, x1
        uint16_t tmp;
        tmp = x1;
        x1 = y1;
        y1 = tmp;

        // x2, y2 = y2, x2
        tmp = x2;
        x2 = y2;
        y2 = tmp;
    }

    // Swap start and end points if necessary
    if(x1 > x2) {
        // x1, x2 = x2, x1;
        uint16_t tmp;
        tmp = x1;
        x1 = x2;
        x2 = tmp;

        // y1, y2 = y2, y1
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    // Recalculate differentials
    dx = x2 - x1;
    dy = y2 - y1;
    // Calculate error
    int16_t error = dx >> 1;

    // ystep = 1 if y1 < y2 else -1
    int8_t ystep = y1 < y2 ? 1 : -1;

    uint16_t y = y1;
    for(int x = x1; x < x2 + 1; x++) {
        // Had to reverse HW ????
        if(!is_steep) {
            draw_pixel(x, y, color);
        } else {
            draw_pixel(y, x, color);
        }
        error -= abs(dy);
        if(error < 0) {
            y += ystep;
            error += dx;
        }
    }
}
void Display::draw_lines(uint16_t coords, COLOR color) {
}  // dunno
void Display::draw_pixel(uint16_t x, uint16_t y, COLOR color) {
    if(m_is_off_grid(x, y, x, y)) {
        ulog_warn("Offgrid");
        return;
    }

    uint8_t data[3];
    data[0] = ((uint32_t)color & 0xFF0000) >> 16;
    data[1] = ((uint32_t)color & 0x00FF00) >> 8;
    data[2] = ((uint32_t)color & 0x0000FF);

    m_block(x, y, x, y, data, 3);
}
void Display::draw_polygon(uint16_t sides, uint16_t x0, uint16_t y0, uint16_t r, COLOR color) {
}  // dunno
void Display::draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color) {
    uint16_t x2 = x + w - 1;
    uint16_t y2 = y + h - 1;
    draw_hline(x, y, w, color);
    draw_hline(x, y2, w, color);
    draw_vline(x, y, h, color);
    draw_vline(x2, y, h, color);
}

void Display::draw_qrcode(QRCode *qrcode, uint16_t x, uint16_t y, uint8_t scale) {
    uint16_t x2 = x + (qrcode->size * scale) - 1;
    uint16_t y2 = y + (qrcode->size * scale) - 1;

    if(m_is_off_grid(x, y, x2, y2)) {
        return;
    }

    uint32_t data_len = BYTES_PER_PIXEL * qrcode->size * scale;
    uint8_t *data = (uint8_t *)malloc(data_len);
    if(data == NULL) {
        ulog_fatal("Display: Can't allocate %d Bytes for Buffer", data_len);
        return;
    }

    uint32_t y_lines = 0;
    for(int my = 0; my < qrcode->size; my++) {
        uint32_t x_bytes = 0;
        for(int csrx = 0; csrx < qrcode->size; csrx++) {
            // scale qr module in x-axis
            uint32_t color = qrcode_getModule(qrcode, csrx, my) ? (uint32_t)COLOR::WHITE : (uint32_t)COLOR::BLACK;
            for(int s = 0; s < scale; s++) {
                data[x_bytes] = (color & 0xFF0000) >> 16;
                x_bytes++;
                data[x_bytes] = (color & 0x00FF00) >> 8;
                x_bytes++;
                data[x_bytes] = (color & 0x0000FF);
                x_bytes++;
            }
        }

        // scale qr module in y axis
        for(int s = 0; s < scale; s++) {
            m_block(x, y + y_lines, x2, y + y_lines, data, data_len);
            y_lines++;
        }
    }

    free(data);
}

void Display::draw_sprite_from_file(const char *path, uint16_t x, uint16_t y) {
    FIL fp;
    FRESULT fr;
    fr = f_open(&fp, path, FA_READ);
    if(fr != FR_OK) {
        ulog_error("Could not open %s, %s", path, FRESULT_str(fr));
        return;
    }

    // read image dimensions
    UINT bytes_read;
    uint16_t w = 0;
    uint16_t h = 0;
    uint8_t tmp[2];

    fr = f_read(&fp, &tmp, 2, &bytes_read);
    if(fr != FR_OK) {
        ulog_error("Could not open read %d Bytes from %s, %s", 2, path, FRESULT_str(fr));
        f_close(&fp);
        return;
    }
    w = (tmp[0] << 8) | tmp[1];

    fr = f_read(&fp, &tmp, 2, &bytes_read);
    if(fr != FR_OK) {
        ulog_error("Could not open read %d Bytes from %s, %s", 2, path, FRESULT_str(fr));
        f_close(&fp);
        return;
    }
    h = (tmp[0] << 8) | tmp[1];

    uint16_t x2 = x + w - 1;
    uint16_t y2 = y + h - 1;

    if(m_is_off_grid(x, y, x2, y2)) {
        return;
    }

    uint32_t data_len = BYTES_PER_PIXEL * w;
    uint8_t *data = (uint8_t *)malloc(data_len);
    if(data == NULL) {
        ulog_fatal("Display: Can't allocate %d Bytes for Buffer", data_len);
        return;
    }

    for(int i = 0; i < h; i++) {
        fr = f_read(&fp, data, data_len, &bytes_read);
        if(fr != FR_OK) {
            ulog_error("Could not open read %d Bytes from %s, %s", data_len, path, FRESULT_str(fr));
            f_close(&fp);
            free(data);
            return;
        }
        m_block(x, y + i, x2, y + i, data, data_len);
    }
    f_close(&fp);
    free(data);
}

bool Display::draw_letter(uint16_t x, uint16_t y, const char letter, Font *font, COLOR color, COLOR background) {
    const uint16_t w = font->get_width();
    const uint16_t h = font->get_height();
    const uint8_t *letter_data;

    if(m_is_off_grid(x, y, x + w - 1, y + h - 1)) {
        return false;
    }

    font->getCharData(letter, &letter_data);

    // if a letter does not exist, a blank is inserted
    if(letter_data == NULL) {
        font->getCharData(' ', &letter_data);
    }

    uint8_t *data = NULL;
    uint32_t data_len = 0;

    if(!m_create_pixel_buffer(&data, &data_len, h, background)) {
        return false;
    }

    uint8_t linecnt = 0;
    uint32_t byteCNT = 0;

    uint32_t cursor = 0;
    for(int byte = 0; byte < font->get_bytes_per_letter(); byte++) {
        for(int i = 0; i < 8; i++) {
            cursor = byteCNT * 3;
            if(letter_data[byte] & (0x01 << i)) {
                data[cursor] = (((uint32_t)color & 0xFF0000) >> 16);
                data[cursor + 1] = (((uint32_t)color & 0x00FF00) >> 8);
                data[cursor + 2] = ((uint32_t)color & 0x0000FF);
            }

            byteCNT += 1;

            if(byteCNT >= h) {
                m_block(x + linecnt, y, x + linecnt, y + h - 1, data, data_len);
                m_fill_pixel_buffer(&data, data_len, background);
                linecnt++;
                byteCNT = 0;
                break;
            }
        }
    }
    free(data);
    return true;
}

// void Display::draw_text_by_lines(uint16_t x, uint16_t y, uint16_t spacing, char *text, font_tmp font, COLOR color, COLOR background) {}
void Display::draw_text(uint16_t x, uint16_t y, uint16_t spacing, const char *text, Font *font, COLOR color, COLOR background) {
    const uint16_t w = font->get_width();
    const uint16_t h = font->get_height();
    uint16_t xcursor = x;

    for(size_t letter_index = 0; letter_index < strlen(text); letter_index++) {
        // Get letter array and letter dimensions
        if(text[letter_index] == '\n') {
            y += h + spacing;
            xcursor = x;
            continue;

        } else if(!draw_letter(xcursor, y, text[letter_index], font, color, background)) {
            // failed
            continue;
        }

        // Fill in spacing
        if(spacing) {
            fill_hrect(xcursor + w, y, spacing, h, background);
        }
        // Position x for next letter
        xcursor += (w + spacing);
    }
}

void Display::fill_circle(uint16_t x0, uint16_t y0, uint16_t r, COLOR color) {
    int16_t f = 1 - r;
    int16_t dx = 1;
    int16_t dy = -r - r;
    int16_t x = 0;
    int16_t y = r;
    draw_vline(x0, y0 - r, 2 * r + 1, color);
    while(x < y) {
        if(f >= 0) {
            y -= 1;
            dy += 2;
            f += dy;
        }
        x += 1;
        dx += 2;
        f += dx;
        draw_vline(x0 + x, y0 - y, 2 * y + 1, color);
        draw_vline(x0 - x, y0 - y, 2 * y + 1, color);
        draw_vline(x0 - y, y0 - x, 2 * x + 1, color);
        draw_vline(x0 + y, y0 - x, 2 * x + 1, color);
    }
}
void Display::fill_ellipse(uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color) {
}
void Display::fill_hrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color) {
    if(m_is_off_grid(x, y, x + w - 1, y + h - 1)) {
        ulog_warn("Offgrid");
        return;
    }
    uint16_t chunk_height = 1024 / w;
    uint16_t chunk_count = h / chunk_height;
    uint16_t remainder = h % chunk_height;
    uint32_t chunk_size = chunk_height * w;
    uint16_t chunk_y = y;

    uint8_t *data = NULL;
    uint32_t data_len = 0;

    if(chunk_count) {
        m_create_pixel_buffer(&data, &data_len, chunk_size, color);
        for(int i = 0; i < chunk_count; i++) {
            m_block(x, chunk_y, x + w - 1, chunk_y + chunk_height - 1, data, data_len);
            chunk_y += chunk_height;
        }
        free(data);
    }
    if(remainder) {
        data = NULL;
        m_create_pixel_buffer(&data, &data_len, remainder * w, color);
        m_block(x, chunk_y, x + w - 1, chunk_y + remainder - 1, data, data_len);
        free(data);
    }
}
void Display::fill_vrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color) {
    if(m_is_off_grid(x, y, x + w - 1, y + h - 1)) {
        ulog_warn("Offgrid");
        return;
    }
    uint16_t chunk_width = 1024 / h;
    uint16_t chunk_count = w / chunk_width;
    uint16_t remainder = w % chunk_width;
    uint32_t chunk_size = chunk_width * h;
    uint16_t chunk_x = x;

    uint8_t *data = NULL;
    uint32_t data_len = 0;

    if(chunk_count) {
        m_create_pixel_buffer(&data, &data_len, chunk_size, color);
        for(int i = 0; i < chunk_count; i++) {
            m_block(chunk_x, y, chunk_x + chunk_width - 1, y + h - 1, data, data_len);
            chunk_x += chunk_width;
        }
        free(data);
    }
    if(remainder) {
        data = NULL;
        m_create_pixel_buffer(&data, &data_len, remainder * h, color);
        m_block(chunk_x, y, chunk_x + remainder - 1, y + h - 1, data, data_len);
        free(data);
    }
}
void Display::fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color) {
    if(m_is_off_grid(x, y, x + w - 1, y + h - 1)) {
        return;
    }
    if(w > h) {
        fill_hrect(x, y, w, h, color);

    } else {
        fill_vrect(x, y, w, h, color);
    }
}

void Display::fill_round_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, COLOR color) {
    fill_rectangle(x, y + radius, w + 1, h - (2 * radius), color);
    fill_rectangle(x + radius, y, w - (2 * radius), h + 1, color);

    fill_circle(x + radius, y + radius, radius, color);          // top Left
    fill_circle(x + radius, y + h - radius, radius, color);      // bottom Left
    fill_circle(x + w - radius, y + radius, radius, color);      // top right
    fill_circle(x + w - radius, y + h - radius, radius, color);  // bottom right
}

void Display::fill_polygon(uint16_t sides, uint16_t x0, uint16_t y0, uint16_t r, COLOR color) {
}  // dunno

uint16_t Display::measure_text(const char *text, Font *font, uint16_t spacing) {
    return (font->get_width() * strlen(text)) + (spacing * strlen(text));
}

// https://github.com/Bodmer/TFT_eSPI/blob/5793878d24161c1ed23ccb136f8564f332506d53/TFT_eSPI.cpp#L2662

// Draw a triangle
void Display::draw_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, Display::COLOR color) {
    draw_line(x0, y0, x1, y1, color);
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x0, y0, color);
}

/***************************************************************************************
** Function name:           fillTriangle
** Description:             Draw a filled triangle using 3 arbitrary points
***************************************************************************************/
// Fill a triangle - original Adafruit function works well and code footprint is small
void Display::fill_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, Display::COLOR color) {
    int32_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if(y0 > y1) {
        transpose(y0, y1);
        transpose(x0, x1);
    }
    if(y1 > y2) {
        transpose(y2, y1);
        transpose(x2, x1);
    }
    if(y0 > y1) {
        transpose(y0, y1);
        transpose(x0, x1);
    }

    if(y0 == y2) {  // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if(x1 < a)
            a = x1;
        else if(x1 > b)
            b = x1;
        if(x2 < a)
            a = x2;
        else if(x2 > b)
            b = x2;
        draw_hline(a, y0, b - a + 1, color);
        return;
    }

    int32_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1, sa = 0, sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).

    if(y1 == y2)
        last = y1;      // Include y1 scanline
    else
        last = y1 - 1;  // Skip it

    for(y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;

        if(a > b)
            transpose(a, b);
        draw_hline(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;

        if(a > b)
            transpose(a, b);
        draw_hline(a, y, b - a + 1, color);
    }
}

void Display::m_block(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *data, uint32_t data_len) {
    uint8_t pos[4];
    pos[0] = (x0 & 0xFF00) >> 8;
    pos[1] = (x0 & 0xFF);
    pos[2] = (x1 & 0xFF00) >> 8;
    pos[3] = (x1 & 0xFF);
    m_write_cmd_with_args(CMDS::SET_COLUMN, pos, 4);

    pos[0] = (y0 & 0xFF00) >> 8;
    pos[1] = (y0 & 0xFF);
    pos[2] = (y1 & 0xFF00) >> 8;
    pos[3] = (y1 & 0xFF);
    m_write_cmd_with_args(CMDS::SET_PAGE, pos, 4);

    m_write_cmd_with_args(CMDS::WRITE_RAM, data, data_len);
}
bool Display::m_is_off_grid(uint16_t xmin, uint16_t ymin, uint16_t xmax, uint16_t ymax) {
    if(xmin < 0) {
        ulog_warn("x-coordinate: %d below minimum of 0.", xmin);
        return true;
    }
    if(ymin < 0) {
        ulog_warn("y-coordinate: %d below minimum of 0.", ymin);
        return true;
    }
    if(xmax >= m_width) {
        ulog_warn("x-coordinate: %d above maximum of %d.", xmax, m_width - 1);
        return true;
    }
    if(ymax >= m_height) {
        ulog_warn("y-coordinate: %d above maximum of %d.", ymax, m_height - 1);
        return true;
    }
    return false;
}

void Display::m_write_data(uint8_t *data, uint16_t data_len) {
    gpio_put(m_dc, 1);
    gpio_put(m_cs, 0);
    spi_set_baudrate(m_spi, SPI1_CLOCK);
    spi_write_blocking(m_spi, data, data_len);
    gpio_put(m_cs, 1);
}
void Display::m_write_cmd(CMDS command) {
    gpio_put(m_dc, 0);
    gpio_put(m_cs, 0);
    spi_set_baudrate(m_spi, SPI1_CLOCK);
    spi_write_blocking(m_spi, (uint8_t *)&command, 1);
    gpio_put(m_cs, 1);
    gpio_put(m_dc, 1);
}
void Display::m_write_cmd_with_args(CMDS command, uint8_t *args, uint16_t args_len) {
    m_write_cmd(command);
    m_write_data(args, args_len);
}
// buffer is heap -> free()
bool Display::m_create_pixel_buffer(uint8_t **data, uint32_t *data_len, const uint16_t pixel_count, COLOR color) {
    *data_len = pixel_count * BYTES_PER_PIXEL;

    (*data) = (uint8_t *)malloc(*data_len);
    if(*data == NULL) {
        ulog_fatal("Display: Can't allocate %d Bytes for Buffer", *data_len);
        return false;
    }

    for(uint32_t i = 0; i < *data_len; i += BYTES_PER_PIXEL) {
        (*data)[i] = ((uint32_t)color & 0xFF0000) >> 16;
        (*data)[i + 1] = ((uint32_t)color & 0x00FF00) >> 8;
        (*data)[i + 2] = ((uint32_t)color & 0x0000FF);
    }
    return true;
}

void Display::m_fill_pixel_buffer(uint8_t **data, const uint32_t data_len, const COLOR color) {
    for(uint32_t i = 0; i < data_len; i += BYTES_PER_PIXEL) {
        (*data)[i] = ((uint32_t)color & 0xFF0000) >> 16;
        (*data)[i + 1] = ((uint32_t)color & 0x00FF00) >> 8;
        (*data)[i + 2] = ((uint32_t)color & 0x0000FF);
    }
}
