#include <time.h>
#include <stdio.h>

#include <stdlib.h> 

#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <errno.h> 

#include <sys/mman.h>
#include <sys/stat.h>

#define BUFSIZE_UART_W 57
#define DEFAULT_DIR "/mnt/data"
int main(void){
	struct tm *local;
	time_t t;
	int fd_hex;
	 char strTmp[20];
	 char bytes_write;
	 char buffer_hex[BUFSIZE_UART_W];
	 char buffer_hex_1[BUFSIZE_UART_W];
	unsigned char i = 0;
	unsigned char	j = 0;
	unsigned char ucCount = 0;
	unsigned int IntegerData; 
	struct stat tStat;
	unsigned char * ptFileMap = NULL;
	unsigned char * pcTmpFile = NULL;


	
	buffer_hex[0] = 0x51;
	buffer_hex[1] = 0xaa;
	for(i = 0; i < (BUFSIZE_UART_W - 2); i++)
	 	buffer_hex[i + 2] = 0x31;
	buffer_hex_1[0] = 0x55;
	buffer_hex_1[1] = 0xaa;
	for(i = 0; i < (BUFSIZE_UART_W - 2); i++)
	 	buffer_hex_1[i + 2] = 0x32;
	/* 获取日历时间 */
	t=time(NULL);


	local=localtime(&t);

		printf("Local min is: %d\n",local->tm_min);
		printf("Local sec is: %d\n",local->tm_sec);

	snprintf(strTmp, 20, "%s/%d_%d", DEFAULT_DIR,local->tm_min, local->tm_sec);
		strTmp[19] = '\0';
	printf("Local file name is: %s\n",strTmp);
#if 0
	if(creat(strTmp,0777)<0){ 
	    printf("create file %s failure!\n",strTmp); 
	    exit(EXIT_FAILURE); 
	}else{ 
	    printf("create file %s success!\n",strTmp); 
	}  
		if((fd_hex=open(strTmp,O_RDWR))==-1) 
	{ 
		fprintf(stderr,"Open %s Error:%s/n",strTmp,strerror(errno)); 
		exit(1); 
	} 
	printf("open %s ok\n",strTmp);
	bytes_write=write(fd_hex,buffer_hex,BUFSIZE_UART_W);
	bytes_write=write(fd_hex,buffer_hex_1,BUFSIZE_UART_W);
#endif
	if((fd_hex=open("/mnt/data/4_32",O_RDWR))==-1) 
	{ 
		fprintf(stderr,"Open %s Error:%s/n",strTmp,strerror(errno)); 
		exit(1); 
	} 
	fstat(fd_hex, &tStat);
	printf("file size %d \n",tStat.st_size);
	ptFileMap = (unsigned char *)mmap(NULL , tStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_hex, 0);
	if (ptFileMap == (unsigned char *)-1)
	{
		printf("faile mmap\n");
		return -1;
	}
	printf("ok\n");
	

	pcTmpFile = ptFileMap;
	i = 1;
	while(!((*ptFileMap) == 0x55))
		{
			ucCount++;
			ptFileMap++;
		}
	if(*(++ptFileMap) == 0xaa){
		ptFileMap = pcTmpFile + ucCount;
		IntegerData = (tStat.st_size - ucCount)/BUFSIZE_UART_W;
		for(j = 0; j < IntegerData; j++){
			for(i = 0; i < BUFSIZE_UART_W; i++)	
			printf("j = %d, i = %d, char = %0x\n",j,(i+1) ,ptFileMap[(j*57) + i]);
			}
		}
	
#if 0
	i = 0;
	while(*ptFileMap)
		{
		i++;
		printf("i = %d, char = %0x\n",i ,*ptFileMap);
		ptFileMap++;
	}
#endif

	ptFileMap = pcTmpFile;
	munmap(ptFileMap,tStat.st_size);
	close(fd_hex);
	return 0;
}

