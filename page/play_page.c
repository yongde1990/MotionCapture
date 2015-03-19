#include <config.h>
#include <render.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

static T_Layout g_atPlayPageIconsLayout[] = {
	{0, 0, 0, 0, "play_info.bmp"},
	{0, 0, 0, 0, "return.bmp"},
	{0, 0, 0, 0, NULL},
};

static T_PageLayout g_tPlayPageLayout = {
	.iMaxTotalBytes = 0,
	.atLayout       = g_atPlayPageIconsLayout,
};

static unsigned char tmp_w[BUFSIZE_UART_W]; 
//static unsigned char tmp_w; 
static int fd;

static struct itimerval Tick;

static pthread_t g_tPlayThreadID;
static pthread_mutex_t g_tPlayThreadMutex  = PTHREAD_MUTEX_INITIALIZER; /* 互斥量 */
static int g_bPlayThreadShouldExit = 0;

/**********************************************************************
 * 函数名称： CalcPlayPageLayout
 * 功能描述： 计算页面中各图标座标值
 * 输入参数： 无
 * 输出参数： ptPageLayout - 内含各图标的左上角/右下角座标值
 * 返 回 值： 无
 ***********************************************************************/
static void  CalcPlayPageLayout(PT_PageLayout ptPageLayout)
{
	int iStartY;
	int iWidth;
	int iHeight;
	int iXres, iYres, iBpp;
	int iTmpTotalBytes;
	PT_Layout atLayout;

	atLayout = ptPageLayout->atLayout;
	GetDispResolution(&iXres, &iYres, &iBpp);
	ptPageLayout->iBpp = iBpp;

	/*   
	 *    ----------------------
	 *                           1/2 * iHeight
	 *         		 return  iHeight
	 *                           1/2 * iHeight
	 *    ----------------------
	 */
	 
	iHeight = iYres * 2 / 10;
	iWidth  = iHeight*2;
	iStartY = iHeight;
	
	/* play_info图标 */
	atLayout[0].iTopLeftY  = iStartY;
	atLayout[0].iBotRightY = atLayout[0].iTopLeftY + iHeight*2 -1;
	atLayout[0].iTopLeftX  = (iXres - iWidth * 2) / 2;
	atLayout[0].iBotRightX = atLayout[0].iTopLeftX + iWidth * 2  - 1;

	iTmpTotalBytes = (atLayout[0].iBotRightX - atLayout[0].iTopLeftX + 1) * (atLayout[0].iBotRightY - atLayout[0].iTopLeftY + 1) * iBpp / 8;
	if (ptPageLayout->iMaxTotalBytes < iTmpTotalBytes)
	{
		ptPageLayout->iMaxTotalBytes = iTmpTotalBytes;
	}
	/* return图标 */
	atLayout[1].iTopLeftY  = atLayout[0].iBotRightY + (iHeight/2)  + 1;
	atLayout[1].iBotRightY = atLayout[1].iTopLeftY + iHeight - 1;
	atLayout[1].iTopLeftX  = iXres - iWidth;
	atLayout[1].iBotRightX = atLayout[1].iTopLeftX + iHeight - 1;

	iTmpTotalBytes = (atLayout[1].iBotRightX - atLayout[1].iTopLeftX + 1) * (atLayout[1].iBotRightY - atLayout[1].iTopLeftY + 1) * iBpp / 8;
	if (ptPageLayout->iMaxTotalBytes < iTmpTotalBytes)
	{
		ptPageLayout->iMaxTotalBytes = iTmpTotalBytes;
	}
}


/**********************************************************************
 * 函数名称： ShowPlayPage
 * 功能描述： 显示"Play页面"
 * 输入参数： ptPageLayout - 内含多个图标的文件名和显示区域
 * 输出参数： 无
 * 返 回 值： 无
 ***********************************************************************/
