#include <config.h>
#include <render.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static T_Layout g_atMotionPageIconsLayout[] = {
	{0, 0, 0, 0, "motion_info.bmp"},
	{0, 0, 0, 0, "return.bmp"},
	{0, 0, 0, 0, NULL},
};

static T_PageLayout g_tMotionPageLayout = {
	.iMaxTotalBytes = 0,
	.atLayout       = g_atMotionPageIconsLayout,
};

static pthread_t g_tMotionThreadID;
static pthread_mutex_t g_tMotionThreadMutex  = PTHREAD_MUTEX_INITIALIZER; /* ������ */
static int g_bMotionThreadShouldExit = 0;

/**********************************************************************
 * �������ƣ� CalcMotionPageLayout
 * ���������� ����ҳ���и�ͼ������ֵ
 * ��������� ��
 * ��������� ptPageLayout - �ں���ͼ������Ͻ�/���½�����ֵ
 * �� �� ֵ�� ��
 ***********************************************************************/
static void  CalcMotionPageLayout(PT_PageLayout ptPageLayout)
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
	
	/* motion_infoͼ�� */
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
 * �������ƣ� ShowMotionPage
 * ���������� ��ʾ"Motionҳ��"
 * ��������� ptPageLayout - �ں����ͼ����ļ�������ʾ����
 * ��������� ��
 * �� �� ֵ�� ��
 ***********************************************************************/
static void ShowMotionPage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	int iError;

	PT_Layout atLayout = ptPageLayout->atLayout;
		
	/* 1. ����Դ� */
	ptVideoMem = GetVideoMem(ID("motion"), 1);
	if (ptVideoMem == NULL)
	{
		DBG_PRINTF("can't get video mem for Motion page!\n");
		return;
	}

	/* 2. �軭���� */

	/* �����û�м������ͼ������� */
	if (atLayout[0].iTopLeftX == 0)
	{
		CalcMotionPageLayout(ptPageLayout);
	}

	iError = GeneratePage(ptPageLayout, ptVideoMem);	

	/* 3. ˢ���豸��ȥ */
	FlushVideoMemToDev(ptVideoMem);

	/* 4. ����Դ� */
	PutVideoMem(ptVideoMem);
}


/**********************************************************************
 * �������ƣ� MotionPageGetInputEvent
 * ���������� Ϊ"Motionҳ��"�����������,�ж������¼�λ����һ��ͼ����
 * ��������� ptPageLayout - �ں����ͼ�����ʾ����
 * ��������� ptInputEvent - �ں��õ�����������
 * �� �� ֵ�� -1     - �������ݲ�λ���κ�һ��ͼ��֮��
 *            ����ֵ - �������������ڵ�ͼ��(PageLayout->atLayout�������һ��)
 ***********************************************************************/
static int MotionPageGetInputEvent(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent)
{
	return GenericGetInputEvent(ptPageLayout, ptInputEvent);
}

/**********************************************************************
 * �������ƣ� MotionThreadFunction
 * ���������� "����ҳ��"�����̺߳���:������ʾ  (���߳����ڶ�ȡ��������)
 * ��������� pVoid - δ��
 * ��������� ��
 * �� �� ֵ�� NULL   - �����˳�
 ***********************************************************************/
