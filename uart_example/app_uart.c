#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>

/********************************************************************
* GPG5����͵�ƽʱ������1��ʼ��ȡ25�ֽ��ұ���������
* GPG6����͵�ƽʱ������2��ʼ��ȡ29�ֽ������������
* ���������ݸ�ʽ:
* (�ұ�24�ֽ�)+(����1�ֽ�)+(���24�ֽ�)+(����5�ֽ�)
********************************************************************/

#define BUFSIZE_UART_R 56
#define BUFSIZE_UART_W 57
/*���õ�data����*/
#define UART_R_Length 54
/*115200������*/
#define BOUNDRATE 111
#if 0
#define BOUNDRATE 960
#endif
int main(void)
{
	char i;
	int fd;
	int ret;  
	char tmp_r[BUFSIZE_UART_R];  
	char tmp_w[BUFSIZE_UART_W]; 
	unsigned char FlagData = 0;
	fd = open("/dev/uart1_2", O_RDWR);  
	if(fd < 0)  
	{  
	    printf("Error: open /dev/uart1_2 error!/n");  
	    return 1;  
	} 
	
	ret = ioctl(fd, BOUNDRATE, 115200);
#if 0
	ret = ioctl(fd, BOUNDRATE, 9600);     
#endif
	memset(tmp_r, 0, sizeof(tmp_r));
	memset(tmp_w, 0, sizeof(tmp_w));
	while(1){

		ret = read(fd, tmp_r, BUFSIZE_UART_R); 
		
		tmp_w[0] = tmp_r[0];
		tmp_w[1] = tmp_r[1];
		tmp_w[2] = 0x36;  /*data���ݳ���*/
		
		for(i = 0; i< UART_R_Length ; i++){
			tmp_w[i+3] = tmp_r[i+2];
			//printf("i = %d ,data = %0x\n",i,tmp_w[i+3]);
		}

		for(i = 3; i< UART_R_Length ; i++){
			if((tmp_w[i] == 0x55) ||(tmp_w[i] == 0xaa))
				FlagData = 1;
		}

		if(!(FlagData == 1))
			ret = write(fd, tmp_w, BUFSIZE_UART_W); 
		FlagData = 0;
	}
	close(fd); 
	return 0;
}  
