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

typedef struct Messages { //é diferente não?
    int rid;    // request id
    pid_t pid;    // process id
    pthread_t tid;    // thread id
    int tskload;    // task load
    int tskres;    // task result
} Message;

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

int main(int argc, char* argv[], char* envp[]) {
    int nsecs;
    char fifoname[25], buffer[30];
    srand(time(NULL));

    if (argc != 4) {
        perror("Usage: s <-t nsecs> fifoname");
        exit(1);
    }
    nsecs = atoi(argv[2]);
    strcpy(fifoname, argv[3]);  //se n tiver /tmp/ acrescentar
    if (strstr(fifoname, "/tmp/") == NULL) {
        sprintf(buffer, "/tmp/%s", fifoname);
        strcpy(fifoname, buffer);
    }
    time_t start = time(NULL);
    time_t endwait = time(NULL) + nsecs;

    for (int tries = 0; tries < 3; tries++) {
        public_pipe = open(fifoname, O_WRONLY);
        if (public_pipe != -1)
            break;
        sleep(1);
    }
    if (public_pipe == -1) {
        perror("Error opening public FIFO");
        exit(1);
    }
    pthread_t tid;
    Message* msg;
    bool processing = true;
    int reading;

    while (start < endwait) {
        msg = (Message*)malloc(sizeof(Message));
        reading = read(public_pipe, msg, sizeof(Message));

        if (reading > 0) {
            op_print(msg, RECVD);
            //do stuff
        }
        else if (reading == 0) {
            free(msg);
        }
        else {
            perror("Error reading public FIFO");
            free(msg);
        }
    }

    //Close and Delete FIFO
    if (close(public_pipe) == -1) {
        perror("Error closing public FIFO");
        exit(1);
    }

    if (unlink(public_pipe) == -1) {
        perror("Error deleting public FIFO");
        exit(1);
    }

    pthread_exit(NULL);
}