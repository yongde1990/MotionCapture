/* 
Filename    : timer.cpp 
 
Compiler    : gcc 4.1.0 on Fedora Core 5 
 
Description : setitimer() set the interval to run function 
 
Synopsis    : #include <sys/time.h> 
 
              int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue); 
 
              struct itimerval { 
 
                struct timerval it_interval; 
 
                struct timerval it_value; 
 
              }; 
 
              struct timeval { 
 
                long tv_sec; 
 
                long tv_usec; 
              }             
*/  
 
#include <stdio.h>    
#include <unistd.h>  
#include <signal.h> 
#include <string.h> 
#include <sys/time.h>

void printMsg(int);  
int main() {   
 
  int res = 0;  
  signal(SIGALRM, printMsg);  

  struct itimerval tick;  

  memset(&tick, 0, sizeof(tick));  

  tick.it_value.tv_sec = 0;  // sec  
  
  tick.it_value.tv_usec = 50000; // micro sec.  
  
  // Interval time to run function  
  
  tick.it_interval.tv_sec = 0;  
  
  tick.it_interval.tv_usec = 50000;  
  
  // Set timer, ITIMER_REAL : real-time to decrease timer,  
  
  //                          send SIGALRM when timeout  
  
  res = setitimer(ITIMER_REAL, &tick, NULL);   
  if (res) {  
  
    printf("Set timer failed!!/n");  
  
  }  

  // Always sleep to catch SIGALRM signal  
  
  while(1) {  
  
    pause();  
  
  }  
  
  
  
  return 0;    
  
}  
  
  
  
void printMsg(int num) {  
  
  printf("%s","Hello World!!\n");  
  
}  
