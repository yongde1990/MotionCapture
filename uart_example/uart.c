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
* 功能:串口每隔50ms毫秒将数据读取串口
*               当应用层写数据时立刻将数据从串口发送出去
*              GPG5输出低电平时，串口1开始读取25字节右臂右手数据
*              GPG6输出低电平时，串口2开始读取29字节左臂左手数据
* 传感器数据格式:
* (右臂24字节)+(右手1字节)+(左臂24字节)+(左手5字节)
********************************************************************/

#define DEVICE_NAME_UART1_2  "uart1_2"
/*
* 传感器发送过来总的有用data字节数长度
* RigthDataLength: 为右臂+右手数据长度
* LeftDataLength:为左臂+左手数据长度
*/
#define DataLength 54
#define RigthDataLength 25
#define LeftDataLength 29
/*起始字节(0x55、0xaa)+Data ,串口读入的数据*/
#define NUM_DATA_R 56
/*
* 对读入的数据进行处理,添加了一个字节的data长度
* 0x55+0xaa+0x36+Data(54个字节)
*/
#define NUM_DATA_W 57
//#define NUM_DATA_W 1
static DECLARE_WAIT_QUEUE_HEAD(uart_rwaitq);

/*是否唤醒读数据的标志位*/
static volatile int ev_ruart = 0;

static struct class *uartdrv_class_uart1_2;
static struct class_device*uartdrv_class_dev_uart1_2;

/*uart tmp data*/
static unsigned char tmp_rr[NUM_DATA_R];
static unsigned char tmp_ww[NUM_DATA_W];

static struct timer_list uart_timer;
/*Uart 1 端口的相关寄存器映射*/
volatile unsigned long *UART_ULCON1 = NULL;
volatile unsigned long *UART_UCON1 = NULL;
volatile unsigned long *UART_URXH1 = NULL;
volatile unsigned long *UART_UTXH1 = NULL;
volatile unsigned long *UART_UBRDIV1 = NULL;
volatile unsigned long *UART_UTRSTAT1 = NULL;
volatile unsigned long *UART_UFCON1 = NULL; 
volatile unsigned long *UART_UFSTAT1 = NULL; 

/*Uart 2 端口的相关寄存器映射*/
volatile unsigned long *UART_ULCON2 = NULL;
volatile unsigned long *UART_UCON2 = NULL;
volatile unsigned long *UART_URXH2 = NULL;
volatile unsigned long *UART_UTXH2 = NULL;
volatile unsigned long *UART_UBRDIV2 = NULL;
volatile unsigned long *UART_UTRSTAT2 = NULL;
volatile unsigned long *UART_UFCON2 = NULL; 
volatile unsigned long *UART_UFSTAT2 = NULL; 

/*GPIO 管脚配置寄存器映射*/
volatile unsigned long *GPHCON = NULL; 
volatile unsigned long *gpgcon = NULL;
volatile unsigned long *gpgdat = NULL;

/* 打开驱动inode结构在内部表示文件，file结构是打开上面传来的文件描述符fd对应的file结构,file结构都指向单个inode */
static int myuart_open(struct inode *inode, struct file * file) 
{ 
	*GPHCON &= ~((0x3<<(6*2))|(0x3<<(7*2))|(0x3<<(4*2))|(0x3<<(5*2))) ;
	*GPHCON |= ((0x2<<(6*2))|(0x2<<(7*2))|(0x2<<(4*2))|(0x2<<(5*2)));
	*gpgcon &= ~((0x3<<(5*2) )|(0x3<<(6*2)));
	*gpgcon |= ((0x1<<(5*2))|(0x1<<(6*2)));
	*gpgdat |= ((1<<5)|(1<<6)) ;/*初始化为高电平*/
	/* 设置寄存器 */
	*UART_UCON1 	=  0x05;
	*UART_ULCON1 	= 0x03; 
	*UART_UCON2 	=  0x05;
	*UART_ULCON2 	= 0x03; 
	return 0; 
} 
 

