#include "setupMode.h"

#include <pico/time.h>
#include <qrcode.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include <sha256.h>
}

#include "driver/display.h"
#include "driver/init_hardware.h"
#include "globals.h"
#include "gui/elements/menu_bar.h"
#include "gui/elements/status_bar.h"
#include "main.h"
#include "modes/baseMode.h"
#include "ulog.h"
#include "utils.h"

SetupMode::SetupMode() {
}

bool SetupMode::init() {
    static const uint8_t qr_version = 8;
    static const uint8_t qr_scale = 5;
    QRCode qrcode;
    char *token;
    uint8_t hash[SHA256_BLOCK_SIZE];
    char hash_char[UID_LEN];  // same length as UID of the Airscout
    SHA256_CTX ctx;
    uint8_t *qrcodeBytes = (uint8_t *)malloc(qrcode_getBufferSize(qr_version));

    generateUniqueId();
    user_settings.getString("token", &token);

    // hash token again for qr code
    sha256_init(&ctx);
    sha256_update(&ctx, token, strlen(token));
    sha256_final(&ctx, hash);

    // set some bits, each array is SHA256_BLOCK_SIZE long
    static const uint8_t mask_OR[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x40, 0x00, 0x40, 0x00, 0x08, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x20, 0x09, 0x20, 0x00};   // |
    static const uint8_t mask_AND[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xff, 0xef, 0xff, 0xff, 0x7f, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfa, 0xff, 0xfe, 0xff, 0xf3, 0xdf, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf};  // &
    for(int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        hash[i] = (hash[i] | mask_OR[i]) & mask_AND[i];
    }

    // convert hash to string
    snprintf(hash_char, UID_LEN, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15], hash[16], hash[17], hash[18], hash[19], hash[20], hash[21], hash[22], hash[23], hash[24], hash[25], hash[26], hash[27], hash[28], hash[29], hash[30], hash[31]);

    // generate qrcode
    uint16_t url_len = strlen(ADD_AIRSCOUT_URL_TEMPLATE) + strlen(hash_char);
    char *url = (char *)calloc(1, url_len);
    snprintf(url, url_len, ADD_AIRSCOUT_URL_TEMPLATE, hash_char);
    qrcode_initText(&qrcode, qrcodeBytes, qr_version, ECC_MEDIUM, url);

    ulog_info("URL %s", url);

    // do gui stuff
    menu_bar.setLayout(GUI::Menu_Bar::Icons::CHECKMARK, GUI::Menu_Bar::Icons::CROSS, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE);
    disp.draw_text(70, STATUS_BAR_HEIGHT + 5, 1, "Add AirScout", &font_Liberation_Mono13x21, Display::COLOR::WHEAT, Display::COLOR::BLACK);
    disp.draw_text(60, STATUS_BAR_HEIGHT + 25, 1, "to your Account", &font_Liberation_Mono13x21, Display::COLOR::WHEAT, Display::COLOR::BLACK);
    disp.draw_qrcode(&qrcode, 37, STATUS_BAR_HEIGHT + 50, qr_scale);

    disp.draw_text(15, STATUS_BAR_HEIGHT + 50 + qr_scale * 49 + 10, 1, "or use the token on the website:", &font_Liberation_Mono8x13, Display::COLOR::WHEAT, Display::COLOR::BLACK);

    char tmp[33] = {0};
    tmp[32] = 0;
    memcpy(tmp, hash_char, 32);
    disp.draw_text(15, STATUS_BAR_HEIGHT + 50 + qr_scale * 49 + 10 + 15, 1, tmp, &font_Liberation_Mono8x13, Display::COLOR::WHEAT, Display::COLOR::BLACK);

    memcpy(tmp, hash_char + 32, 32);
    disp.draw_text(15, STATUS_BAR_HEIGHT + 50 + qr_scale * 49 + 10 + 30, 1, tmp, &font_Liberation_Mono8x13, Display::COLOR::WHEAT, Display::COLOR::BLACK);

    free(token);
    free(qrcodeBytes);
    free(url);
    return true;
}

void SetupMode::run() {
    static absolute_time_t lastBTNCheck = 0;
    if(absolute_time_diff_us(lastBTNCheck, get_absolute_time()) > BUTTON_CHECK_INTERVAL_MS * 1000) {
        lastBTNCheck = get_absolute_time();
        BTN btn = getNextButton();
        while(btn != BTN::NONE) {
            switch(btn) {
                case BTN::BTN1:
                    user_settings.setBool("is_set_up", true);
                    switchToMode(MODES::Idle);
                    break;

                case BTN::BTN2:
                    switchToMode(MODES::Standby);
                    break;

                default:
                    break;
            }
            btn = getNextButton();
        }
    }
}
void SetupMode::exit() {
}