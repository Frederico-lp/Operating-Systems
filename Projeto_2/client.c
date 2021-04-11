#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <signal.h>
#include <pthread.h>

typedef struct Messages {
   int requestNumber;
   int number;
   int pid;
   int tid;
   int serverRes;
   int clientRes;
} Message;

void *clientThread(void *arg){

}
 

int main(int argc, char* argv[], char* envp[]) {
    int nsecs;
    char* fifoname;
    srand (time(NULL));

    if(argc != 4){
        perror("Usage: c <-t nsecs> fifoname");
        exit(1);//return 1?
    }

    nsecs = argv[1];
    strcpy(fifoname, argv[2]);

    pthread_t thread_id;
    while(??){
        if(pthread_create(&thread_id, NULL, clientThread, NULL))
            perror("Error creating thread");

    }
    //pthread_join()


    

}