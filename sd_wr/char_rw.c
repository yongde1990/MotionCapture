#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h> 

#define BUFFER_SIZE 3

int main(int argc,char **argv) 
{ 
int fd_hex,fd_char; 
int bytes_write; 
char buffer_hex[BUFFER_SIZE] = {0x11,0x22,0x33}; 
char buffer_char[BUFFER_SIZE] = {'1','2','3'};
if(argc!=3) 
{ 
	fprintf(stderr,"Usage:%s fromfile tofile/n/a",argv[0]); 
	exit(1); 
} 
/* 打开源文件 */ 
if((fd_hex=open(argv[1],O_RDONLY|O_WRONLY))==-1) 
{ 
	fprintf(stderr,"Open %s Error:%s/n",argv[1],strerror(errno)); 
	exit(1); 
} 
if((fd_char=open(argv[2],O_RDONLY|O_WRONLY))==-1) 
{ 
	fprintf(stderr,"Open %s Error:%s/n",argv[1],strerror(errno)); 
	exit(1); 
} 
bytes_write=write(fd_hex,buffer_hex,BUFFER_SIZE);
bytes_write=write(fd_char,buffer_char,BUFFER_SIZE);

close(fd_hex); 
close(fd_char);
exit(0); 
} 

