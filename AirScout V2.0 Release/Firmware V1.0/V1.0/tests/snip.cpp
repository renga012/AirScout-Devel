    /*
    disp.draw_text(10, 10, 1, "Connecting to Wifi..", &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
    wifi.enableStation();

    WSTATUS wstatus = wifi.connectToAvailable();
    ip_addr_t ipaddr;
    wifi.getSTAIP(&ipaddr);

    ulog_debug("Wifi: %s, IP: %s", wifi.statusToStr(wifi.status_STA()), ipaddr_ntoa(&ipaddr));

    disp.draw_text(10, 10, 1, "Setting RTC Via NTP..", &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
    if(wstatus == WSTATUS::OK) {
        START_PROFILE();
        if(!setRTCviaNTP()) {
            ulog_error("NTP FAILED");
        }
        END_PROFILE("NTP")
    } else{
        ulog_warn("Connection Fail");
    }
    wifi.disableStation();
*/
    // wifi_connection_t *wifi_data;
    // uint16_t wifi_data_len;

    // user_settings.getWifiConnections(&wifi_data, &wifi_data_len);
    // for(int i = 0; i < wifi_data_len; i++) {
    //     ulog_debug("SAVED SSID: %s\t, PASSWD: %s\t", wifi_data[i].ssid, wifi_data[i].passwd);
    //     wifi_connection_t_delete(&wifi_data[i]);
    // }

    // wifi.enableAP("Hello", "12456789", WAUTH::WPA2_AES_PSK);

    // // wifi.enableStation();

    // cyw43_ev_scan_result_t *scan_results;
    // uint16_t result_cnt;

    // // while(1) {
    // START_PROFILE();
    // WSTATUS status = wifi.scan(&scan_results, &result_cnt, true);
    // END_PROFILE("Wifi scan");

    // if(status != WSTATUS::OK) {
    //     ulog_error("Scan Failed");

    // } else {
    //     ulog_debug("Scanned %d nets", result_cnt);
    //     for(int i = 0; i < result_cnt; i++) {
    //         ulog_debug("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u", scan_results[i].ssid, scan_results[i].rssi, scan_results[i].channel, scan_results[i].bssid[0], scan_results[i].bssid[1], scan_results[i].bssid[2], scan_results[i].bssid[3], scan_results[i].bssid[4], scan_results[i].bssid[5], scan_results[i].auth_mode);
    //     }
    //     free(scan_results);
    //     result_cnt = 0;
    // }

    // status = wifi.status_AP();
    // ulog_info("Status: %s", wifi.statusToStr(status));
    // sleep_ms(5000);
    // }






    // wifi.connectToAvailable();
    // setRTCviaNTP();





    // FIL fp;
    // FRESULT fr;
    // const char *path = ubx_db_dump_file;
    // fr = f_open(&fp, path, FA_READ);
    // if(fr != FR_OK) {
    //     ulog_warn("Error while opening %s\n%s", path, FRESULT_str(fr));
    // }
    // uint64_t size = f_size(&fp);
    // UINT bytes_read = 0;
    // char *buffer = (char *)calloc(size, 1);

    // if(buffer == NULL) {
    //     ulog_fatal("Failed to allocate Buffer with %llu Bytes for JSON", size);
    // }

    // fr = f_read(&fp, buffer, size, &bytes_read);
    // if(fr != FR_OK) {
    //     ulog_error("Error while reading %s\n%s", path, FRESULT_str(fr));
    // }

    // fr = f_close(&fp);
    // if(fr != FR_OK) {
    //     ulog_error("Error while closing %s\n%s", path, FRESULT_str(fr));
    // }

    // for(int i =0; i < bytes_read; i++){
    //     printf("%02X", buffer[i]);
    // }

    // printf("\n");










// bar.drawStatic();
        // sleep_ms(1000);
        // printf("Changing color\n");
        // uint64_t start = micros();
        // display.clear(Display::COLOR::WHITE);
        // display.display_toggle(true);
        // printf("Changed color in %.2fms\n", (micros() - start)/1000.0);
        // printf("Hello, world!\n");
        // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN));
        // bme680.getValues(&data, &n_fields);
        // if(uart0_readable) {
        //     printf("Uart readable\n");
        //     while(uart_is_readable(uart0)) {
        //         printf("%c", uart_getc(uart0));
        //     uart0_readable = false;
        // }
        // M8Q.init();
        // uart_write_blocking(uart0, (uint8_t*)"Hello World", 12);
        // M8Q.init();
        // if(n_fields) {w
        // printf("T: %.2f,P: %.2f,H: %.2f,G: %.2f,S: 0x%x\n", data.temperature, data.pressure, data.humidity, data.gas_resistance, data.status);
        // }

        // uart_read_blocking(uart_inst_t *uart, uint8_t *dst, size_t len)

        // M8Q.m_read_nmea(&lines);
        // if(lines.buff_len > 0){

        //     puts((char *)lines.buffer);
        //     lines.buff_len = 0;
        //     lines.read_bytes = 0;
        //     free(lines.buffer);
        // }

        // memset(&gnss_data, 0, sizeof(gnss_data_t));
        // // M8Q.getValues(&gnss_data);
        // if(gnss_data.newData && gnss_data.valid) {
        //     char tmp[200];
        //     datetime_to_str(tmp, 200, &gnss_data.utc);
        //     ulog_trace("GNSS DATA: %s , %f Lat, %f long at %fm Altitude, Fix: %d, ", tmp, gnss_data.latitude, gnss_data.longitude, gnss_data.altitude, gnss_data.fix);
        //     gnss_data.valid ? ulog_trace("Valid\n") : ulog_trace("Invalid\n");
        // }

        // if(uart0_rx_data.data_read > 0) {
        //     M8Q.processUBX();
        // }
        // if(count > 10) {
        //     count = 0;
        //     M8Q.dump_db();
        // }
        // count++;

        // sleep_ms(250);
        // START_PROFILE();
        // for(int i = 0; i < meas_data_len; i++) {
        //     meas_data[i] = getAllValues();

        //     // ulog_debug("Time  : %04d-%02d-%02d_%02d:%02d:%02d, is good %d", meas_data[i].time.year, meas_data[i].time.month, meas_data[i].time.day, meas_data[i].time.hour, meas_data[i].time.min, meas_data[i].time.sec, meas_data[i].time_good);
        //     // ulog_debug("Bme680: %.2f°C, %.2f%%, %.2fPa, %d Ohm, is good: %d", meas_data[i].temperature, meas_data[i].humidity, meas_data[i].pressure, meas_data[i].gas, meas_data[i].bme680_data_good);
        //     // ulog_debug("camm8q: Lat: %.8f, Long: %.8f, Alt: %.2fm, is good: %d", meas_data[i].latitude, meas_data[i].longitude, meas_data[i].altitude, meas_data[i].camm8q_data_good);
        //     sleep_ms(100);
        // }
        // END_PROFILE("get all Values");

        // char chunk_path[30];
        // sprintf(chunk_path, "chunks/chunk_%05d.json", chunk_num);
        // writeDataToFile(meas_data, meas_data_len, chunk_path);
        // chunk_num++;
        /*
    if(count > 150){
        count = -100000;
        M8Q.dump_db();
    }
    count++;*/
