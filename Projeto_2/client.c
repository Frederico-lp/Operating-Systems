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

int fd;

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

    //mandar pedido pelo public pipe

   //escreve

    //abrir pipe
    if ((private_pipe = open(privateFIFO, O_RDONLY)) == -1) {
        perror("Error opening private FIFO");
        free(msg);  //free msg ou tenho de dar free a tudo la dentro antes?
        exit(1);
    }
    
    //ler pipe
   //


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
    

    fd = open(fifoname, O_WRONLY);// open to write-only, fd == file descriptor

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
