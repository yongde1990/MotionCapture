

#ifndef _PAGE_MANAGER_H
#define _PAGE_MANAGER_H

#include <input_manager.h>
#include <disp_manager.h>

typedef struct PageParams {
    int iPageID;                  /* 页面的ID */
    char strCurPictureFile[256];  /* 要处理的第1个图片文件 */
}T_PageParams, *PT_PageParams;

typedef struct PageLayout {
	int iTopLeftX;        /* 这个区域的左上角、右下角坐标 */
	int iTopLeftY;
	int iBotRightX;
	int iBotRightY;
	int iBpp;             /* 一个象素用多少位来表示 */
	int iMaxTotalBytes;
	PT_Layout atLayout;  /* 数组: 这个区域分成好几个小区域 */
}T_PageLayout, *PT_PageLayout;

typedef struct PageAction {
	char *name;            /* 页面名字 */
	void (*Run)(PT_PageParams ptParentPageParams);  /* 页面的运行函数 */
	int (*GetInputEvent)(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent);  /* 获得输入数据的函数 */
	int (*Prepare)(void);         /* (未实现)后台准备函数: 为加快程序运行而同时处理某些事情 */
	struct PageAction *ptNext;    /* 链表 */
}T_PageAction, *PT_PageAction;

/* 页面配置信息 */
typedef struct PageCfg {
    int iIntervalSecond;      /* 连播模式下图片的显示间隔 */
    char strSeletedDir[256];  /* 连播模式下要显示哪个目录下的图片 */ 
}T_PageCfg, *PT_PageCfg;



//#define ID(name)   (int(name[0]) + int(name[1]) + int(name[2]) + int(name[3]))

/**********************************************************************
 * 函数名称： ID
 * 功能描述： 根据名字算出一个唯一的整数,它用来标识VideoMem中的显示数据
 * 输入参数： strName - 名字
 * 输出参数： 无
 * 返 回 值： 一个唯一的整数
 ***********************************************************************/
int ID(char *strName);

/**********************************************************************
 * 函数名称： MainPageInit
 * 功能描述： 注册"主页面"
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int MainPageInit(void);

/**********************************************************************
 * 函数名称： MotionPageInit
 * 功能描述： 注册"MotionPageInit跟随页面"
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int MotionPageInit(void);

/**********************************************************************
 * 函数名称： PlaybackPageInit
 * 功能描述： 注册"PlaybackPageInit回放页面"
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int PlaybackPageInit(void);

/**********************************************************************
 * 函数名称： PlayPageInit
 * 功能描述： 注册"PlayPageInit执行文件页面"
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int PlayPageInit(void);

/**********************************************************************
 * 函数名称： SavePageInit
 * 功能描述： 注册"SavePageInit保存页面"
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int SavePageInit(void);

/**********************************************************************
 * 函数名称： RegisterPageAction
 * 功能描述： 注册"页面模块", "页面模块"含有页面显示的函数
 * 输入参数： ptPageAction - 一个结构体,内含"页面模块"的操作函数
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int RegisterPageAction(PT_PageAction ptPageAction);

/**********************************************************************
 * 函数名称： PagesInit
 * 功能描述： 调用各个"页面模块"的初始化函数,就是注册它们
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int PagesInit(void);

/**********************************************************************
 * 函数名称： GeneratePage
 * 功能描述： 从图标文件中解析出图像数据并放在指定区域,从而生成页面数据
 * 输入参数： ptPageLayout - 内含多个图标的文件名和显示区域
 *            ptVideoMem   - 在这个VideoMem里构造页面数据
 * 输出参数： 无
 * 返 回 值： 0      - 成功
 *            其他值 - 失败
 ***********************************************************************/
int GeneratePage(PT_PageLayout ptPageLayout, PT_VideoMem ptVideoMem);

/**********************************************************************
 * 函数名称： GenericGetInputEvent
 * 功能描述： 读取输入数据,并判断它位于哪一个图标上
 * 输入参数： ptPageLayout - 内含多个图标的显示区域
 * 输出参数： ptInputEvent - 内含得到的输入数据
 * 返 回 值： -1     - 输入数据不位于任何一个图标之上
 *            其他值 - 输入数据所落在的图标(PageLayout->atLayout数组的哪一项)
 ***********************************************************************/
int GenericGetInputEvent(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent);

/**********************************************************************
 * 函数名称： Page
 * 功能描述： 根据名字取出指定的"页面模块"
 * 输入参数： pcName - 名字
 * 输出参数： 无
 * 返 回 值： NULL   - 失败,没有指定的模块, 
 *            非NULL - "页面模块"的PT_PageAction结构体指针
 ***********************************************************************/
PT_PageAction Page(char *pcName);

/**********************************************************************
 * 函数名称： TimeMSBetween
 * 功能描述： 两个时间点的间隔:单位ms
 * 输入参数： tTimeStart - 起始时间点
 *            tTimeEnd   - 结束时间点
 * 输出参数： 无
 * 返 回 值： 间隔,单位ms
 ***********************************************************************/
int TimeMSBetween(struct timeval tTimeStart, struct timeval tTimeEnd);

/**********************************************************************
 * 函数名称： GetPageCfg
 * 功能描述： 获得页面的配置参数,
 * 输入参数： 无
 * 输出参数： ptPageCfg - 内含得到的参数
 * 返 回 值： 无
 ***********************************************************************/
void GetPageCfg(PT_PageCfg ptPageCfg);

#endif /* _PAGE_MANAGER_H */

