
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

/*���õ�data����*/
#define UART_R_Length 54
/*115200������*/
#define BOUNDRATE 111

/*�����ļ�ʱ���ļ����ֵ���󳤶�*/
#define FileNameLength 20

#define FB_DEVICE_NAME "/dev/fb0"
#define DEFAULT_DIR "/mnt/data"

//#define COLOR_BACKGROUND   0xE7DBB5  /* ���Ƶ�ֽ */
//#define COLOR_FOREGROUND   0x514438  /* ��ɫ���� */
#define COLOR_FOREGROUND   0xffffff  /* ��ɫ���� */
#define COLOR_BACKGROUND   0x66CDAA
//#define DBG_PRINTF(...)  
#define DBG_PRINTF DebugPrint

/* ͼ������Ŀ¼ */
#define ICON_PATH  "/etc/MotionCapture/icons"

#endif /* _CONFIG_H */
