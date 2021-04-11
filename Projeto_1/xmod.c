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

#define ERROR 100  //error id

static int nftot=0;
int nfmod=0;
char name[100];

struct timeval startTime; //inicio do prog
FILE* logFile; //log location
char* logPath; //logpath location
bool logging;
bool vFlag = false, cFlag = false, rFlag = false;
bool waiting;


void print_usage(){
    printf("Usage: [executable] [flag1] [flag2] [flag3] [mode] [filename]\n\n");
    printf("-> [executable] - name of the executable file\n");
    printf("-> [flag1] [flag2] [flag3] - flags -v -c or -R (can be ommited)\n");
    printf("-> [mode] - permissions to be set (in octal (0755) or manual ([u|g|o|a][+|-|=][rwx]))\n");
    printf("-> [filename] - name of the file/directoy to be changed\n\n");
}


float timeDiff(struct timeval start, struct timeval stop) {
    double secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
    return secs * 1000;
}

void logExit(int ret) {
    if (logging) {
        struct timeval curtime;
        gettimeofday(&curtime, NULL);
        fprintf(logFile, "%f ; %d ; PROC_EXIT ; %d\n", timeDiff(startTime, curtime), getpid(), ret);
    }
    exit(ret);
}

void chmodWrapper(char* arg, mode_t mode) {
    sleep(1);
    struct stat buf;
    stat(arg, &buf);
    int statchmod = buf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    nftot++;
    if((buf.st_mode&0777) != (mode&0777)){
        chmod(arg, mode);
        nfmod++;
        if(cFlag || vFlag){
            printf("mode of %s changed from 0%o to 0%o\n", arg, buf.st_mode & 0777, mode & 0777);

        }
        if (logging) {
            struct timeval curtime;
            gettimeofday(&curtime, NULL);
            stat(arg, &buf);
            int statchmod2 = buf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
            fprintf(logFile, "%f ; %d ; FILE_MODF ; %s : %o : %o\n", timeDiff(startTime, curtime), getpid(), arg, statchmod, statchmod2);
        }
    }
    else if(vFlag)
        printf("mode of %s retained as 0%o\n", arg, mode & 0777);
    
}

void chmodFolder(int flags, mode_t mode, int argc, char* argv[], char* envp[]){
  DIR *dir = opendir(argv[2+flags]);
  struct dirent *entry;
  char newArg[512];
  char* arg = argv[2 + flags];

  if(dir == NULL){
    perror("opendir()");
    logExit(-1);
  }
  while((entry = readdir(dir)) != NULL){
    if(entry->d_type != DT_DIR){
      sprintf(newArg, "%s/%s", arg, entry->d_name);
      chmodWrapper(newArg, mode);
    }
    
    else if(entry->d_type == DT_DIR && rFlag){
      if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){

        sprintf(newArg, "%s/%s", arg, entry->d_name);
        char* newArgv[argc];
        for (int i = 0; i < argc; i++) {
            newArgv[i] = argv[i];
        }
        newArgv[2 + flags] = newArg;
        newArgv[3 + flags] = NULL;
        setpgid(0,0);
        pid_t id = fork();

        //printf("I am the process: %d\n", id);
        switch(id){
            case -1:
                perror("fork()");
                logExit(-1);

                break;
            case 0:
                /* If the process is the child process, then it should enter a new directory */
                if (execve(argv[0], newArgv, envp) == -1) {
                    perror("execve()");
                }
                /* Separation between blocks of information */
                if(cFlag || vFlag)  
                    printf("\n");

                logExit(0);
                break;

            default:
                /* If the process is the parent process, it will wait for the child processes to finish */
                wait(NULL);                 
                break;

        }
      }
    } 
  }

  closedir(dir);
  
}

