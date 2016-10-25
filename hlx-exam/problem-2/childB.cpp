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
#include <math.h>

#include <vector>
#include <algorithm>

#include "common.h"

#define MODULE_NAME "childB"

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

void signal_handler(int signo)
{
    dprintf("Child process exits\n");
    sem_close(g_print_sem);
    sem_close(g_queue_sem);
    munmap(g_shm_integer, sizeof(*g_shm_integer));

    exit(1);
}

char readIntegerFromParent(void)
{
    char n;
    // this not pretty, but works OK when
    // there is only one produce and one consumer
    while(1) {
        sem_wait(g_queue_sem);

        if(-1!=*g_shm_integer) { // queue is not empty, read integer
           n = *g_shm_integer; // read one integer from queue
           *g_shm_integer = -1; // empty queue
           sem_post(g_queue_sem);
           break;
        }

        // queue is empty, busy waiting for it to be not empty
        sem_post(g_queue_sem);
        usleep(100*1000);
        continue; // continue looping
    }

    return n;
}

int main(int argc, char **argv)
{
    // initialize
    g_print_sem = sem_open(PRINT_SEM_KEY, 0);
    if(SEM_FAILED==g_print_sem) {
        fprintf(stderr, "cannot open semaphore: %s\n", strerror(errno));
        exit(1);
    }

    // semaphore with parent
    g_queue_sem = sem_open(INT_SEM_KEY, 0); // binary semaphore
    if(SEM_FAILED==g_queue_sem) {
        fprintf(stderr, "cannot open semaphore: %s\n", strerror(errno));
        sem_close(g_print_sem);
        exit(1);
    }

    // shared memory with parent
    g_shm_handle = shm_open(INT_QUEUE_KEY, O_RDWR, 0644);
    if(-1==g_shm_handle) {
        fprintf(stderr, "cannot create handle to shared memory: %s\n", strerror(errno));
        sem_close(g_print_sem);
        sem_close(g_queue_sem);
        exit(1);
    }
    g_shm_integer = (char *)mmap(0, sizeof(*g_shm_integer), PROT_READ|PROT_WRITE, MAP_SHARED, 
                         g_shm_handle, 0);
    if(MAP_FAILED==g_shm_integer) {
        fprintf(stderr, "cannot attach to share memory: %s\n", strerror(errno));
        sem_close(g_print_sem);
        sem_close(g_queue_sem);
        shm_unlink(INT_QUEUE_KEY);
        exit(1);
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    dprintf("Child process started\n");

    std::vector<char> integers;

    while(1) {
        char a;
        
        a = readIntegerFromParent();

        if(a>0) { // positive integer, push to vector
            integers.push_back(a);
            continue;
        }

        // if(a==0)
        // "zero" end this batch of inputs, 
        // calculate standard deviation and print results

        double sum = 0;
        double mean, variance, stddev;

        if(integers.empty()) continue; // guard against zero inputs

        dprintf_lock();
        dprintf_simple("[%s] Random Numbers Received: ", MODULE_NAME);
        
        for(unsigned int i=0; i<integers.size(); ++i) {
            dprintf_simple("%d ", integers[i]);
            sum += integers[i];
        }
        dprintf_simple("\n");
        dprintf_unlock();

        // calculate mean
        mean = sum/integers.size();

        // calculate std deviation
        sum = 0;
        for(unsigned int i=0; i<integers.size(); ++i) {
            double dev = ( (double)integers[i] - mean);
            sum += (dev*dev);
        }
        variance = sum/integers.size();
        stddev = sqrt(variance);

        // print sorted integers
        std::sort(integers.begin(), integers.end());
        dprintf_lock();
        dprintf_simple("[%s] Sorted Sequence: ", MODULE_NAME);
        for(unsigned int i=0; i<integers.size(); ++i) {
            dprintf_simple("%d ", integers[i]);
        }
        dprintf_simple("\n");
        dprintf_unlock();

        dprintf("Standard Deviation: %f\n", stddev);

        integers.clear();
    } // end of while loop

    dprintf("Child process exits\n");

    sem_close(g_print_sem);
    sem_close(g_queue_sem);
    munmap(g_shm_integer, sizeof(*g_shm_integer));

    return 0;
}