static void *MotionThreadFunction(void *pVoid)
{

	unsigned char i;
	int fd;
	int ret; 
	char * tmp = NULL;
	unsigned char tmp_r[BUFSIZE_UART_R];  
	unsigned char tmp_w[BUFSIZE_UART_W]; 
	unsigned char FlagData = 0;
    int bExit = 0;
/* 1.�򿪴��� */
	fd = open("/dev/uart1_2", O_RDWR);  
	if(fd < 0)  
	{  
	    printf("Error: open /dev/uart1_2 error!/n");  
	    return NULL;  
	} 
	
/*2.���ô��ڲ�����*/
	ret = ioctl(fd, BOUNDRATE, 115200);
	tmp = memset(tmp_r, 0, sizeof(tmp_r));
	tmp = memset(tmp_w, 0, sizeof(tmp_w));
	
    while (1)
    {
        /* 1. ���ж��Ƿ�Ҫ�˳� */
        pthread_mutex_lock(&g_tMotionThreadMutex);
        bExit = g_bMotionThreadShouldExit;
        pthread_mutex_unlock(&g_tMotionThreadMutex);

        if (bExit)
        {
        	/*�رմ��ڲ��ж����һ֡���ݵ���ȷ��*/
		close(fd); 
            	return NULL;
        }
  	/*����zigbee�ڵ�Ӵ��ڴ���������*/
	ret = read(fd, tmp_r, BUFSIZE_UART_R); 
	tmp_w[0] = tmp_r[0];
	tmp_w[1] = tmp_r[1];
	tmp_w[2] = 0x36;  /*data���ݳ���*/
	
	for(i = 0; i< UART_R_Length ; i++){
		tmp_w[i+3] = tmp_r[i+2];
		}
/*
	for(i = 3; i< UART_R_Length ; i++){
			if((tmp_w[i] == 0x55) ||(tmp_w[i] == 0xaa))
				FlagData = 1;
		}

	if(!(FlagData == 1))
	*/
		/*��������*/
		ret = write(fd, tmp_w, BUFSIZE_UART_W); 
	FlagData = 0;
    }
    return NULL;
}

/**********************************************************************
 * �������ƣ� MotionPageRun
 * ���������� "Motionҳ��"�����к���: ��ʾ�˵�ͼ��,�����û�����������Ӧ
 * ��������� ptParentPageParams - δ��
 * ��������� ��
 * �� �� ֵ�� ��
 ***********************************************************************/
static void MotionPageRun(PT_PageParams ptParentPageParams)
{
	int iIndex;
	T_InputEvent tInputEvent;
	int bPressed = 0;
	int iIndexPressed = -1;
    	T_PageParams tPageParams;
	g_bMotionThreadShouldExit = 0;
   	tPageParams.iPageID = ID("motion");
	
	/* 1. ��ʾҳ�� */
	ShowMotionPage(&g_tMotionPageLayout);

	/* 2. �������ڴ����߳� */
	 pthread_create(&g_tMotionThreadID, NULL, MotionThreadFunction, NULL);
	/* 3. ����GetInputEvent��������¼����������� */
	while (1)
	{	
		iIndex = MotionPageGetInputEvent(&g_tMotionPageLayout, &tInputEvent);
		if (tInputEvent.iPressure == 0)
		{
			/* ������ɿ� */
			if (bPressed)
			{
				/* �����а�ť������ */
				ReleaseButton(&g_atMotionPageIconsLayout[iIndexPressed]);
				bPressed = 0;

				if (iIndexPressed == iIndex) /* ���º��ɿ�����ͬһ����ť */
				{
					switch (iIndexPressed)
					{
						case 1: /* "������Ŀ¼"��ť */
						{
							   pthread_mutex_lock(&g_tMotionThreadMutex);
						            g_bMotionThreadShouldExit = 1;   /* MotionThreadFunction�̼߳�⵽�������Ϊ1����˳� */
						            pthread_mutex_unlock(&g_tMotionThreadMutex);
						            pthread_join(g_tMotionThreadID, NULL);  /* �ȴ����߳��˳� */
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
					PressButton(&g_atMotionPageIconsLayout[iIndexPressed]);
				}
			}
		}		
	}	
}

static T_PageAction g_tMotionPageAction = {
	.name          = "motion",
	.Run           = MotionPageRun,
	.GetInputEvent = MotionPageGetInputEvent,
};


/**********************************************************************
 * �������ƣ�MotionPageInit
 * ���������� ע��"Motionҳ��"
 * ��������� ��
 * ��������� ��
 * �� �� ֵ�� 0 - �ɹ�, ����ֵ - ʧ��
 ***********************************************************************/
int MotionPageInit(void)
{
	return RegisterPageAction(&g_tMotionPageAction);
}
