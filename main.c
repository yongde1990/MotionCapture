#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <encoding_manager.h>
#include <fonts_manager.h>
#include <disp_manager.h>
#include <input_manager.h>
#include <pic_operation.h>
#include <render.h>
#include <string.h>
#include <picfmt_manager.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>


/*  MotionCapture */
int main(int argc, char **argv)
{	
	int iError;
	/* ע�����ͨ�� */
	DebugInit();
	/* ��ʼ������ͨ�� */
	InitDebugChanel();
#if 0
	if (argc != 2)
	{
		DBG_PRINTF("Usage:\n");
		DBG_PRINTF("%s <freetype_file>\n", argv[0]);
		return 0;
	}
#endif
	/* ע�Ტ��ʼ����ʾ�豸 */
	DisplayInit();
	SelectAndInitDefaultDispDev("fb");

	/* 
	 * VideoMem: Ϊ�ӿ���ʾ�ٶ�,�����������ڴ��й������ʾ��ҳ�������,
	             (����ڴ��ΪVideoMem)
	 *           ��ʾʱ�ٰ�VideoMem�е����ݸ��Ƶ��豸���Դ���
	 * �����ĺ�����Ƿ���Ķ��ٸ�VideoMem
	 * ������ȡΪ0, ����ζ�����е���ʾ���ݶ�������ʾʱ���ֳ�����,Ȼ��д���Դ�
	 * �����ĸ�����: �����桢����ģʽ���桢
	 *                                       ����ģʽ���桢�ط�ģʽ���桢ִ���ļ�����
	 */
	AllocVideoMem(4);
    /* ע�������豸 */
	InputInit();
    /* ���ô����������豸�ĳ�ʼ������ 
      * ��ʼ�����Ͷ����������ݣ�û�а���ʱ������
      */
	AllInputDevicesInit();
    /* ע�����ģ�� */
    EncodingInit();
    /* ע���ֿ�ģ�� */
	iError = FontsInit();
	if (iError)
	{
		DBG_PRINTF("FontsInit error!\n");
	}

    /* ����freetype�ֿ����õ��ļ�������ߴ� */
	iError = SetFontsDetail("freetype", "/MSYH.TTF", 24);
	if (iError)
	{
		DBG_PRINTF("SetFontsDetail error!\n");
	}
    /* ע��ͼƬ�ļ�����ģ�� */
    PicFmtsInit();
    /* ע��ҳ�� */
    PagesInit();
    /* ������ҳ�� */
	Page("main")->Run(NULL);

	return 0;
}

