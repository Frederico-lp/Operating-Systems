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

#define BUFFER 1024

static int public_pipe;
static int requestNumber = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int gaveup = 0;

typedef struct Messages {
    int rid;    // request id
    pid_t pid;    // process id
    pthread_t tid;    // thread id
    int tskload;    // task load
    int tskres;    // task result
} Message;

Message* create_msg(){
    Message* msg = (Message*)malloc(sizeof(Message));
    msg->rid = requestNumber;
    msg->tskload = rand() % 9 + 1;
    msg->pid = getpid();
    msg->tid = pthread_self();
    msg->tskres = -1;
    
    return msg;
}

void op_print(char op[], Message msg){
    
    time_t cur_time=time(NULL);
    int i=msg.rid, t=msg.tskload, pid=msg.pid, tid=msg.tid, res=msg.tskres, ret_value;
    //returns the number of bytes that are printed or a negative value if error
    ret_value=fprintf(stdout, "%ld ; %d ; %d ; %d ; %d ; %d ; %s\n", cur_time, i, t, pid, tid, res, op);

    if(ret_value<=0){
        perror("Error in fprintf\n");
    }

    fflush(stdout);
}

void giveUp(Message* msg) {
    
    op_print("GAVUP", *msg);  
}

static void cleanup_unlock_mutex(void *p)
{
    pthread_mutex_unlock(p);
}

void *clientThread(){
    char privateFIFO[1000];
    int private_pipe;
    sprintf(privateFIFO, "/tmp/%d.%ld", getpid(), pthread_self());
    printf("%s\n", privateFIFO);

    if(mkfifo(privateFIFO, 0660) < 0){
        perror("Error creating private FIFO");
        exit(1);
    }

    Message *msg = create_msg();

    pthread_cleanup_push(cleanup_unlock_mutex, &mutex);
    pthread_mutex_lock(&mutex);
    requestNumber++;
    pthread_cleanup_pop(1);


    //send request through public pipe
    int ret_value = write(public_pipe, msg, sizeof(Message));

    //write() returns the number of bytes written into the array or -1 if error
    if(ret_value > 0){
        op_print("IWANT", *msg);
    }
    else if(ret_value < 0){
        //acho q vai faltar o log aqui
        free(msg);
        perror("Error sending request to public FIFO");
        exit(1);
    }


    //open private pipe
    if ((private_pipe = open(privateFIFO, O_RDONLY)) == -1) {
        free(msg);  //free msg ou tenho de dar free a tudo la dentro antes?
        perror("Error opening private FIFO");
        exit(1);
    }

    //read private pipe
    //timeout?
    int ret_value2 = read(private_pipe, msg, sizeof(Message));

    if(ret_value2 < 0){
        free(msg);  //free msg ou tenho de dar free a tudo la dentro antes?
        perror("Error reading private FIFO");
        exit(1);
    }
    else if(ret_value2 > 0){

        //-1 se o serviço já está encerrado (pelo que o pedido não foi atendido);
        if(msg->tskres != -1){
            op_print("GOTRS",*msg); 
        }
        /*
        else if (gaveup){
            op_print("GAVUP", *msg);
        }
        */
        //else op_print("CLOSD", *msg);
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

    int nsecs;
    char fifoname[25], buffer[30];
    srand (time(NULL));

    if(argc != 4){
        perror("Usage: c <-t nsecs> fifoname");
        exit(1);
    }
    nsecs = atoi(argv[2]);
    strcpy(fifoname, argv[3]);  //se n tiver /tmp/ acrescentar
    if(strstr(fifoname, "/tmp/") == NULL){
        sprintf(buffer, "/tmp/%s", fifoname);
        strcpy(fifoname, buffer);
    }
    time_t start = time(NULL);
    time_t endwait = time(NULL) + nsecs;
    
    for(int tries = 0; tries < 3; tries ++){
        public_pipe = open(fifoname, O_WRONLY);
        if(public_pipe != -1)
            break;
        sleep(1);
    }
    if(public_pipe == -1){
        perror("Error opening public FIFO");
        exit(1);
    }
    pthread_t tid;
    
    while(start < endwait){ 
        if(pthread_create(&tid, NULL, clientThread, NULL)){  
            perror("Error creating thread");
            exit(1);
        }
        pthread_detach(tid);

        double time_aux = (rand() % 10 + 5) * 100;
        usleep(time_aux);

        start = time(NULL);

    }
    //giveUp();
    //man pthread_cleanup_push;
    pthread_exit(0);
}