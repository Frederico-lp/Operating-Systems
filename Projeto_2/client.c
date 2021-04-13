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

int fd;

typedef struct Messages {
   int requestNumber;
   int number;
   int pid;
   int tid;
   int serverRes;
   int clientRes;
} Message;

void *clientThread(void *arg){
    char *privateFIFO;
    sprintf(privateFIFO, "/tmp/%d.%d", getpid(), gettid());
    
    if(mkfifo(privateFIFO, 0777) < 0){// q permissao escolho?
        perror("Error creating private FIFO");
        exit(1);//exit ou return?
    }
    //mandar pedido pelo public pipe
   //
    //abrir pipe
    if ((private_pipe = open(privateFIFO, O_RDONLY)) == -1) {
        perror("Error opening private FIFO");
        exit(1);
    }
    //escrever ou ler(uma delas)
   //
    //fechar pipe
    if (close(private_pipe) == -1) {
        perror("Error closing private FIFO");
        pthread_exit(NULL);
    }
    //eliminar pipe
    if (unlink(privateFIFO) == -1){
        perror("Error deleting private FIFO");
        exit(1);
    }

    pthread_exit(NULL);

}
 

int main(int argc, char* argv[], char* envp[]) {
    int nsecs, timeout;
    char* fifoname;
    srand (time(NULL));

    if(argc != 4){
        perror("Usage: c <-t nsecs> fifoname");
        exit(1);//return 1?
    }

    nsecs = argv[1];
    strcpy(fifoname, argv[2]);  //se n tiver /tmp/ acho q tenho de acrescentar
    //quem cria o canal publico? server ou client??
    //mkfifo(fifoname, )
    fd = open(fifoname, O_WRONLY);// open to write-only, fd == file descriptor

    pthread_t thread_id;
    while(timeout){  //tem a ver com o timeout mas n sei o que e suposto fazer
        if(pthread_create(&thread_id, NULL, clientThread, NULL))
            perror("Error creating thread");

        delay();

    }
    //pthread_join()
    //pthread_exit()


    

}