void signal_handler(int sig) {
    //printf("%d\n", sig);

    ///*argv[2+flags],*/
    printf("pid: %d, fich/dir: %s, nftot: %d, nfmod: %d\n", getpid(), name, nftot, nfmod);
    
    if (logging) {
        struct timeval curtime;
        gettimeofday(&curtime, NULL);
        fprintf(logFile, "%f ; %d ; SIGNAL_RECV ; %d\n", timeDiff(startTime, curtime), getpid(), sig);
    }
    int aux=getpgrp();
    if(getpid() == aux){
        while(1){
            printf("Do you want to exit? [Y/N]\n");
            char answer;
            scanf("%c",&answer);
            if(answer=='Y' || answer=='y'){
                killpg(aux, SIGUSR1);
                if (logging) {
                    struct timeval curtime;
                    gettimeofday(&curtime, NULL);
                    fprintf(logFile, "%f ; %d ; SIGNAL_SENT ; %d : %d\n", timeDiff(startTime, curtime), getpid(), SIGUSR1, aux);
                }
                logExit(1);
            }
            else if (answer == 'N' || answer == 'n') {
                killpg(aux, SIGUSR2);
                if (logging) {
                    struct timeval curtime;
                    gettimeofday(&curtime, NULL);
                    fprintf(logFile, "%f ; %d ; SIGNAL_SENT ; %d : %d\n", timeDiff(startTime, curtime), getpid(), SIGUSR2, aux);
                }
                return;
            }
        }
    }
    else{
        waiting = true;
        while (waiting) {

        }
    }
}

void go_signal(int sig) {
    if (logging) {
        struct timeval curtime;
        gettimeofday(&curtime, NULL);
        fprintf(logFile, "%f ; %d ; SIGNAL_RECV ; %d\n", timeDiff(startTime, curtime), getpid(), sig);
    }
    waiting = false;
}

void quit_signal(int sig) {
    if (logging) {
        struct timeval curtime;
        gettimeofday(&curtime, NULL);
        fprintf(logFile, "%f ; %d ; SIGNAL_RECV ; %d\n", timeDiff(startTime, curtime), getpid(), sig);
    }
    logExit(1);
}

