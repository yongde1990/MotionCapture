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
static pthread_mutex_t g_tPlayThreadMutex  = PTHREAD_MUTEX_INITIALIZER; /* ������ */
static int g_bPlayThreadShouldExit = 0;

/**********************************************************************
 * �������ƣ� CalcPlayPageLayout
 * ���������� ����ҳ���и�ͼ������ֵ
 * ��������� ��
 * ��������� ptPageLayout - �ں���ͼ������Ͻ�/���½�����ֵ
 * �� �� ֵ�� ��
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
	
	/* play_infoͼ�� */
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
 * �������ƣ� ShowPlayPage
 * ���������� ��ʾ"Playҳ��"
 * ��������� ptPageLayout - �ں����ͼ����ļ�������ʾ����
 * ��������� ��
 * �� �� ֵ�� ��
 ***********************************************************************/
static void ShowPlayPage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	int iError;

	PT_Layout atLayout = ptPageLayout->atLayout;
		
	/* 1. ����Դ� */
	ptVideoMem = GetVideoMem(ID("mplay"), 1);
	if (ptVideoMem == NULL)
	{
		DBG_PRINTF("can't get video mem for Play page!\n");
		return;
	}

	/* 2. �軭���� */

	/* �����û�м������ͼ������� */
	if (atLayout[0].iTopLeftX == 0)
	{
		CalcPlayPageLayout(ptPageLayout);
	}

	iError = GeneratePage(ptPageLayout, ptVideoMem);	

	/* 3. ˢ���豸��ȥ */
	FlushVideoMemToDev(ptVideoMem);

	/* 4. ����Դ� */
	PutVideoMem(ptVideoMem);
}


/**********************************************************************
 * �������ƣ� PlayPageGetInputEvent
 * ���������� Ϊ"Playҳ��"�����������,�ж������¼�λ����һ��ͼ����
 * ��������� ptPageLayout - �ں����ͼ�����ʾ����
 * ��������� ptInputEvent - �ں��õ�����������
 * �� �� ֵ�� -1     - �������ݲ�λ���κ�һ��ͼ��֮��
 *            ����ֵ - �������������ڵ�ͼ��(PageLayout->atLayout�������һ��)
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
 * �������ƣ� PlayThreadFunction
 * ���������� "����ҳ��"�����̺߳���:������ʾ  (���߳����ڶ�ȡ��������)
 * ��������� pVoid - δ��
 * ��������� ��
 * �� �� ֵ�� NULL   - �����˳�
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
	/*��ʼ����ʱ��*/
	  Tick.it_value.tv_sec = 0; 
	  Tick.it_value.tv_usec = 50000;
	/* ���ʱ�� */
	  Tick.it_interval.tv_sec = 0;  
	  Tick.it_interval.tv_usec = 50000;  
	
/* 1.�򿪴��� ,�򿪴洢�ļ�*/
/* 1.1�򿪴����豸 */
	fd = open("/dev/uart1_2", O_RDWR);  
	if(fd < 0)  
	{  
	    printf("Error: open /dev/uart1_2 error!/n");  
	    return NULL;  
	} 
/* 1.2���ļ���ӳ���ļ����ڴ��� */
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
/*2.���ô��ڲ�����*/
	ret = ioctl(fd, BOUNDRATE, 115200);
	signal(SIGALRM, TxUart);
	res = setitimer(ITIMER_REAL, &Tick, NULL);
	 if (res) {  
    		printf("Set timer failed!\n");
  	}  
/*3.  �Ӵ洢�ļ��з���BUFSIZE_UART_W�ֽ����ݵ�cotx-m3 */
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
/* 4 �ͷ���Դ���رմ��ڣ��ر��ļ�*/
	munmap(ptFileMap,tStat.st_size);
	close(fd_hex);
	close(fd);
    while (1)
    {
        /* 1. ���ж��Ƿ�Ҫ�˳� */
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
 * �������ƣ� PlayPageRun
 * ���������� "Playҳ��"�����к���: ��ʾ�˵�ͼ��,�����û�����������Ӧ
 * ��������� ptParentPageParams - δ��
 * ��������� ��
 * �� �� ֵ�� ��
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

	/* 1. ��ʾҳ�� */
	ShowPlayPage(&g_tPlayPageLayout);

	/* 2. �������ڴ���ͱ��������߳� */
	 pthread_create(&g_tPlayThreadID, NULL, PlayThreadFunction, &tPageParams);
	/* 3. ����GetInputEvent��������¼����������� */
	while (1)
	{	
		iIndex = PlayPageGetInputEvent(&g_tPlayPageLayout, &tInputEvent);
		if (tInputEvent.iPressure == 0)
		{
			/* ������ɿ� */
			if (bPressed)
			{
				/* �����а�ť������ */
				ReleaseButton(&g_atPlayPageIconsLayout[iIndexPressed]);
				bPressed = 0;

				if (iIndexPressed == iIndex) /* ���º��ɿ�����ͬһ����ť */
				{
					switch (iIndexPressed)
					{
						case 1: /* "���ػط�Ŀ¼"��ť */
						{
							   pthread_mutex_lock(&g_tPlayThreadMutex);
						            g_bPlayThreadShouldExit = 1;   /* PlayThreadFunction�̼߳�⵽�������Ϊ1����˳� */
						            pthread_mutex_unlock(&g_tPlayThreadMutex);
						            pthread_join(g_tPlayThreadID, NULL);  /* �ȴ����߳��˳� */
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
 * �������ƣ�PlayPageInit
 * ���������� ע��"Playҳ��"
 * ��������� ��
 * ��������� ��
 * �� �� ֵ�� 0 - �ɹ�, ����ֵ - ʧ��
 ***********************************************************************/
int PlayPageInit(void)
{
	return RegisterPageAction(&g_tPlayPageAction);
}
