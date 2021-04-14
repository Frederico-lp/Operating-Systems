#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <signal.h>
#include <pthread.h>

int public_pipe;

typedef struct Messages {
   int requestNumber;
   int number;
   int pid;
   int tid;
   int clientRes;
} Message;

Message* create_msg(int id){
    Message* msg = (Message*)malloc(sizeof(Message));
    msg->requestNumber = id;
    msg->number = rand() % 9 + 1;
    msg->pid = getpid();
    msg->tid = pthread_self();
    msg->clientRes = -1;
    
    return msg;
}

void *clientThread(void *arg){
    char *privateFIFO;
    int private_pipe;
    sprintf(privateFIFO, "/tmp/%d.%ld", getpid(), pthread_self());
    
    if(mkfifo(privateFIFO, 0666) < 0){// q permissao escolho?
        perror("Error creating private FIFO");
        exit(1);//exit ou return?
    }

    Message *msg = create_msg(*(int*)arg);

    //send request through public pipe
    if(write(public_pipe, msg, sizeof(Message)) == -1){
        //acho q vai faltar o log aqui
        free(msg);
        perror("Error sending request to public FIFO");
        exit(1);//exit ou return?
    }



    //open private pipe
    if ((private_pipe = open(privateFIFO, O_RDONLY)) == -1) {
        free(msg);  //free msg ou tenho de dar free a tudo la dentro antes?
        perror("Error opening private FIFO");
        exit(1);
    }
    
    //read private pipe
    if(read(private_pipe, msg, sizeof(Message) == -1)){
        free(msg);  //free msg ou tenho de dar free a tudo la dentro antes?
        perror("Error reading private FIFO");
        exit(1);
    }

    //fechar pipe
    if (close(private_pipe) == -1) {
        free(msg);
        perror("Error closing private FIFO");
        pthread_exit(NULL);
    }

    //eliminar pipe
    if (unlink(privateFIFO) == -1){
        free(msg);
        perror("Error deleting private FIFO");
        exit(1);
    }

    free(msg);
    pthread_exit(NULL);

}
 

int main(int argc, char* argv[], char* envp[]) {
    int nsecs, timeout, *requestNumber;
    requestNumber = malloc(sizeof(int));
    *requestNumber = 1;
    char* fifoname;
    srand (time(NULL));

    if(argc != 4){
        perror("Usage: c <-t nsecs> fifoname");
        exit(1);//return 1?
    }

    nsecs = atoi(argv[1]);
    strcpy(fifoname, argv[2]);  //se n tiver /tmp/ acrescentar
    if(strstr("/tmp/", fifoname) == NULL)
        sprintf(fifoname, "/tmp/%s", fifoname);
    

    public_pipe = open(fifoname, O_WRONLY);// open to write-only, public_pipe == file descriptor

    pthread_t thread_id;
    int start = time(NULL);
    time_t endwait = time(NULL) + nsecs;
    while(start < endwait){  //tem a ver com o timeout mas n sei o que e suposto fazer
        if(pthread_create(&thread_id, NULL, clientThread, requestNumber)){    //request number
            perror("Error creating thread");
            exit(1);
        }

        //delay(rand() % 10);
        int time_aux = (rand() % 10 + 5) * 0.001;
        sleep(time_aux);
        *requestNumber++;
        start = time(NULL);
    }

    //pthread_join()
    //pthread_exit()
}
