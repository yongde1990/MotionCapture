#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h> 
#include <linux/sched.h> 
#include <linux/miscdevice.h> 
#include <linux/delay.h> 
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <linux/interrupt.h>
#include <linux/device.h>

/********************************************************************
* ����:����ÿ��50ms���뽫���ݶ�ȡ����
*               ��Ӧ�ò�д����ʱ���̽����ݴӴ��ڷ��ͳ�ȥ
*              GPG5����͵�ƽʱ������1��ʼ��ȡ25�ֽ��ұ���������
*              GPG6����͵�ƽʱ������2��ʼ��ȡ29�ֽ������������
* ���������ݸ�ʽ:
* (�ұ�24�ֽ�)+(����1�ֽ�)+(���24�ֽ�)+(����5�ֽ�)
********************************************************************/

#define DEVICE_NAME_UART1_2  "uart1_2"
/*
* ���������͹����ܵ�����data�ֽ�������
* RigthDataLength: Ϊ�ұ�+�������ݳ���
* LeftDataLength:Ϊ���+�������ݳ���
*/
#define DataLength 54
#define RigthDataLength 25
#define LeftDataLength 29
/*��ʼ�ֽ�(0x55��0xaa)+Data ,���ڶ��������*/
#define NUM_DATA_R 56
/*
* �Զ�������ݽ��д���,�����һ���ֽڵ�data����
* 0x55+0xaa+0x36+Data(54���ֽ�)
*/
#define NUM_DATA_W 57
//#define NUM_DATA_W 1
static DECLARE_WAIT_QUEUE_HEAD(uart_rwaitq);

/*�Ƿ��Ѷ����ݵı�־λ*/
static volatile int ev_ruart = 0;

static struct class *uartdrv_class_uart1_2;
static struct class_device*uartdrv_class_dev_uart1_2;

/*uart tmp data*/
static unsigned char tmp_rr[NUM_DATA_R];
static unsigned char tmp_ww[NUM_DATA_W];

static struct timer_list uart_timer;
/*Uart 1 �˿ڵ���ؼĴ���ӳ��*/
volatile unsigned long *UART_ULCON1 = NULL;
volatile unsigned long *UART_UCON1 = NULL;
volatile unsigned long *UART_URXH1 = NULL;
volatile unsigned long *UART_UTXH1 = NULL;
volatile unsigned long *UART_UBRDIV1 = NULL;
volatile unsigned long *UART_UTRSTAT1 = NULL;
volatile unsigned long *UART_UFCON1 = NULL; 
volatile unsigned long *UART_UFSTAT1 = NULL; 

/*Uart 2 �˿ڵ���ؼĴ���ӳ��*/
volatile unsigned long *UART_ULCON2 = NULL;
volatile unsigned long *UART_UCON2 = NULL;
volatile unsigned long *UART_URXH2 = NULL;
volatile unsigned long *UART_UTXH2 = NULL;
volatile unsigned long *UART_UBRDIV2 = NULL;
volatile unsigned long *UART_UTRSTAT2 = NULL;
volatile unsigned long *UART_UFCON2 = NULL; 
volatile unsigned long *UART_UFSTAT2 = NULL; 

/*GPIO �ܽ����üĴ���ӳ��*/
volatile unsigned long *GPHCON = NULL; 
volatile unsigned long *gpgcon = NULL;
volatile unsigned long *gpgdat = NULL;

/* ������inode�ṹ���ڲ���ʾ�ļ���file�ṹ�Ǵ����洫�����ļ�������fd��Ӧ��file�ṹ,file�ṹ��ָ�򵥸�inode */
static int myuart_open(struct inode *inode, struct file * file) 
{ 
	*GPHCON &= ~((0x3<<(6*2))|(0x3<<(7*2))|(0x3<<(4*2))|(0x3<<(5*2))) ;
	*GPHCON |= ((0x2<<(6*2))|(0x2<<(7*2))|(0x2<<(4*2))|(0x2<<(5*2)));
	*gpgcon &= ~((0x3<<(5*2) )|(0x3<<(6*2)));
	*gpgcon |= ((0x1<<(5*2))|(0x1<<(6*2)));
	*gpgdat |= ((1<<5)|(1<<6)) ;/*��ʼ��Ϊ�ߵ�ƽ*/
	/* ���üĴ��� */
	*UART_UCON1 	=  0x05;
	*UART_ULCON1 	= 0x03; 
	*UART_UCON2 	=  0x05;
	*UART_ULCON2 	= 0x03; 
	return 0; 
} 
 

