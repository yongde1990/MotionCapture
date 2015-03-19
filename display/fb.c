#include <config.h>
#include <disp_manager.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>

static int FBDeviceInit(void);
static int FBShowPixel(int iX, int iY, unsigned int dwColor);
static int FBCleanScreen(unsigned int dwBackColor);
static int FBShowPage(PT_VideoMem ptVideoMem);

/*
*系统用来关联是那个设备被打开
*液晶屏设备的文件描述符
*/
static int g_fd;
/*液晶屏可变参数*/
static struct fb_var_screeninfo g_tFBVar;
/*液晶屏固定参数*/
static struct fb_fix_screeninfo g_tFBFix;			
static unsigned char *g_pucFBMem;
static unsigned int g_dwScreenSize;
/*液晶屏一行占有多少字节*/
static unsigned int g_dwLineWidth;
/*液晶屏一个像素占有多少字节*/
static unsigned int g_dwPixelWidth;

static T_DispOpr g_tFBOpr = {
	.name        = "fb",
	.DeviceInit  = FBDeviceInit,
	.ShowPixel   = FBShowPixel,
	.CleanScreen = FBCleanScreen,
	.ShowPage    = FBShowPage,
};

/**********************************************************************
 * 函数名称： FBDeviceInit
 * 功能描述： "framebuffer显示设备"的初始化函数
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
static int FBDeviceInit(void)
{
	int ret;
	
	g_fd = open(FB_DEVICE_NAME, O_RDWR);
	if (0 > g_fd)
	{
		DBG_PRINTF("can't open %s\n", FB_DEVICE_NAME);
	}
	/*获得可变参数*/
	ret = ioctl(g_fd, FBIOGET_VSCREENINFO, &g_tFBVar);
	if (ret < 0)
	{
		DBG_PRINTF("can't get fb's var\n");
		return -1;
	}
	/*获得固定参数*/
	ret = ioctl(g_fd, FBIOGET_FSCREENINFO, &g_tFBFix);
	if (ret < 0)
	{
		DBG_PRINTF("can't get fb's fix\n");
		return -1;
	}
	/*本程序应用的4.3寸屏，x = 480 ; y = 272; 每个像素占2个字节*/
	g_dwScreenSize = g_tFBVar.xres * g_tFBVar.yres * g_tFBVar.bits_per_pixel / 8;
	/*mmap功能:直接将物理内存直接映射到用户虚拟内存，使用户空间可以直接对物理空间操作*/
	g_pucFBMem = (unsigned char *)mmap(NULL , g_dwScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
	if (0 > g_pucFBMem)	
	{
		DBG_PRINTF("can't mmap\n");
		return -1;
	}

	g_tFBOpr.iXres       = g_tFBVar.xres;
	g_tFBOpr.iYres       = g_tFBVar.yres;
	g_tFBOpr.iBpp        = g_tFBVar.bits_per_pixel;
	g_tFBOpr.iLineWidth  = g_tFBVar.xres * g_tFBOpr.iBpp / 8;
	g_tFBOpr.pucDispMem  = g_pucFBMem;

	g_dwLineWidth  = g_tFBVar.xres * g_tFBVar.bits_per_pixel / 8;
	g_dwPixelWidth = g_tFBVar.bits_per_pixel / 8;
	
	return 0;
}


