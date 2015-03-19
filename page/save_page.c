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
static pthread_mutex_t g_tSaveThreadMutex  = PTHREAD_MUTEX_INITIALIZER; /* ������ */
static int g_bSaveThreadShouldExit = 0;

/**********************************************************************
 * �������ƣ� CalcSavePageLayout
 * ���������� ����ҳ���и�ͼ������ֵ
 * ��������� ��
 * ��������� ptPageLayout - �ں���ͼ������Ͻ�/���½�����ֵ
 * �� �� ֵ�� ��
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
	
	/* save_infoͼ�� */
	atLayout[0].iTopLeftY  = iStartY;
	atLayout[0].iBotRightY = atLayout[0].iTopLeftY + iHeight*2 -1;
	atLayout[0].iTopLeftX  = (iXres - iWidth * 2) / 2;
	atLayout[0].iBotRightX = atLayout[0].iTopLeftX + iWidth * 2  - 1;

	iTmpTotalBytes = (atLayout[0].iBotRightX - atLayout[0].iTopLeftX + 1) * (atLayout[0].iBotRightY - atLayout[0].iTopLeftY + 1) * iBpp / 8;
	if (ptPageLayout->iMaxTotalBytes < iTmpTotalBytes)
	{
		ptPageLayout->iMaxTotalBytes = iTmpTotalBytes;
	}
	/* returnͼ�� */
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
 * �������ƣ� ShowSavePage
 * ���������� ��ʾ"Saveҳ��"
 * ��������� ptPageLayout - �ں����ͼ����ļ�������ʾ����
 * ��������� ��
 * �� �� ֵ�� ��
 ***********************************************************************/
static void ShowSavePage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	int iError;

	PT_Layout atLayout = ptPageLayout->atLayout;
		
	/* 1. ����Դ� */
	ptVideoMem = GetVideoMem(ID("save"), 1);
	if (ptVideoMem == NULL)
	{
		DBG_PRINTF("can't get video mem for Save page!\n");
		return;
	}

	/* 2. �軭���� */

	/* �����û�м������ͼ������� */
	if (atLayout[0].iTopLeftX == 0)
	{
		CalcSavePageLayout(ptPageLayout);
	}

	iError = GeneratePage(ptPageLayout, ptVideoMem);	

	/* 3. ˢ���豸��ȥ */
	FlushVideoMemToDev(ptVideoMem);

	/* 4. ����Դ� */
	PutVideoMem(ptVideoMem);
}


/**********************************************************************
 * �������ƣ� SavePageGetInputEvent
 * ���������� Ϊ"Saveҳ��"�����������,�ж������¼�λ����һ��ͼ����
 * ��������� ptPageLayout - �ں����ͼ�����ʾ����
 * ��������� ptInputEvent - �ں��õ�����������
 * �� �� ֵ�� -1     - �������ݲ�λ���κ�һ��ͼ��֮��
 *            ����ֵ - �������������ڵ�ͼ��(PageLayout->atLayout�������һ��)
 ***********************************************************************/
static int SavePageGetInputEvent(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent)
{
	return GenericGetInputEvent(ptPageLayout, ptInputEvent);
}

/**********************************************************************
 * �������ƣ� SaveThreadFunction
 * ���������� "����ҳ��"�����̺߳���:������ʾ  (���߳����ڶ�ȡ��������)
 * ��������� pVoid - δ��
 * ��������� ��
 * �� �� ֵ�� NULL   - �����˳�
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
	/*����ʱ��(����)�����ļ�����*/
	struct tm *Time;
   	time_t tTime;
	char strFileName[FileNameLength];
	
	/* 1.�򿪴��� ,�򿪴洢�ļ�*/
	fd = open("/dev/uart1_2", O_RDWR);  
	if(fd < 0)  
	{  
	    printf("Error: open /dev/uart1_2 error!/n");  
	    return NULL;  
	} 

	/* 2. �����ļ�*/
	
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

/*3.���ô��ڲ�����*/
	ret = ioctl(fd, BOUNDRATE, 115200);
	tmp = memset(tmp_r, 0, sizeof(tmp_r));
	tmp = memset(tmp_w, 0, sizeof(tmp_w));
	
    while (1)
    {
        /* 1. ���ж��Ƿ�Ҫ�˳� */
        pthread_mutex_lock(&g_tSaveThreadMutex);
        bExit = g_bSaveThreadShouldExit;
        pthread_mutex_unlock(&g_tSaveThreadMutex);

        if (bExit)
        {
        	/*
        	* �رմ��ڲ��ж����һ֡���ݵ���ȷ��
        	* �رմ洢���ݵ��ļ�
        	*/
		close(fd); 
		close(fd_hex);
		return NULL;
        }
  	/*
  	* ����zigbee�ڵ�Ӵ��ڴ���������
  	* ���浽�洢�ļ�
	*/

	ret = read(fd, tmp_r, BUFSIZE_UART_R); 
	tmp_w[0] = tmp_r[0];
	tmp_w[1] = tmp_r[1];
	tmp_w[2] = 0x36;  /*data���ݳ���*/
	
	for(i = 0; i< UART_R_Length ; i++)
		tmp_w[i+3] = tmp_r[i+2];

	/*��������*/
	ret = write(fd, tmp_w, BUFSIZE_UART_W); 
	
	bytes_write=write(fd_hex,tmp_w,BUFSIZE_UART_W);
    	}	 
    return NULL;
}

/**********************************************************************
 * �������ƣ� SavePageRun
 * ���������� "Saveҳ��"�����к���: ��ʾ�˵�ͼ��,�����û�����������Ӧ
 * ��������� ptParentPageParams - δ��
 * ��������� ��
 * �� �� ֵ�� ��
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
	
	/* 1. ��ʾҳ�� */
	ShowSavePage(&g_tSavePageLayout);

	/* 2. �������ڴ���ͱ��������߳� */
	 pthread_create(&g_tSaveThreadID, NULL, SaveThreadFunction, NULL);
	/* 3. ����GetInputEvent��������¼����������� */
	while (1)
	{	
		iIndex = SavePageGetInputEvent(&g_tSavePageLayout, &tInputEvent);
		if (tInputEvent.iPressure == 0)
		{
			/* ������ɿ� */
			if (bPressed)
			{
				/* �����а�ť������ */
				ReleaseButton(&g_atSavePageIconsLayout[iIndexPressed]);
				bPressed = 0;

				if (iIndexPressed == iIndex) /* ���º��ɿ�����ͬһ����ť */
				{
					switch (iIndexPressed)
					{
						case 1: /* "������Ŀ¼"��ť */
						{
							   pthread_mutex_lock(&g_tSaveThreadMutex);
						            g_bSaveThreadShouldExit = 1;   /* SaveThreadFunction�̼߳�⵽�������Ϊ1����˳� */
						            pthread_mutex_unlock(&g_tSaveThreadMutex);
						            pthread_join(g_tSaveThreadID, NULL);  /* �ȴ����߳��˳� */
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
			/* ����״̬ */
			if (iIndex != -1)
			{
				if (!bPressed)
				{
					/* δ�����°�ť */
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
 * �������ƣ�SavePageInit
 * ���������� ע��"Saveҳ��"
 * ��������� ��
 * ��������� ��
 * �� �� ֵ�� 0 - �ɹ�, ����ֵ - ʧ��
 ***********************************************************************/
int SavePageInit(void)
{
	return RegisterPageAction(&g_tSavePageAction);
}
