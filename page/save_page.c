#include <config.h>
#include <render.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static T_Layout g_atSavePageIconsLayout[] = {
	{0, 0, 0, 0, "save_info.bmp"},
	{0, 0, 0, 0, "return.bmp"},
	{0, 0, 0, 0, NULL},
};

static T_PageLayout g_tSavePageLayout = {
	.iMaxTotalBytes = 0,
	.atLayout       = g_atSavePageIconsLayout,
};

static pthread_t g_tSaveThreadID;
static pthread_mutex_t g_tSaveThreadMutex  = PTHREAD_MUTEX_INITIALIZER; /* 互斥量 */
static int g_bSaveThreadShouldExit = 0;

/**********************************************************************
 * 函数名称： CalcSavePageLayout
 * 功能描述： 计算页面中各图标座标值
 * 输入参数： 无
 * 输出参数： ptPageLayout - 内含各图标的左上角/右下角座标值
 * 返 回 值： 无
 ***********************************************************************/
static void  CalcSavePageLayout(PT_PageLayout ptPageLayout)
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
	
	/* save_info图标 */
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
 * 函数名称： ShowSavePage
 * 功能描述： 显示"Save页面"
 * 输入参数： ptPageLayout - 内含多个图标的文件名和显示区域
 * 输出参数： 无
 * 返 回 值： 无
 ***********************************************************************/
static void ShowSavePage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	int iError;

	PT_Layout atLayout = ptPageLayout->atLayout;
		
	/* 1. 获得显存 */
	ptVideoMem = GetVideoMem(ID("save"), 1);
	if (ptVideoMem == NULL)
	{
		DBG_PRINTF("can't get video mem for Save page!\n");
		return;
	}

	/* 2. 描画数据 */

	/* 如果还没有计算过各图标的坐标 */
	if (atLayout[0].iTopLeftX == 0)
	{
		CalcSavePageLayout(ptPageLayout);
	}

	iError = GeneratePage(ptPageLayout, ptVideoMem);	

	/* 3. 刷到设备上去 */
	FlushVideoMemToDev(ptVideoMem);

	/* 4. 解放显存 */
	PutVideoMem(ptVideoMem);
}


/**********************************************************************
 * 函数名称： SavePageGetInputEvent
 * 功能描述： 为"Save页面"获得输入数据,判断输入事件位于哪一个图标上
 * 输入参数： ptPageLayout - 内含多个图标的显示区域
 * 输出参数： ptInputEvent - 内含得到的输入数据
 * 返 回 值： -1     - 输入数据不位于任何一个图标之上
 *            其他值 - 输入数据所落在的图标(PageLayout->atLayout数组的哪一项)
 ***********************************************************************/
static int SavePageGetInputEvent(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent)
{
	return GenericGetInputEvent(ptPageLayout, ptInputEvent);
}

/**********************************************************************
 * 函数名称： SaveThreadFunction
 * 功能描述： "连播页面"的子线程函数:用于显示  (主线程用于读取输入数据)
 * 输入参数： pVoid - 未用
 * 输出参数： 无
 * 返 回 值： NULL   - 正常退出
 ***********************************************************************/
