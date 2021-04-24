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

static int public_pipe;
__thread int gaveup = 0;

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

void op_print(char op[], Message msg){
    time_t cur_time=time(NULL);
    int i=msg.requestNumber, t=msg.number, pid=msg.pid, tid=msg.tid, res=msg.clientRes, ret_value;
    //returns the number of bytes that are printed or a negative value if error
    ret_value=fprintf(stdout, "%ld ; %d ; %d ; %d ; %d ; %d ; %s\n", cur_time, i, t, pid, tid, res, op);

    if(ret_value<=0){
        perror("Error in fprintf\n");
    }

    fflush(stdout);
}

void gavup(int sig) {
    gaveup = 1;
}

void *clientThread(void *arg){
    signal(SIGUSR1, gavup);
    char privateFIFO[25];
    int private_pipe;
    sprintf(privateFIFO, "/tmp/%d.%ld", getpid(), pthread_self());
    
    if(mkfifo(privateFIFO, 0666) < 0){// q permissao escolho?
        perror("Error creating private FIFO");
        exit(1);
    }

    Message *msg = create_msg(*(int*)arg);

    free(arg);

    //send request through public pipe
    int ret_value = write(public_pipe, msg, sizeof(Message));

    //write() returns the number of bytes written into the array or -1 if error
    if(ret_value>0){
        op_print("IWANT", *msg);
    }
    else if(ret_value == -1){

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
    int ret_value2 = read(private_pipe, msg, sizeof(Message));
    if(ret_value2 == -1){
        free(msg);  //free msg ou tenho de dar free a tudo la dentro antes?
        perror("Error reading private FIFO");
        exit(1);
    }
    else if(ret_value2>0){

        //-1 se o serviço já está encerrado (pelo que o pedido não foi atendido);
        if(msg->clientRes == -1){
            op_print("CLOSD",*msg); //esta ao contrario?
        }
        else if (gaveup) {
            op_print("GAVUP", *msg);
        }
        else op_print("GOTRS", *msg);
    }
    else op_print("FAILD", *msg);   // fail or close?

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

    int nsecs, *requestNumber;
    requestNumber = malloc(sizeof(int));
    *requestNumber = 1;
    char fifoname[25];
    srand (time(NULL));

    if(argc != 4){
        perror("Usage: c <-t nsecs> fifoname");
        exit(1);//return 1?
    }
    nsecs = atoi(argv[2]);
    strcpy(fifoname, argv[3]);  //se n tiver /tmp/ acrescentar
    if(strstr(fifoname, "/tmp/") == NULL)
        sprintf(fifoname, "/tmp/%s", fifoname);
    
    int tries = 0;
    for(int tries = 0; tries < 3; tries ++){
        public_pipe = open(fifoname, O_WRONLY);
        if(public_pipe != -1)
            break;
        sleep(0.01);
    }
    if(public_pipe == -1){
        perror("Error opening public FIFO");
        free(requestNumber);
        exit(1);
    }
    pthread_t thread_id[9999];
    time_t start = time(NULL);
    time_t endwait = time(NULL) + nsecs;
    int threadnum = 0;
    while(start < endwait){ 
        printf("AQUI\n");
        if(pthread_create(&thread_id[threadnum], NULL, clientThread, requestNumber)){    //request number
            perror("Error creating thread");
            exit(1);
        }
        pthread_detach(thread_id);
        //delay(rand() % 10 + 5);
        int time_aux = (rand() % 10 + 5) * 0.001;
        sleep(time_aux);
        //*requestNumber++;
        start = time(NULL);
        threadnum++;
    }
    
    for (int i = 0; i < threadnum; i++) {
        pthread_kill(thread_id[i], SIGUSR1);
    }
    

    free(requestNumber);
    //pthread_join()
    pthread_exit(NULL);
}