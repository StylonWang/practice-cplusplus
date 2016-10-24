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

#include <vector>

#include "common.h"

#define MODULE_NAME "mainParent"

static sem_t *g_print_sem;

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

    //dprintf("ChildB Process Created\n");
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

void passToChildB(std::vector<char> &integers)
{

}

void shutdownChilds(FILE *pipeA, pid_t pidB)
{
    // shutdown childA
    fprintf(pipeA, "-1\n"); // 
}

void waitForChilds(FILE *pipeA, pid_t pidB)
{
    pclose(pipeA);
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

    //TODO: kill childs
    exit(1);
}

int main(int argc, char **argv)
{
    FILE *pipeA;
    pid_t pidB;

    // initialize
    srandom(time(NULL));
    g_print_sem = sem_open(PRINT_SEM_KEY, O_CREAT, 0644, 1); // binary semaphore
    if(SEM_FAILED==g_print_sem) {
        fprintf(stderr, "cannot open semaphore: %s\n", strerror(errno));
        exit(1);
    }

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

    return 0;
}