static void *SaveThreadFunction(void *pVoid)
{
	int bExit = 0;
	char i;
	int fd;
	int fd_hex;
	int ret; 
	char tmp_r[BUFSIZE_UART_R];  
	char tmp_w[BUFSIZE_UART_W]; 
	char * tmp = NULL;
	ssize_t bytes_write;
	/*根据时间(分秒)生成文件名称*/
	struct tm *Time;
   	time_t tTime;
	char strFileName[FileNameLength];
	
	/* 1.打开串口 ,打开存储文件*/
	fd = open("/dev/uart1_2", O_RDWR);  
	if(fd < 0)  
	{  
	    printf("Error: open /dev/uart1_2 error!/n");  
	    return NULL;  
	} 

	/* 2. 创建文件*/
	
    	tTime=time(NULL);
	Time=localtime(&tTime);
	
	snprintf(strFileName, FileNameLength, "%s/%d_%d", DEFAULT_DIR,Time->tm_min, Time->tm_sec);
    	strFileName[FileNameLength - 1] = '\0';
	
	if(creat(strFileName,0777)<0){ 
	        printf("create save file %s failure!\n",strFileName); 
	        exit(EXIT_FAILURE); 
    	}
	
	if((fd_hex=open(strFileName,O_RDWR))==-1) 
	{ 
		fprintf(stderr,"Open %s Error:%s/n",strFileName,strerror(errno)); 
		exit(1); 
	} 

/*3.设置串口波特率*/
	ret = ioctl(fd, BOUNDRATE, 115200);
	tmp = memset(tmp_r, 0, sizeof(tmp_r));
	tmp = memset(tmp_w, 0, sizeof(tmp_w));
	
    while (1)
    {
        /* 1. 先判断是否要退出 */
        pthread_mutex_lock(&g_tSaveThreadMutex);
        bExit = g_bSaveThreadShouldExit;
        pthread_mutex_unlock(&g_tSaveThreadMutex);

        if (bExit)
        {
        	/*
        	* 关闭串口并判断最后一帧数据的正确性
        	* 关闭存储数据的文件
        	*/
		close(fd); 
		close(fd_hex);
		return NULL;
        }
  	/*
  	* 接收zigbee节点从串口传来的数据
  	* 保存到存储文件
	*/

	ret = read(fd, tmp_r, BUFSIZE_UART_R); 
	tmp_w[0] = tmp_r[0];
	tmp_w[1] = tmp_r[1];
	tmp_w[2] = 0x36;  /*data数据长度*/
	
	for(i = 0; i< UART_R_Length ; i++)
		tmp_w[i+3] = tmp_r[i+2];

	/*发送数据*/
	ret = write(fd, tmp_w, BUFSIZE_UART_W); 
	
	bytes_write=write(fd_hex,tmp_w,BUFSIZE_UART_W);
    	}	 
    return NULL;
}

/**********************************************************************
 * 函数名称： SavePageRun
 * 功能描述： "Save页面"的运行函数: 显示菜单图标,根据用户输入作出反应
 * 输入参数： ptParentPageParams - 未用
 * 输出参数： 无
 * 返 回 值： 无
 ***********************************************************************/
static void SavePageRun(PT_PageParams ptParentPageParams)
{
	int iIndex;
	T_InputEvent tInputEvent;
	int bPressed = 0;
	int iIndexPressed = -1;
    	T_PageParams tPageParams;
	g_bSaveThreadShouldExit = 0;
   	tPageParams.iPageID = ID("save");
	
	/* 1. 显示页面 */
	ShowSavePage(&g_tSavePageLayout);

	/* 2. 创建串口传输和保存数据线程 */
	 pthread_create(&g_tSaveThreadID, NULL, SaveThreadFunction, NULL);
	/* 3. 调用GetInputEvent获得输入事件，进而处理 */
	while (1)
	{	
		iIndex = SavePageGetInputEvent(&g_tSavePageLayout, &tInputEvent);
		if (tInputEvent.iPressure == 0)
		{
			/* 如果是松开 */
			if (bPressed)
			{
				/* 曾经有按钮被按下 */
				ReleaseButton(&g_atSavePageIconsLayout[iIndexPressed]);
				bPressed = 0;

				if (iIndexPressed == iIndex) /* 按下和松开都是同一个按钮 */
				{
					switch (iIndexPressed)
					{
						case 1: /* "返回主目录"按钮 */
						{
							   pthread_mutex_lock(&g_tSaveThreadMutex);
						            g_bSaveThreadShouldExit = 1;   /* SaveThreadFunction线程检测到这个变量为1后会退出 */
						            pthread_mutex_unlock(&g_tSaveThreadMutex);
						            pthread_join(g_tSaveThreadID, NULL);  /* 等待子线程退出 */
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
					PressButton(&g_atSavePageIconsLayout[iIndexPressed]);
				}
			}
		}		
	}	
}

static T_PageAction g_tSavePageAction = {
	.name          = "save",
	.Run           = SavePageRun,
	.GetInputEvent = SavePageGetInputEvent,
};


/**********************************************************************
 * 函数名称：SavePageInit
 * 功能描述： 注册"Save页面"
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0 - 成功, 其他值 - 失败
 ***********************************************************************/
int SavePageInit(void)
{
	return RegisterPageAction(&g_tSavePageAction);
}