static void ShowPlayPage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	int iError;

	PT_Layout atLayout = ptPageLayout->atLayout;
		
	/* 1. 获得显存 */
	ptVideoMem = GetVideoMem(ID("mplay"), 1);
	if (ptVideoMem == NULL)
	{
		DBG_PRINTF("can't get video mem for Play page!\n");
		return;
	}

	/* 2. 描画数据 */

	/* 如果还没有计算过各图标的坐标 */
	if (atLayout[0].iTopLeftX == 0)
	{
		CalcPlayPageLayout(ptPageLayout);
	}

	iError = GeneratePage(ptPageLayout, ptVideoMem);	

	/* 3. 刷到设备上去 */
	FlushVideoMemToDev(ptVideoMem);

	/* 4. 解放显存 */
	PutVideoMem(ptVideoMem);
}


/**********************************************************************
 * 函数名称： PlayPageGetInputEvent
 * 功能描述： 为"Play页面"获得输入数据,判断输入事件位于哪一个图标上
 * 输入参数： ptPageLayout - 内含多个图标的显示区域
 * 输出参数： ptInputEvent - 内含得到的输入数据
 * 返 回 值： -1     - 输入数据不位于任何一个图标之上
 *            其他值 - 输入数据所落在的图标(PageLayout->atLayout数组的哪一项)
 ***********************************************************************/
static int PlayPageGetInputEvent(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent)
{
	return GenericGetInputEvent(ptPageLayout, ptInputEvent);
}

static void TxUart(int num) {
//	unsigned char i = 0;
	char BytesWrite;
	if((tmp_w[0] == 0x55)&&(tmp_w[1] == 0xaa))
		BytesWrite=write(fd ,tmp_w,BUFSIZE_UART_W);
} 

/**********************************************************************
 * 函数名称： PlayThreadFunction
 * 功能描述： "连播页面"的子线程函数:用于显示  (主线程用于读取输入数据)
 * 输入参数： pVoid - 未用
 * 输出参数： 无
 * 返 回 值： NULL   - 正常退出
 ***********************************************************************/
static void *PlayThreadFunction(void *pVoid)
{
	int bExit = 0;
	unsigned char i = 0;
	unsigned int j = 0;
	unsigned int IntegerData; 

	int res = 0;  

//	char BytesWrite;
	
	int fd_hex;
	int ret;
	
	struct stat tStat;
	unsigned char * ptFileMap = NULL;
	unsigned char * ptTmpMap = NULL;
	
	PT_PageParams tpFileInfo;
	unsigned char ucCount = 0;
	
	tpFileInfo = (PT_PageParams)pVoid;

	memset(&Tick, 0, sizeof(Tick));  
	/*初始唤醒时间*/
	  Tick.it_value.tv_sec = 0; 
	  Tick.it_value.tv_usec = 50000;
	/* 间隔时间 */
	  Tick.it_interval.tv_sec = 0;  
	  Tick.it_interval.tv_usec = 50000;  
	
/* 1.打开串口 ,打开存储文件*/
/* 1.1打开串口设备 */
	fd = open("/dev/uart1_2", O_RDWR);  
	if(fd < 0)  
	{  
	    printf("Error: open /dev/uart1_2 error!/n");  
	    return NULL;  
	} 
/* 1.2打开文件及映射文件到内存中 */
	if((fd_hex=open(tpFileInfo->strCurPictureFile,O_RDWR))==-1) 
	{ 
		fprintf(stderr,"Open %s Error:%s/n",tpFileInfo->strCurPictureFile,strerror(errno)); 
		exit(1); 
	} 
	//printf("file open %s\n",tpFileInfo->strCurPictureFile);
	fstat(fd_hex, &tStat);
	ptFileMap = (unsigned char *)mmap(NULL , tStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_hex, 0);
	if (ptFileMap == (unsigned char *)-1)
	{
		printf("faile mmap\n");
		return NULL;
	}
	ptTmpMap = ptFileMap;
/*2.设置串口波特率*/
	ret = ioctl(fd, BOUNDRATE, 115200);
	signal(SIGALRM, TxUart);
	res = setitimer(ITIMER_REAL, &Tick, NULL);
	 if (res) {  
    		printf("Set timer failed!\n");
  	}  
/*3.  从存储文件中发送BUFSIZE_UART_W字节数据到cotx-m3 */
	while(!((*ptFileMap) == 0x55)){
		ucCount++;
		ptFileMap++;
	}
	if(*(++ptFileMap) == 0xaa){
		ptFileMap = ptTmpMap + ucCount;
		
		IntegerData = (tStat.st_size - ucCount)/BUFSIZE_UART_W;
		
		for(j = 0; j < IntegerData; j++){
			for(i = 0; i < BUFSIZE_UART_W; i++)
				{
				tmp_w[i]  = ptFileMap[(j*57) + i];
				//printf("j = %d, i = %d, tmp_w = %0x\n",j,(i+1),tmp_w[i]);
				//BytesWrite=write(fd ,&tmp_w,1);
			}
		pause();
			}
		}
/* 4 释放资源，关闭串口，关闭文件*/
	munmap(ptFileMap,tStat.st_size);
	close(fd_hex);
	close(fd);
    while (1)
    {
        /* 1. 先判断是否要退出 */
        pthread_mutex_lock(&g_tPlayThreadMutex);
        bExit = g_bPlayThreadShouldExit;
        pthread_mutex_unlock(&g_tPlayThreadMutex);

        if (bExit)
        {	
		return NULL;
        }
    }
    return NULL;
}

