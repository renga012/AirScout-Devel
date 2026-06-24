#ifndef GUI_FONTS_FONT_H
#define GUI_FONTS_FONT_H
#include <stddef.h>
#include <stdint.h>
#include <ulog.h>

class Font {
public:
    virtual void getCharData(const char c, const uint8_t **data) = 0;
    virtual inline const uint16_t get_width() = 0;
    virtual inline const uint16_t get_height() = 0;
    virtual inline const uint8_t get_letter_count() = 0;
    virtual inline const uint16_t get_bytes_per_letter() = 0;
    virtual inline const char *get_name() = 0;

private:
    const uint8_t *m_chars = NULL;
    const uint8_t **m_char_data = NULL;
    const uint16_t m_width = 0;
    const uint16_t m_height = 0;
    const uint8_t m_letter_count = 0;
    const uint16_t m_bytes_per_letter = 0;
    const char *m_name = "Empty Font";
};

#endif  // GUI_FONTS_FONT_H