/* 从设备读取数据，上层的read函数会调用到这里，file是read的fd对应的结构, ppos是系统回调 */
static ssize_t myuart_read (struct file *file, char __user *buff, size_t count, loff_t *ppos)
{
	int i; 
	/*休眠，在定时器处理函数里唤醒*/
    	wait_event_interruptible(uart_rwaitq, ev_ruart);
	*gpgdat &= ~(1<<5) ; /*低电平*/
	/*右臂右手*/
	while(!((*UART_UTRSTAT1) & 0x1)); 
		tmp_rr[0] = (*UART_URXH1);
	if((tmp_rr[0]) == 0x55){
		while(!((*UART_UTRSTAT1) & 0x1)); 
			tmp_rr[1] = (*UART_URXH1);
		if((tmp_rr[1]) == 0xaa){
			for(i = 0; i < RigthDataLength; i++) 
			{ 
				while(!((*UART_UTRSTAT1) & 0x1));       /* 扫描寄存器是否有数据 */
			    	tmp_rr [i+2] =(*UART_URXH1);               /*  存放在buff里 */
			}
			}
		}
	*gpgdat |= (1<<5) ; /*高电平*/
	*gpgdat &= ~(1<<6) ; /*低电平*/
	while(!((*UART_UTRSTAT2) & 0x1)); 
		tmp_rr[0] = (*UART_URXH2);
	if((tmp_rr[0]) == 0x55){
		while(!((*UART_UTRSTAT2) & 0x1)); 
			tmp_rr[1] = (*UART_URXH2);
		if((tmp_rr[1]) == 0xaa){
			for(i = 0; i < LeftDataLength; i++) 
			{ 
				while(!((*UART_UTRSTAT2) & 0x1));       /* 扫描寄存器是否有数据 */
			    	tmp_rr[i+2+RigthDataLength] =(*UART_URXH2);/*  存放在buff里 */
			}
			}
		}
	*gpgdat |= (1<<6) ; /*高电平*/
	copy_to_user(buff, tmp_rr, NUM_DATA_R);
	ev_ruart = 0;
    	return strlen(buff);   /* 返回buff的长度 */
} 

 /* 向设备写入数据，上层的write函数会调用到这里，file是write的fd对应的结构, offp是系统回调 */
static ssize_t myuart_write (struct file *file, const char __user *buff, size_t count, loff_t *offp)
{ 
	int i; 
	char ch;  
	
	copy_from_user(tmp_ww, buff, NUM_DATA_W);
	for(i = 0; i < count; i++) 
	{ 
	    ch = tmp_ww[i];  
	    while(!(((*UART_UTRSTAT2) & 0x2) == 0x2));            /* 扫描寄存器是否有数据 */
	    *UART_UTXH2 = ch;                                           /* 把一格字节写入寄存器 */
	}
	return 0; 
} 
 /*  设置串口的波特率 */
 static long myuart_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    int ret; 
    ret = -EINVAL; 
      switch(cmd)  
    { 
        case 111:      /* 功过命令来配置波特率 115200*/
        { 
            if(arg == 115200) 
            { 
            	 *UART_UBRDIV1 = 26;
                *UART_UBRDIV2 = 26; 
                ret = 0; 
            } 
        };
	break; 
 	case 960:      /* 功过命令来配置波特率 9600*/
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
	/*关定时器*/
    return 0; 
} 
 /* file_operations结构是建立驱动程序和设备编号的连接，内部是一组函数指针，每个打开的文件，也就是file结构，和一组函数关联，这些操作主要用来实现系统调用的 */
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
	ev_ruart = 1;                  /* 表示50ms 发生了 */
    	wake_up_interruptible(&uart_rwaitq);   /* 唤醒休眠的进程 */
}

static int major_uart1_2; 
 /* 初始化驱动模块,驱动程序要注册一个设备文件，并对其进行操作 */
static int my_uart_init(void) 
{     
	init_timer(&uart_timer);
	uart_timer.expires = jiffies+HZ;
	uart_timer.function = uart_timer_function;
	add_timer(&uart_timer);
	 /* 注册一个字符设备，并分配设备编号，major是设备号，name是驱动程序名称 fops是file_operations结构 */
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

      return 0;        /* 注册成功返回0 */
} 
 /*  卸载模块 */
static void  my_uart_exit(void) 
{
	  class_device_unregister(uartdrv_class_dev_uart1_2);
	  class_destroy(uartdrv_class_uart1_2);
      /* 如果不使用该设备时释放编号 */
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

