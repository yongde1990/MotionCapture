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
	/* 注册调试通道 */
	DebugInit();
	/* 初始化调试通道 */
	InitDebugChanel();
#if 0
	if (argc != 2)
	{
		DBG_PRINTF("Usage:\n");
		DBG_PRINTF("%s <freetype_file>\n", argv[0]);
		return 0;
	}
#endif
	/* 注册并初始化显示设备 */
	DisplayInit();
	SelectAndInitDefaultDispDev("fb");

	/* 
	 * VideoMem: 为加快显示速度,我们事先在内存中构造好显示的页面的数据,
	             (这个内存称为VideoMem)
	 *           显示时再把VideoMem中的数据复制到设备的显存上
	 * 参数的含义就是分配的多少个VideoMem
	 * 参数可取为0, 这意味着所有的显示数据都是在显示时再现场生成,然后写入显存
	 * 共有四个界面: 主界面、跟随模式界面、
	 *                                       保存模式界面、回放模式界面、执行文件界面
	 */
	AllocVideoMem(4);
    /* 注册输入设备 */
	InputInit();
    /* 调用触摸屏输入设备的初始化函数 
      * 初始化完后就读触摸屏数据，没有按下时，休眠
      */
	AllInputDevicesInit();
    /* 注册编码模块 */
    EncodingInit();
    /* 注册字库模块 */
	iError = FontsInit();
	if (iError)
	{
		DBG_PRINTF("FontsInit error!\n");
	}

    /* 设置freetype字库所用的文件和字体尺寸 */
	iError = SetFontsDetail("freetype", "/MSYH.TTF", 24);
	if (iError)
	{
		DBG_PRINTF("SetFontsDetail error!\n");
	}
    /* 注册图片文件解析模块 */
    PicFmtsInit();
    /* 注册页面 */
    PagesInit();
    /* 运行主页面 */
	Page("main")->Run(NULL);

	return 0;
}

