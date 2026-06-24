#include "fs.h"

#include <ff.h>
#include <pico/time.h>
#include <sd_card.h>
#include <string.h>
#include <ulog.h>

FATFS fs;
bool filesystem_ok = false;

const char *FRESULT_str(FRESULT i) {
    switch(i) {
        case FR_OK:
            return "Succeeded";
        case FR_DISK_ERR:
            return "A hard error occurred in the low level disk I/O layer";
        case FR_INT_ERR:
            return "Assertion failed";
        case FR_NOT_READY:
            return "The physical drive cannot work";
        case FR_NO_FILE:
            return "Could not find the file";
        case FR_NO_PATH:
            return "Could not find the path";
        case FR_INVALID_NAME:
            return "The path name format is invalid";
        case FR_DENIED:
            return "Access denied due to prohibited access or directory full";
        case FR_EXIST:
            return "Access denied due to prohibited access (exists)";
        case FR_INVALID_OBJECT:
            return "The file/directory object is invalid";
        case FR_WRITE_PROTECTED:
            return "The physical drive is write protected";
        case FR_INVALID_DRIVE:
            return "The logical drive number is invalid";
        case FR_NOT_ENABLED:
            return "The volume has no work area (mount)";
        case FR_NO_FILESYSTEM:
            return "There is no valid FAT volume";
        case FR_MKFS_ABORTED:
            return "The f_mkfs() aborted due to any problem";
        case FR_TIMEOUT:
            return "Could not get a grant to access the volume within defined "
                   "period";
        case FR_LOCKED:
            return "The operation is rejected according to the file sharing "
                   "policy";
        case FR_NOT_ENOUGH_CORE:
            return "LFN working buffer could not be allocated";
        case FR_TOO_MANY_OPEN_FILES:
            return "Number of open files > FF_FS_LOCK";
        case FR_INVALID_PARAMETER:
            return "Given parameter is invalid";
        default:
            return "Unknown";
    }
}

int8_t initfs(FATFS *fs) {
    FRESULT fr;
    FIL fil;
    int ret;
    char test_filename[] = "testtt.txt";

    // Initialize SD card
    if(!sd_init_driver()) {
        ulog_error("ERROR: Could not initialize SD card");
        return -1;
    }
    // Mount drive
    fr = f_mount(fs, "", 1);
    if(fr != FR_OK) {
        ulog_error("ERROR: Could not mount filesystem (%d)", fr);
        return -1;
    }
    sleep_ms(1000);
    // Open file for writing ()
    fr = f_open(&fil, test_filename, FA_WRITE | FA_CREATE_ALWAYS);
    if(fr != FR_OK) {
        ulog_error("ERROR: Could not open test file (%d)", fr);
        return -1;
    }

    // Write something to file
    ret = f_printf(&fil, "This is another test");
    if(ret < 0) {
        ulog_error("ERROR: Could not write to file (%d)", ret);
        f_close(&fil);
        return -1;
    }
    ret = f_printf(&fil, "of writing to an SD card.");
    if(ret < 0) {
        ulog_error("ERROR: Could not write to file (%d)", ret);
        f_close(&fil);
        return -1;
    }

    // Close file
    fr = f_close(&fil);
    if(fr != FR_OK) {
        ulog_error("ERROR: Could not close file (%d)", fr);
        return -1;
    }

    // Open file for reading ()
    fr = f_open(&fil, "sprites/network-wireless-disconnected.bin", FA_READ);
    if(fr != FR_OK) {
        ulog_error("ERROR: Could not open test file (%d)", fr);
        f_close(&fil);
        return -1;
    }

    UINT read;
    uint8_t buff[100];
    memset(buff, 0, 100);
    fr = f_read(&fil, buff, 100, &read);
    if(fr != FR_OK) {
        ulog_error("ERROR: Could not read test file (%d)", fr);
        f_close(&fil);
        return -1;
    }

    // Close file
    fr = f_close(&fil);
    if(fr != FR_OK) {
        ulog_error("ERROR: Could not close file (%d)", fr);
        return -1;
    }

    // DIR dp;
    // fr = f_opendir(&dp, "/");
    // if(fr != FR_OK) {
    //     ulog_error("ERROR: Could not open directory (%d)", fr);
    //     return -1;
    // }

    // FILINFO infos;

    // for(int i = 0; i < 20; i++) {
    //     fr = f_readdir(&dp, &infos);
    //     if(fr != FR_OK) {
    //         ulog_error("ERROR: Could not read directory (%d)", fr);
    //         return -1;
    //     }

    //     ulog_error("DIR: First name: %s\n", infos.fname);
    // }

    // fr = f_closedir(&dp);
    // if(fr != FR_OK) {
    //     ulog_error("ERROR: Could not close dir (%d)", fr);
    //     return -1;
    // }

    // unmount
    // f_unmount("");

    return 0;
}

// https://elm-chan.org/fsw/ff/doc/getfree.html
uint32_t getFreeSpace() {
    FATFS *fs;
    DWORD free_cluster, free_sectors;

    /* Get volume information and free clusters of drive 1 */
    FRESULT res = f_getfree("", &free_cluster, &fs);
    if(res) return 0;

    /* Get total sectors and free sectors */
    free_sectors = free_cluster * fs->csize;
    return free_sectors >> 11; // / 2 / 1024; // return MiB, 512 bytes per sector
}