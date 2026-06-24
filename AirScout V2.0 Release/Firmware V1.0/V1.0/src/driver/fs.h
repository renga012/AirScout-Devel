#ifndef DRIVER_FS_H
#define DRIVER_FS_H
#include <ff.h>
int8_t initfs(FATFS *fs);
const char *FRESULT_str(FRESULT i);
uint32_t getFreeSpace();

extern bool filesystem_ok;

// wrapper

// my_f_open(FIL *fp, const TCHAR *path, BYTE mode){
//     f_open(FIL *fp, const TCHAR *path, BYTE mode)
// }

extern FATFS fs;;
#endif  // DRIVER_FS_H
