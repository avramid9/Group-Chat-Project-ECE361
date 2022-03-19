#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
 #include <unistd.h>
#include <stdbool.h>
 #define STDIN 0 // file descriptor for standard input

 int main(void)
 {
     struct timeval tv;
     fd_set readfds;

     tv.tv_sec = 2;
     tv.tv_usec = 500000;

     FD_ZERO(&readfds);
     char u[100];
     int num=0;
     FD_SET(STDIN, &readfds);
     bool loop=true;
     while(loop){
         printf("first");
         scanf("%s",u);
         printf("before");
         fflush(stdout);
         // don't care about writefds and exceptfds:
         select(STDIN+1, &readfds, NULL, NULL, NULL);
         printf("second");
         fflush(stdout);
         for(int i=0;i<3;i++){
            if (FD_ISSET(STDIN, &readfds)){
                char c[200];
                scanf("%s",c);
                printf("A key was pressed! %d\n",num);
                //FD_ZERO(&readfds);
            }       
            else{
                num++;
            }
            //printf("Timed out.\n");
         }
     }
     
     

     return 0;
 }