/* ���豸��ȡ���ݣ��ϲ��read��������õ����file��read��fd��Ӧ�Ľṹ, ppos��ϵͳ�ص� */
static ssize_t myuart_read (struct file *file, char __user *buff, size_t count, loff_t *ppos)
{
	int i; 
	/*���ߣ��ڶ�ʱ���������﻽��*/
    	wait_event_interruptible(uart_rwaitq, ev_ruart);
	*gpgdat &= ~(1<<5) ; /*�͵�ƽ*/
	/*�ұ�����*/
	while(!((*UART_UTRSTAT1) & 0x1)); 
		tmp_rr[0] = (*UART_URXH1);
	if((tmp_rr[0]) == 0x55){
		while(!((*UART_UTRSTAT1) & 0x1)); 
			tmp_rr[1] = (*UART_URXH1);
		if((tmp_rr[1]) == 0xaa){
			for(i = 0; i < RigthDataLength; i++) 
			{ 
				while(!((*UART_UTRSTAT1) & 0x1));       /* ɨ��Ĵ����Ƿ������� */
			    	tmp_rr [i+2] =(*UART_URXH1);               /*  �����buff�� */
			}
			}
		}
	*gpgdat |= (1<<5) ; /*�ߵ�ƽ*/
	*gpgdat &= ~(1<<6) ; /*�͵�ƽ*/
	while(!((*UART_UTRSTAT2) & 0x1)); 
		tmp_rr[0] = (*UART_URXH2);
	if((tmp_rr[0]) == 0x55){
		while(!((*UART_UTRSTAT2) & 0x1)); 
			tmp_rr[1] = (*UART_URXH2);
		if((tmp_rr[1]) == 0xaa){
			for(i = 0; i < LeftDataLength; i++) 
			{ 
				while(!((*UART_UTRSTAT2) & 0x1));       /* ɨ��Ĵ����Ƿ������� */
			    	tmp_rr[i+2+RigthDataLength] =(*UART_URXH2);/*  �����buff�� */
			}
			}
		}
	*gpgdat |= (1<<6) ; /*�ߵ�ƽ*/
	copy_to_user(buff, tmp_rr, NUM_DATA_R);
	ev_ruart = 0;
    	return strlen(buff);   /* ����buff�ĳ��� */
} 

 /* ���豸д�����ݣ��ϲ��write��������õ����file��write��fd��Ӧ�Ľṹ, offp��ϵͳ�ص� */
static ssize_t myuart_write (struct file *file, const char __user *buff, size_t count, loff_t *offp)
{ 
	int i; 
	char ch;  
	
	copy_from_user(tmp_ww, buff, NUM_DATA_W);
	for(i = 0; i < count; i++) 
	{ 
	    ch = tmp_ww[i];  
	    while(!(((*UART_UTRSTAT2) & 0x2) == 0x2));            /* ɨ��Ĵ����Ƿ������� */
	    *UART_UTXH2 = ch;                                           /* ��һ���ֽ�д��Ĵ��� */
	}
	return 0; 
} 
 /*  ���ô��ڵĲ����� */
 static long myuart_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    int ret; 
    ret = -EINVAL; 
      switch(cmd)  
    { 
        case 111:      /* �������������ò����� 115200*/
        { 
            if(arg == 115200) 
            { 
            	 *UART_UBRDIV1 = 26;
                *UART_UBRDIV2 = 26; 
                ret = 0; 
            } 
        };
	break; 
 	case 960:      /* �������������ò����� 9600*/
        { 
            if(arg == 9600) 
            { 
            	 *UART_UBRDIV1 = 325; 
                *UART_UBRDIV2 = 325; 
                ret = 0; 
            } 
        };
	break; 
        default: 
        return -EINVAL; 
      } 
	return ret;
} 
 
static int myuart_close(struct inode *inode, struct file *filp) 
{ 
	/*�ض�ʱ��*/
    return 0; 
} 
 /* file_operations�ṹ�ǽ�������������豸��ŵ����ӣ��ڲ���һ�麯��ָ�룬ÿ���򿪵��ļ���Ҳ����file�ṹ����һ�麯����������Щ������Ҫ����ʵ��ϵͳ���õ� */
static struct file_operations uart_fops = { 
 
