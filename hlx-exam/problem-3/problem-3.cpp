#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

using namespace std;

// mutex provide atomicity
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

// condition variables used as signals between threads,
// so each thread is less likely to contend for "mutex" lock every 900 mili seconds.
static pthread_cond_t cond_ab = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_bc = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_cd = PTHREAD_COND_INITIALIZER;

static long count = 0;

void *thread_A_routine(void *data)
{
    // signal to thread B to start execution
    pthread_cond_signal(&cond_ab);

    while(1) {
        usleep(900*1000);

        pthread_mutex_lock(&mutex);
        if(count<=50) {
            ++count;
        }
        cerr << count << endl;
        pthread_mutex_unlock(&mutex);
    }
}

void *thread_B_routine(void *data)
{
    // wait for signal to thread A
    pthread_mutex_lock(&cond_mutex);
    pthread_cond_wait(&cond_ab, &cond_mutex);
    pthread_mutex_unlock(&cond_mutex);
    // start execution 100 miliseconds later
    usleep(100*1000);
    // signal to thread C to start execution
    pthread_cond_signal(&cond_bc);

    while(1) {
        usleep(900*1000);

        pthread_mutex_lock(&mutex);
        if(count<=50) {
            count = count+2;
        }
        cerr << count << endl;
        pthread_mutex_unlock(&mutex);
    }
}

void *thread_C_routine(void *data)
{
    // wait for signal to thread B
    pthread_mutex_lock(&cond_mutex);
    pthread_cond_wait(&cond_bc, &cond_mutex);
    pthread_mutex_unlock(&cond_mutex);
    // start execution 100 miliseconds later
    usleep(100*1000);
    // signal to thread C to start execution
    pthread_cond_signal(&cond_cd);

    while(1) {
        usleep(900*1000);

        pthread_mutex_lock(&mutex);
        if(count>10) {
            --count;
        }
        cerr << count << endl;
        pthread_mutex_unlock(&mutex);
    }
}

void *thread_D_routine(void *data)
{
    // wait for signal to thread C
    pthread_mutex_lock(&cond_mutex);
    pthread_cond_wait(&cond_cd, &cond_mutex);
    pthread_mutex_unlock(&cond_mutex);
    // start execution 100 miliseconds later
    usleep(100*1000);

    while(1) {

        usleep(900*1000);

        pthread_mutex_lock(&mutex);
        if(count>30) {
            count = count-3;
        }
        cerr << count << endl;
        pthread_mutex_unlock(&mutex);
    }
}

int main(int argc, char **argv)
{
    pthread_t thB, thC, thD;
    int ret;

    // create threads B, C and D
    ret = pthread_create(&thB, NULL, thread_B_routine, NULL);
    if(ret) {
        cerr << "cannot create thread B: " << strerror(ret) << endl;
        return 1;
    }
    ret = pthread_create(&thC, NULL, thread_C_routine, NULL);
    if(ret) {
        cerr << "cannot create thread C: " << strerror(ret) << endl;
        return 1;
    }
    ret = pthread_create(&thD, NULL, thread_D_routine, NULL);
    if(ret) {
        cerr << "cannot create thread D: " << strerror(ret) << endl;
        return 1;
    }

    // running thread A in main thread

    thread_A_routine(NULL);

    return 0;
}