/**********************************************************************
 * 函数名称： PlayPageRun
 * 功能描述： "Play页面"的运行函数: 显示菜单图标,根据用户输入作出反应
 * 输入参数： ptParentPageParams - 未用
 * 输出参数： 无
 * 返 回 值： 无
 ***********************************************************************/
static void PlayPageRun(PT_PageParams ptParentPageParams)
{
	int iIndex;
	T_InputEvent tInputEvent;
	int bPressed = 0;
	int iIndexPressed = -1;
    	T_PageParams tPageParams;
	
	g_bPlayThreadShouldExit = 0;
   	tPageParams.iPageID = ID("mplay");
	snprintf(tPageParams.strCurPictureFile, 256, "%s", ptParentPageParams->strCurPictureFile);
	tPageParams.strCurPictureFile[255] = '\0';

	/* 1. 显示页面 */
	ShowPlayPage(&g_tPlayPageLayout);

	/* 2. 创建串口传输和保存数据线程 */
	 pthread_create(&g_tPlayThreadID, NULL, PlayThreadFunction, &tPageParams);
	/* 3. 调用GetInputEvent获得输入事件，进而处理 */
	while (1)
	{	
		iIndex = PlayPageGetInputEvent(&g_tPlayPageLayout, &tInputEvent);
		if (tInputEvent.iPressure == 0)
		{
			/* 如果是松开 */
			if (bPressed)
			{
				/* 曾经有按钮被按下 */
				ReleaseButton(&g_atPlayPageIconsLayout[iIndexPressed]);
				bPressed = 0;

				if (iIndexPressed == iIndex) /* 按下和松开都是同一个按钮 */
				{
					switch (iIndexPressed)
					{
						case 1: /* "返回回放目录"按钮 */
						{
							   pthread_mutex_lock(&g_tPlayThreadMutex);
						            g_bPlayThreadShouldExit = 1;   /* PlayThreadFunction线程检测到这个变量为1后会退出 */
						            pthread_mutex_unlock(&g_tPlayThreadMutex);
						            pthread_join(g_tPlayThreadID, NULL);  /* 等待子线程退出 */
							return;
						}
						default:
						{
							break;
						}
					}
				}

				iIndexPressed = -1;
			}
		}
		else
		{
			/* 按下状态 */
			if (iIndex != -1)
			{
				if (!bPressed)
				{
					/* 未曾按下按钮 */
					bPressed = 1;
					iIndexPressed = iIndex;
					PressButton(&g_atPlayPageIconsLayout[iIndexPressed]);
				}
			}
		}		
	}	
}

static T_PageAction g_tPlayPageAction = {
	.name          = "mplay",
	.Run           = PlayPageRun,
	.GetInputEvent = PlayPageGetInputEvent,
};


/**********************************************************************
 * 函数名称：PlayPageInit
 * 功能描述： 注册"Play页面"
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int PlayPageInit(void)
{
	return RegisterPageAction(&g_tPlayPageAction);
}