      .owner 		=  	THIS_MODULE, 
      .open 		=  	myuart_open, 
      .unlocked_ioctl =  	myuart_ioctl, 
      .read			=    	myuart_read, 
      .write			=   	myuart_write, 
      .release		= 	myuart_close ,
};

static void uart_timer_function(unsigned long data){
	mod_timer(&uart_timer, jiffies+HZ/20);
	ev_ruart = 1;                  /* ��ʾ50ms ������ */
    	wake_up_interruptible(&uart_rwaitq);   /* �������ߵĽ��� */
}

static int major_uart1_2; 
 /* ��ʼ������ģ��,��������Ҫע��һ���豸�ļ�����������в��� */
static int my_uart_init(void) 
{     
	init_timer(&uart_timer);
	uart_timer.expires = jiffies+HZ;
	uart_timer.function = uart_timer_function;
	add_timer(&uart_timer);
	 /* ע��һ���ַ��豸���������豸��ţ�major���豸�ţ�name�������������� fops��file_operations�ṹ */
      	major_uart1_2 = register_chrdev(0, DEVICE_NAME_UART1_2, &uart_fops); 
	if (major_uart1_2 < 0)  { 
	        printk(" can't register major number\n"); 
	        return major_uart1_2; 
	 }
 	uartdrv_class_uart1_2 = class_create(THIS_MODULE, "UART1_2");
	uartdrv_class_dev_uart1_2 = class_device_create(uartdrv_class_uart1_2, NULL, MKDEV(major_uart1_2, 0), NULL, DEVICE_NAME_UART1_2); /* /dev/uart1_2 */

	UART_ULCON1 	= (volatile unsigned long *)ioremap(0x50004000, 40);
	UART_UCON1  	= (volatile unsigned long *)ioremap(0x50004004, 40);
	UART_UFCON1  	= (volatile unsigned long *)ioremap(0x50004008, 40);
	UART_UTRSTAT1 = (volatile unsigned long *)ioremap(0x50004010, 40); 
	UART_UFSTAT1   = (volatile unsigned long *)ioremap(0x50004018, 40);
	UART_URXH1  	= (volatile unsigned long *)ioremap(0x50004024, 40);
	UART_UTXH1 	= (volatile unsigned long *)ioremap(0x50004020, 40);
	UART_UBRDIV1 	= (volatile unsigned long *)ioremap(0x50004028, 40);
	
	UART_ULCON2 	= (volatile unsigned long *)ioremap(0x50008000, 40);
	UART_UCON2  	= (volatile unsigned long *)ioremap(0x50008004, 40);
	UART_UTRSTAT2 = (volatile unsigned long *)ioremap(0x50008010, 40); 	
	UART_URXH2  	= (volatile unsigned long *)ioremap(0x50008024, 40);
	UART_UTXH2 	= (volatile unsigned long *)ioremap(0x50008020, 40);
	UART_UBRDIV2 	= (volatile unsigned long *)ioremap(0x50008028, 40);
	
	GPHCON 		= (volatile unsigned long *)ioremap(0x56000070, 40);
	gpgcon 			= (volatile unsigned long *)ioremap(0x56000060, 16);
	gpgdat 			= gpgcon + 1;

      return 0;        /* ע��ɹ�����0 */
} 
 /*  ж��ģ�� */
static void  my_uart_exit(void) 
{
	  class_device_unregister(uartdrv_class_dev_uart1_2);
	  class_destroy(uartdrv_class_uart1_2);
      /* �����ʹ�ø��豸ʱ�ͷű�� */
          unregister_chrdev(major_uart1_2, DEVICE_NAME_UART1_2); 
	   iounmap(gpgcon);
	   iounmap(GPHCON);
	   iounmap(UART_ULCON1);
	   iounmap(UART_UFCON1);
	   iounmap(UART_UFSTAT1);
	   iounmap(UART_UCON1);
	   iounmap(UART_URXH1);
	   iounmap(UART_UTXH1);
	   iounmap(UART_UBRDIV1);
	   iounmap(UART_UTRSTAT1);
	   
	   iounmap(UART_ULCON2);
	   iounmap(UART_UCON2);
	   iounmap(UART_URXH2);
	   iounmap(UART_UTXH2);
	   iounmap(UART_UBRDIV2);
	   iounmap(UART_UTRSTAT2);
	   
	   del_timer(&uart_timer); 
} 
module_init(my_uart_init); 
module_exit(my_uart_exit); 
MODULE_LICENSE("GPL");

