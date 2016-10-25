#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <vector>

#include "common.h"

#define MODULE_NAME "mainParent"

static sem_t *g_print_sem;

static sem_t *g_queue_sem;
static int g_shm_handle;

// integer queue(with capacity 1) shared between parent and childB
// -1: not holding any integer value
// 0: integer value indicating the end of this batch
// >0: integer
static char *g_shm_integer;

#define dprintf(fmt, args...) \
    do { \
        sem_wait(g_print_sem); \
        fprintf(stdout, "[%s] " fmt, MODULE_NAME, ## args); \
        sem_post(g_print_sem); \
    } while(0)

#define dprintf_lock() do { sem_wait(g_print_sem); } while(0)
#define dprintf_unlock() do { sem_post(g_print_sem); } while(0)

// MUST protect dprintf_simple with dprintf_lock/unlock by yourself
#define dprintf_simple(fmt, args...) \
    do { \
        fprintf(stdout, fmt, ## args); \
    } while(0)


FILE *runChildA(void)
{
    FILE *pipe = popen("./childA", "w");
    
    if(pipe) {
        dprintf("ChildA Process Created\n");
    }
    else {
        dprintf("ChildA Process NOT created: %s\n", strerror(errno));
    }

    return pipe;
}

pid_t runChildB(void)
{
    pid_t pid;

    pid = fork();
    if(0==pid) { // child

        sem_close(g_print_sem);
        sem_close(g_queue_sem);
        munmap(g_shm_integer, sizeof(*g_shm_integer));

        execl("./childB", "childB",  NULL); // return on error
        dprintf("failed to exec ChildB: %s\n", strerror(errno));
        return 0;
    }
    else if(-1==pid) { // error
        dprintf("failed to fork ChildB: %s\n", strerror(errno));
        return pid;
    }
    
    // parent
    dprintf("ChildB Process Created\n");
    return pid;
}

void passToChildA(FILE *pipe, std::vector<char> &integers)
{
    for(unsigned int i=0; i<integers.size(); ++i) {
        //fprintf(stderr, "send %d\n", integers[i]); //debug
        fprintf(pipe, "%d\n", integers[i]);
    }
    fprintf(pipe, "0\n"); // end this batch
    fflush(pipe);
}

void passIntegerToChildB(char number)
{
    // this not pretty, but works OK when
    // there is only one produce and one consumer
    while(1) {
        sem_wait(g_queue_sem);
        if(-1==*g_shm_integer) { // queue is not full, insert integer
            *g_shm_integer = number;
            sem_post(g_queue_sem);
            break;
        }

        // queue is full, busy waiting for it to be empty
        sem_post(g_queue_sem);
        usleep(30*1000);
        continue; // continue looping
    }
}

void passToChildB(std::vector<char> &integers)
{
    for(unsigned int i=0; i<integers.size(); ++i) {
        passIntegerToChildB(integers[i]); 
    }
    passIntegerToChildB(0); // end this batch 
}

void shutdownChilds(FILE *pipeA, pid_t pidB)
{
    // shutdown childA
    fprintf(pipeA, "-1\n"); // 

    // shutdown childB
    kill(pidB, SIGTERM);
}

void waitForChilds(FILE *pipeA, pid_t pidB)
{
    // wait for childA to exit
    pclose(pipeA);

    // wait for childB to exit
    int status;
    waitpid(pidB, &status, 0);
}

void generateIntegers(int n, std::vector<char> &integers)
{
    integers.clear(); // is this needed?

    dprintf_lock();

    dprintf_simple("[%s] Generating %d random integers: ", MODULE_NAME, n);
    for(int i=0; i<n; ++i) {
        char t = 1+random()%50;
        integers.push_back(t);
        dprintf_simple("%d ", t);
    }
    dprintf_simple("\n");

    dprintf_unlock();
}

void signal_handler(int signo)
{
    fprintf(stderr, "got signal %d, exit\n", signo);
    sem_close(g_print_sem);
    sem_close(g_queue_sem);
    munmap(g_shm_integer, sizeof(*g_shm_integer));
    shm_unlink(INT_QUEUE_KEY);
    
    exit(1);
}

int main(int argc, char **argv)
{
    FILE *pipeA;
    pid_t pidB;

    // initialize
    srandom(time(NULL));
    
    // semaphore for print locking
    g_print_sem = sem_open(PRINT_SEM_KEY, O_CREAT, 0644, 1); // binary semaphore
    if(SEM_FAILED==g_print_sem) {
        fprintf(stderr, "cannot open semaphore: %s\n", strerror(errno));
        exit(1);
    }

    // semaphore with childB
    g_queue_sem = sem_open(INT_SEM_KEY, O_CREAT, 0644, 1); // binary semaphore
    if(SEM_FAILED==g_queue_sem) {
        fprintf(stderr, "cannot open semaphore: %s\n", strerror(errno));
        sem_close(g_print_sem);
        exit(1);
    }

    // shared memory to ChildB
    g_shm_handle = shm_open(INT_QUEUE_KEY, O_CREAT|O_RDWR, 0644);
    if(-1==g_shm_handle) {
        fprintf(stderr, "cannot create handle to share memory: %s\n", strerror(errno));
        sem_close(g_print_sem);
        sem_close(g_queue_sem);
        exit(1);
    }
    ftruncate(g_shm_handle, sizeof(*g_shm_integer));
    g_shm_integer = (char *)mmap(0, sizeof(*g_shm_integer), PROT_READ|PROT_WRITE, MAP_SHARED, 
                         g_shm_handle, 0);
    if(MAP_FAILED==g_shm_integer) {
        fprintf(stderr, "cannot attach to share memory: %s\n", strerror(errno));
        sem_close(g_print_sem);
        sem_close(g_queue_sem);
        shm_unlink(INT_QUEUE_KEY);
        exit(1);
    }
    close(g_shm_handle);

    *g_shm_integer = 0;

    // install signal handler for Ctrl-C
    signal(SIGINT, signal_handler);

    dprintf("Main Process Started\n");

    pipeA = runChildA();
    pidB = runChildB();

    while(1) {
        int n;
        char buf[8];
        std::vector<char> integers;

        dprintf("Enter a positive integer or -1 to exit:\n");

        fgets(buf, sizeof(buf), stdin);
        n = atoi(buf);

        if(-1==n) break;   // got instruction to exit
        if(n<=0) continue; // we need positive integers

        generateIntegers(n, integers);

        passToChildA(pipeA, integers);
        passToChildB(integers);
    }

    shutdownChilds(pipeA, pidB);

    dprintf("Process Waits\n");
    waitForChilds(pipeA, pidB);
    dprintf("Process Exits\n");

    sem_close(g_print_sem);
    sem_close(g_queue_sem);
    munmap(g_shm_integer, sizeof(*g_shm_integer));
    shm_unlink(INT_QUEUE_KEY);

    return 0;
}

