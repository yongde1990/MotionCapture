
#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdio.h>
#include <debug_manager.h>

#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h> 
#include <sys/mman.h>
#include <unistd.h>

#define BUFSIZE_UART_R 56
#define BUFSIZE_UART_W 57

/*有用的data长度*/
#define UART_R_Length 54
/*115200波特率*/
#define BOUNDRATE 111

/*保存文件时，文件名字的最大长度*/
#define FileNameLength 20

#define FB_DEVICE_NAME "/dev/fb0"
#define DEFAULT_DIR "/mnt/data"

//#define COLOR_BACKGROUND   0xE7DBB5  /* 泛黄的纸 */
//#define COLOR_FOREGROUND   0x514438  /* 褐色字体 */
#define COLOR_FOREGROUND   0xffffff  /* 褐色字体 */
#define COLOR_BACKGROUND   0x66CDAA
//#define DBG_PRINTF(...)  
#define DBG_PRINTF DebugPrint

/* 图标所在目录 */
#define ICON_PATH  "/etc/MotionCapture/icons"

#endif /* _CONFIG_H */