//NOTA: se retirar alguma coisa a um deles, ele esta a adicionar
//para ja vou ignorar as flags
int main(int argc, char* argv[], char* envp[]) {
    int flags = 0;
    signal(SIGINT, signal_handler);
    signal(SIGUSR1, quit_signal);
    signal(SIGUSR2, go_signal);
    waiting = false;
    if(argc < 3){
        print_usage();
        logExit(-1);
    }

    else if(argc > 3){
        for(int i = 0; i < argc - 3; i++){
            if(strcmp(argv[i+1], "-v") == 0 || strcmp(argv[i + 1], "-V") == 0){
                vFlag = true;
                flags++;
            }
            else if(strcmp(argv[i+1], "-c") == 0 || strcmp(argv[i + 1], "-C") == 0){
                cFlag = true;
                flags++;
            }
            else if(strcmp(argv[i+1], "-r") == 0 || strcmp(argv[i + 1], "-R") == 0){
                rFlag = true;
                flags++;
            }
        }
    }
    strcpy(name, argv[2 + flags]);
    logPath = getenv("LOG_FILENAME");
    if (logPath == NULL) {
        logging = false;
    }
    
    else {
        logging = true;
        logFile = fopen(logPath, "a");
        if (!logFile) {
            perror("fopen()");
        }
        gettimeofday(&startTime, NULL);
        fprintf(logFile, "%f ; %d ; PROC_CREATE ; ", 0.0f, getpid());
        int i = 0;
        while (argv[i] != NULL) {
            fprintf(logFile, "%s ", argv[i]);
            i++;
        }
        fprintf(logFile, "\n");
        //process creation log
    }
   
    char user, op, perm[3], opFlag;    //user, operation,permission
    int id;    //id is to identify what we will do
    int j = 2;
    bool allFlag = false;
    mode_t mode = 0, temp = 0;
    struct stat startMode;

    if (argv[1 + flags] == NULL) {
        logExit(1);
    }


    //printf(" %c\n", argv[1][0]);
    if(argv[1 + flags][0] == '0'){    //octal mode
        for(int l = 1; l < 4; l++){ 
            if(l == 1){
                switch (argv[1+flags][1]){
                case '7':
                    mode = S_IRWXU;
                    break;
                case '6':
                    mode = S_IRUSR | S_IWUSR;
                    break;
                case '5':
                    mode = S_IRUSR | S_IXUSR;
                    break;
                case '4':
                    mode = S_IRUSR;
                    break;
                case '3':
                    mode = S_IWUSR | S_IXUSR;
                    break;
                case '2':
                    mode = S_IWUSR;
                    break;
                case '1':
                    mode = S_IXUSR;
                    break;
                case '0':
                    break;
                default:
                    id = ERROR;
                    break;
                }
            }
            if(l == 2){
                switch (argv[1+flags][2]){
                case '7':
                    mode |= S_IRWXG;
                    break;
                case '6':
                    mode |= S_IRGRP | S_IWGRP;
                    break;
                case '5':
                    mode |= S_IRGRP | S_IXGRP;
                    break;
                case '4':
                    mode |= S_IRGRP;
                    break;
                case '3':
                    mode |= S_IWGRP | S_IXGRP;
                    break;
                case '2':
                    mode |= S_IWGRP;
                    break;
                case '1':
                    mode |= S_IXGRP;
                    break;
                case '0':
                    break;
                default:
                    id = ERROR;
                    break;
                }

            }
            if(l == 3){
                switch (argv[1+flags][3]){
                case '7':
                    mode |= S_IRWXO;
                    break;
                case '6':
                    mode |= S_IROTH | S_IWOTH;
                    break;
                case '5':
                    mode |= S_IROTH | S_IXOTH;
                    break;
                case '4':
                    mode |= S_IROTH;
                    break;
                case '3':
                    mode |= S_IWOTH | S_IXOTH;
                    break;
                case '2':
                    mode |= S_IWOTH;
                    break;
                case '1':
                    mode |= S_IXOTH;
                    break;
                case '0':
                    break;
                default:
                    id = ERROR;
                    break;
                }

            }       
        }
        
        if(id == ERROR){ 
        printf("Error in permissions");
        logExit(1);  //pq q esta a dar success?
        }
        
        struct stat buffer;
        if (stat(argv[2 + flags], &buffer)) {
            perror("stat()");
            logExit(-1);
        }
        if(S_ISDIR(buffer.st_mode)){ //If it is a directory, we need its path
        char *path = (char *) malloc(512 * sizeof(char));
        if(path == NULL){
            perror("malloc()");
            logExit(-1);
        }          
        path = getenv("PWD");   //Reads the env variable that has the path
        if(path == NULL){
            perror("getenv()");
            logExit(-1);
        }
        chmodFolder(flags, mode, argc, argv, envp);
        logExit(0);
        }

        else{
        chmodWrapper(argv[2+flags], mode); 
        logExit(0);
        }

    }   //end of octal mode

    else{ //normal mode
        user = argv[1+flags][0];
        op = argv[1+flags][1];
        while( (argv[1+flags][j] != '\0') && (j < 5) ){  //to add more than 1 permission
            perm[j - 2] = argv[1+flags][j];
            switch (user)
            {
            case 'a':
                id = 8;
                allFlag = true;
                break;
            case 'u':
                id = 8;
                break;
            case 'g':
                id = 5;
                break;
            case 'o':
                id = 2;
                break;
            default:
                id = ERROR;
                break;
            }
            //da para simplificar isto com enums??
            switch (op)
            {
            case '-':
                opFlag = 1;
                break;
            case '+':
                opFlag = 2;
                break;
            case '=':
                opFlag = 3;
                break;
            default:
                id = ERROR;
                break;
            }
            //fazer for para os varios espaços do vetor
            switch (perm[j - 2])
            {
            case 'r':
                id -= 0;
                break;
            case 'w':
                id -= 1;
                break;
            case 'x':
                id -= 2;
                break;
            default:
                id = ERROR;
                break;
            }

            if(id == ERROR){ 
                printf("Error in permissions");
                logExit(1);
            }

            if(allFlag){
                mode |= (1 << id) | (1 << (id - 3)) | (1 << (id - 6));
            }
            else mode |= 1 << id; //mode_t masks work like this

            stat(argv[2+flags],&startMode);


            j++;
        }   //while ends here
        
        switch (opFlag) {
            case 1:
                temp = mode;
                mode ^= startMode.st_mode;   //- (^ = xor) nao funciona de retirar algo sem nada
                break;

            case 2:
                mode |= startMode.st_mode;  //+
                break;

            default:
                break;
        }


        struct stat buffer;
        stat(argv[2 + flags], &buffer);
        if(S_ISDIR(buffer.st_mode)){ //If it is a directory, we need its path
        char *path = (char *) malloc(255 * sizeof(char));
        if(path == NULL){
            perror("malloc()");
            logExit(-1);
        }          

        path = getenv("PWD");   //Reads the env variable that has the path
        if(path == NULL){
            perror("getenv()");
            logExit(-1);
        }

        sprintf(path, "%s/%s", path, argv[2 + flags]);
        chmodFolder(flags, mode, argc, argv, envp);
        logExit(0);
        }

        else{
        chmodWrapper(argv[2+flags], mode); 
        logExit(0);
        }
    }   //end of normal mode
}