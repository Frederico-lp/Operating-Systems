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
#include <semaphore.h>

#include "lib.h"

typedef struct Messages { 
    int rid;    // request id
    pid_t pid;    // process id
    pthread_t tid;    // thread id
    int tskload;    // task load
    int tskres;    // task result
} Message;

sem_t bufszSem, semFull;
int bufsz = 20;
static int public_pipe;
static int consuming = 1;
Message** buffer;
int lastBufferPos = 0;
pthread_mutex_t buffer_pos;




void op_print(char op[], Message msg) {
    time_t cur_time = time(NULL);
    int i = msg.rid, t = msg.tskload, pid = msg.pid, tid = msg.tid, res = msg.tskres, ret_value;
    //returns the number of bytes that are printed or a negative value if error
    ret_value = fprintf(stdout, "%ld ; %d ; %d ; %d ; %u ; %d ; %s\n", cur_time, i, t, pid, tid, res, op);

    if (ret_value <= 0) {
        perror("Error in fprintf\n");
    }

    fflush(stdout);
}

void* producerThread(void* message) {

    Message* msg = (Message*) message;

    if(sem_wait(&bufszSem) == -1){
        free(msg);
        perror("Error waiting for semaphore\n");
    }

    msg->tskres = task(msg->tskload);

    if(msg->tskres){
        op_print("TSKEX", *msg);
    }

    pthread_mutex_lock(&buffer_pos);
    buffer[lastBufferPos] = msg;
    lastBufferPos++;
    pthread_mutex_unlock(&buffer_pos);

    if(sem_post(&semFull) == -1){
        free(msg);
        perror("Error unlocking semaphore\n");
    }

    //free(msg);  

}

void* consumerThread(){

    char privateFIFO[1000];
    int private_pipe;

    Message* msg;
    //vai ser preciso um private pipe por cada request portanto esta mal aquilo em cima
    //se a funçao devolver a mensagem é facil, e so fazer o sprintf e abrir o pipe em cada 
    //vez que é para enviar a msg
    while(consuming){

        if(sem_wait(&semFull) == -1){
            perror("Empty\n");
        }

        pthread_mutex_lock(&buffer_pos);
        lastBufferPos--;
        msg = buffer[lastBufferPos]; // pos-1 pq pos foi incrementada dps de guardar msg na producer
        pthread_mutex_unlock(&buffer_pos);

        sprintf(privateFIFO, "/tmp/%d.%lu", msg->pid, msg->tid);
        printf("fifo privado é %s\n", privateFIFO);
        op_print("TESTE", *msg);

        if ((private_pipe = open(privateFIFO, O_WRONLY)) == -1) {
            free(msg);
            perror("Error opening private FIFO");
            exit(1);
        }

        int ret_value = write(private_pipe, msg, sizeof(Message));

        if(ret_value>0){
            op_print("TSKDN", *msg);
        }
        else if(ret_value==-1){
            op_print("FAILD", *msg);
        }

        if(sem_post(&bufszSem) == -1){
            perror("Error unlocking semaphore\n");
        }
    }
} 

int main(int argc, char* argv[], char* envp[]) {


    printf("%d", argc);
    printf("AQUI1");
    int nsecs;
    char publicFIFO[25], bfr[30];
    srand(time(NULL));

    
    if (argc != 4 && argc != 6) {
        perror("Usage: s <-t nsecs>  [-l bufsz] fifoname");
        //exit(1);
    }
    
    printf("AQUI2");
    

    nsecs = atoi(argv[2]);
    printf("AQUI3");

    if(argc == 4)
        strcpy(publicFIFO, argv[3]);  
    else if (argc == 6){
        strcpy(publicFIFO, argv[5]); 
        bufsz = atoi(argv[4]);
    }
    printf("AQUI4");
    printf("%s\n", publicFIFO);

    if (strstr(publicFIFO, "/tmp/") == NULL) {
        sprintf(bfr, "/tmp/%s", publicFIFO);
        strcpy(publicFIFO, bfr);
    }


    buffer = malloc(bufsz * sizeof(Message*));
    for(int i = 0; i < bufsz; i++)
        buffer[i] = malloc((sizeof(Message*)) );
        
    
    if(sem_init(&bufszSem, 0, bufsz) == -1){ //n pode ser 0 para n ser partilhado, 1??
        perror("Error creating semaphore\n");
    }    
    if(sem_init(&semFull, 0, 0) == -1){ //0 is the initial value 
        perror("Error creating semaphore\n");
    } 
    pthread_mutex_init(&buffer_pos, NULL);
    

    time_t start = time(NULL);
    time_t endwait = time(NULL) + nsecs;


    unlink(publicFIFO);////////////////////////////// como encravava n estava a apaagr e dava erro a seguir
    if(mkfifo(publicFIFO, 0777) == -1){
        perror("Error creating public FIFO");
        //exit(1);
    }
    printf("%s\n", publicFIFO);

    if((public_pipe = open(publicFIFO, O_RDONLY)) == -1){
        perror("Error opening public FIFO in server");
        //exit(1);
    }
        
    pthread_t tid;
    Message* msg;
    bool processing = true;
    int reading;

    if (pthread_create(&tid, NULL, consumerThread, NULL)) {
        //cria thread produtora
        perror("Error creating thread");
        //exit(1);
    }

    while (start < endwait) {   //tenho de dar free a msg em todos os ciclos?
        msg = (Message*)malloc(sizeof(Message));
        reading = read(public_pipe, msg, sizeof(Message));

        if (reading > 0) {
            op_print("RECVD", *msg);    

            if (pthread_create(&tid, NULL, producerThread, (void*) msg)) {
                //cria thread produtora
                perror("Error creating thread");
                //exit(1);
            }

            pthread_detach(tid);
        }

        else if (reading == 0) {
            free(msg);
        }

        else {
            perror("Error reading public FIFO");
            free(msg);
            break;
        }
        start = time(NULL);
    }

    consuming = 0;

    //Close and Delete FIFO
    if (close(public_pipe) == -1) {
        perror("Error closing public FIFO");
        //exit(1);
    }

    if (unlink(publicFIFO) == -1) {
        perror("Error deleting public FIFO");
        //exit(1);
    }

    sem_destroy(&bufszSem);
    sem_destroy(&semFull);
    pthread_mutex_destroy(&buffer_pos);

    pthread_exit(NULL);
}