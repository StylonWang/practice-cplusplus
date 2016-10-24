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

// TODO: protect fprintf with semaphore
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

pid_t runChildA(void)
{

    //dprintf("ChildA Process Created\n");
}

pid_t runChildB(void)
{

    //dprintf("ChildB Process Created\n");
}

void passToChildA(std::vector<char> &integers)
{

}

void passToChildB(std::vector<char> &integers)
{

}

void shutdownChilds(pid_t pidA, pid_t pidB)
{

}

void waitForChilds(pid_t pidA, pid_t pidB)
{

}

void generateIntegers(int n, std::vector<char> &integers)
{
    integers.clear(); // is this needed?

    dprintf_lock();

    dprintf_simple("Generating %d random integers: ", n);
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
    pid_t pidA, pidB;

    // initialize
    srandom(time(NULL));
    g_print_sem = sem_open(PRINT_SEM_KEY, O_CREAT, 0644, 1); // binary semaphore
    if(SEM_FAILED==g_print_sem) {
        fprintf(stderr, "cannot open semaphore: %s\n", strerror(errno));
        exit(1);
    }

    signal(SIGINT, signal_handler); // don't quit on Ctrl-C

    dprintf("Main Process Started\n");

    pidA = runChildA();
    pidB = runChildB();

    while(1) {
        int n;
        char buf[8];
        std::vector<char> integers;

        dprintf("Enter a positive integer or -1 to exit:     ");

        fgets(buf, sizeof(buf), stdin);
        n = atoi(buf);

        if(-1==n) break;   // got instruction to exit
        if(n<=0) continue; // we need positive integers

        generateIntegers(n, integers);

        passToChildA(integers);
        passToChildB(integers);
    }

    shutdownChilds(pidA, pidB);

    dprintf("Process Waits\n");
    waitForChilds(pidA, pidB);
    dprintf("Process Exits\n");

    sem_close(g_print_sem);

    return 0;
}