/**********************************************************************
 * 函数名称： FBShowPixel
 * 功能描述： 设置FrameBuffer的指定象素为某颜色
 * 输入参数： iX - 象素的X坐标
 *            iY - 象素的Y坐标
 *            dwColor - 颜色值,格式为32Bpp,即0x00RRGGBB
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
static int FBShowPixel(int iX, int iY, unsigned int dwColor)
{
	unsigned char *pucFB;/*液晶屏中描绘像素的位置*/
	unsigned short *pwFB16bpp;
	unsigned int *pdwFB32bpp;
	unsigned short wColor16bpp; /* 565 */
	int iRed;
	int iGreen;
	int iBlue;

	if ((iX >= g_tFBVar.xres) || (iY >= g_tFBVar.yres))
	{
		DBG_PRINTF("out of region\n");
		return -1;
	}

	pucFB      = g_pucFBMem + g_dwLineWidth * iY + g_dwPixelWidth * iX;
	pwFB16bpp  = (unsigned short *)pucFB;
	pdwFB32bpp = (unsigned int *)pucFB;
	
	switch (g_tFBVar.bits_per_pixel)
	{
		case 8:
		{
			*pucFB = (unsigned char)dwColor;
			break;
		}
		case 16:
		{
			/* 从dwBackColor中取出红绿蓝三原色,
			 * 构造为16Bpp的颜色
			 */
			iRed   = (dwColor >> (16+3)) & 0x1f;
			iGreen = (dwColor >> (8+2)) & 0x3f;
			iBlue  = (dwColor >> 3) & 0x1f;
			wColor16bpp = (iRed << 11) | (iGreen << 5) | iBlue;
			*pwFB16bpp	= wColor16bpp;
			break;
		}
		case 32:
		{
			*pdwFB32bpp = dwColor;
			break;
		}
		default :
		{
			DBG_PRINTF("can't support %d bpp\n", g_tFBVar.bits_per_pixel);
			return -1;
		}
	}

	return 0;
}

/**********************************************************************
 * 函数名称： FBShowPage
 * 功能描述： 把PT_VideoMem中的颜色数据在FrameBuffer上显示出来
 * 输入参数： ptVideoMem - 内含整屏的象素数据
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
static int FBShowPage(PT_VideoMem ptVideoMem)
{
	/*memcpy(void *dest, const void *src, size_t count)*/
	memcpy(g_tFBOpr.pucDispMem, ptVideoMem->tPixelDatas.aucPixelDatas, ptVideoMem->tPixelDatas.iTotalBytes);
	return 0;
}

/**********************************************************************
 * 函数名称： FBCleanScreen
 * 功能描述： "framebuffer显示设备"的清屏函数
 * 输入参数： dwBackColor - 整个屏幕设置为该颜色
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
static int FBCleanScreen(unsigned int dwBackColor)
{
	unsigned char *pucFB;
	unsigned short *pwFB16bpp;
	unsigned int *pdwFB32bpp;
	unsigned short wColor16bpp; /* 565 */
	int iRed;
	int iGreen;
	int iBlue;
	int i = 0;

	pucFB      = g_pucFBMem;/*显存起始地址*/
	pwFB16bpp  = (unsigned short *)pucFB;
	pdwFB32bpp = (unsigned int *)pucFB;

	switch (g_tFBVar.bits_per_pixel)
	{
		case 8:
		{
			memset(g_pucFBMem, dwBackColor, g_dwScreenSize);
			break;
		}
		case 16:
		{
			/* 从dwBackColor中取出红绿蓝三原色,
			 * 构造为16Bpp的颜色
			 */
			iRed   = (dwBackColor >> (16+3)) & 0x1f;
			iGreen = (dwBackColor >> (8+2)) & 0x3f;
			iBlue  = (dwBackColor >> 3) & 0x1f;
			wColor16bpp = (iRed << 11) | (iGreen << 5) | iBlue;
			while (i < g_dwScreenSize)
			{
				*pwFB16bpp	= wColor16bpp;
				pwFB16bpp++;
				i += 2;
			}
			break;
		}
		case 32:
		{
			while (i < g_dwScreenSize)
			{
				*pdwFB32bpp	= dwBackColor;
				pdwFB32bpp++;
				i += 4;
			}
			break;
		}
		default :
		{
			DBG_PRINTF("can't support %d bpp\n", g_tFBVar.bits_per_pixel);
			return -1;
		}
	}

	return 0;
}

/**********************************************************************
 * 函数名称： FBInit
 * 功能描述： 注册"framebuffer显示设备"
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2013/02/08	     V1.0	  韦东山	      创建
 ***********************************************************************/
int FBInit(void)
{
	return RegisterDispOpr(&g_tFBOpr);
}

