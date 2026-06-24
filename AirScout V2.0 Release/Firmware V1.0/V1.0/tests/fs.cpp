
static int8_t testFs() {
    FRESULT fr;
    FATFS fs;
    FIL fil;
    int ret;
    char buf[100];
    char filename[] = "test02.txt";

    // Wait for user to press 'enter' to continue
    printf("\r\nSD card test. Press 'enter' to start.\r\n");
    while(true) {
        buf[0] = getchar();
        if((buf[0] == '\r') || (buf[0] == '\n')) {
            break;
        }
    }

    // Initialize SD card
    if(!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
        return -1;
    }
    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if(fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        return -1;
    }

    // Open file for writing ()
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if(fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        return -1;
    }

    // Write something to file
    ret = f_printf(&fil, "This is another test\r\n");
    if(ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        return -1;
    }
    ret = f_printf(&fil, "of writing to an SD card.\r\n");
    if(ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        return -1;
    }

    // Close file
    fr = f_close(&fil);
    if(fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        return -1;
    }

    // Open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if(fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        return -1;
    }

    // Print every line in file over serial
    printf("Reading from file '%s':\r\n", filename);
    printf("---\r\n");
    while(f_gets(buf, sizeof(buf), &fil)) {
        printf("%s", buf);
    }
    printf("\r\n---\r\n");

    // Close file
    fr = f_close(&fil);
    if(fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        return -1;
    }

    const char *filename2 = "test03.txt";
    fr = f_open(&fil, filename2, FA_WRITE | FA_CREATE_ALWAYS);
    if(fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        return -1;
    }

    // Write something to file
    ret = f_printf(&fil, "Hello\r\n");
    if(ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        return -1;
    }
    ret = f_printf(&fil, "test >w<\r\n");
    if(ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        return -1;
    }
    // Close file
    fr = f_close(&fil);
    if(fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        return -1;
    }

    DIR dp;
    fr = f_opendir(&dp, "/");
    if(fr != FR_OK) {
        printf("ERROR: Could not open directory (%d)", fr);
        return -1;
    }

    FILINFO infos;

    for(int i = 0; i < 20; i++) {
        fr = f_readdir(&dp, &infos);
        if(fr != FR_OK) {
            printf("ERROR: Could not read directory (%d)", fr);
            return -1;
        }

        printf("DIR: First name: %s\n", infos.fname);
    }

    fr = f_closedir(&dp);
    if(fr != FR_OK) {
        printf("ERROR: Could not close dir (%d)\r\n", fr);
        return -1;
    }
    // Unmount drive
    f_unmount("0:");
    return 0;
}