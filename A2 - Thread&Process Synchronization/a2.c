#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "a2_helper.h"

#define BARRIER_CAPACITY 6

void *p4_threadfunc(void *param);
void *p5_threadfunc(void *param);
void *p6_threadfunc(void *param);

typedef struct
{
    int thread_id;
    sem_t *t4_begin;
    sem_t *t1_done;
    sem_t *t6_1_end;
    sem_t *t4_2_end;
} TH_STRUCT_4;

typedef struct
{
    int thread_id;
    pthread_mutex_t *mutex;
    pthread_cond_t *barrier_full;
    pthread_cond_t *t12_can_end;
    pthread_cond_t *i_am_special;

    pthread_cond_t *priority_cond;
    pthread_mutex_t *priority_mutex;
} TH_STRUCT_5;

int barrier_count = 0;
int tstar_running = 0;
int tstar_expecting = 1;

int main()
{
    init();

    pid_t p1, p2, p3, p4, p5, p6, p7;
    pthread_t p4_tids[4], p5_tids[36], p6_tids[4];
    sem_t *t6_1_end = sem_open("t6_1_end", O_CREAT, 0644, 0);
    sem_t *t4_2_end = sem_open("t4_2_end", O_CREAT, 0644, 0);


    info(BEGIN, 1, 0);

    p1 = getpid();
    printf("%d\n", p1);
    p2 = fork();
    if (p2 == -1)
    {
        printf("Error creating process 2\n");
        return -1;
    }
    else if (p2 == 0)
    { //p2
        info(BEGIN, 2, 0);

        p7 = fork();
        if (p7 == -1)
        {
            printf("Error creating process 7\n");
            return -1;
        }
        else if (p7 == 0)
        { //p7
            info(BEGIN, 7, 0);

            info(END, 7, 0);
        }
        else
        { //p2
            wait(NULL);
            info(END, 2, 0);
        }
    }
    else
    { //P1
        p3 = fork();
        if (p3 == -1)
        {
            printf("Error creating process 3\n");
            return -1;
        }
        else if (p3 == 0)
        { //p3
            info(BEGIN, 3, 0);

            p4 = fork();
            if (p4 == -1)
            {
                printf("Error creating process 4\n");
                return -1;
            }
            else if (p4 == 0)
            { //p4
                info(BEGIN, 4, 0);

                p5 = fork();
                if (p5 == -1)
                {
                    printf("Error creating process 5\n");
                    return -1;
                }
                else if (p5 == 0) //p5
                {
                    info(BEGIN, 5, 0);
                    TH_STRUCT_5 params[36];
                    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
                    pthread_cond_t barrier_full = PTHREAD_COND_INITIALIZER;
                    pthread_cond_t t12_can_end = PTHREAD_COND_INITIALIZER;
                    pthread_cond_t i_am_special = PTHREAD_COND_INITIALIZER;
                    //sem_t t12_can_end;
                    //sem_init(&i_am_special, 0, 6);
                    pthread_mutex_t priority_mutex = PTHREAD_MUTEX_INITIALIZER;
                    pthread_cond_t priority_cond = PTHREAD_COND_INITIALIZER;

                    for (int i = 0; i < 36; i++)
                    {
                        params[i].thread_id = i + 1;
                        params[i].barrier_full = &barrier_full;
                        params[i].t12_can_end = &t12_can_end;
                        params[i].i_am_special = &i_am_special;
                        params[i].mutex = &mutex;

                        params[i].priority_mutex = &priority_mutex;
                        params[i].priority_cond = &priority_cond;

                        pthread_create(&p5_tids[i], NULL, p5_threadfunc, &params[i]);
                    }

                    for (int i = 0; i < 36; i++)
                    {
                        pthread_join(p5_tids[i], NULL);
                    }
                    pthread_mutex_destroy(&mutex);
                    pthread_mutex_destroy(&priority_mutex);
                    pthread_cond_destroy(&barrier_full);
                    pthread_cond_destroy(&t12_can_end);
                    pthread_cond_destroy(&i_am_special);
                    pthread_cond_destroy(&priority_cond);
                    info(END, 5, 0);
                }
                else
                {
                    p6 = fork();
                    if (p6 == -1)
                    {
                        printf("Error creating process 6\n");
                        return -1;
                    }
                    else if (p6 == 0) //p6
                    {
                        info(BEGIN, 6, 0);

                        TH_STRUCT_4 params[4];

                        for (int i = 0; i < 4; i++)
                        {
                            params[i].thread_id = i + 1;
                            params[i].t4_begin = NULL;
                            params[i].t1_done = NULL;
                            params[i].t6_1_end = t6_1_end;
                            params[i].t4_2_end = t4_2_end;
                            pthread_create(&p6_tids[i], NULL, p6_threadfunc, &params[i]);
                        }

                        for (int i = 0; i < 4; i++)
                        {
                            pthread_join(p6_tids[i], NULL);
                        }

                        info(END, 6, 0);
                    }
                    else //p4
                    {
                        TH_STRUCT_4 params[4];
                        sem_t t4_begin, t1_done;
                        sem_init(&t4_begin, 0, 0);
                        sem_init(&t1_done, 0, 0);

                        for (int i = 0; i < 4; i++)
                        {
                            params[i].thread_id = i + 1;
                            params[i].t4_begin = &t4_begin;
                            params[i].t1_done = &t1_done;
                            params[i].t6_1_end = t6_1_end;
                            params[i].t4_2_end = t4_2_end;
                            pthread_create(&p4_tids[i], NULL, p4_threadfunc, &params[i]);
                        }

                        for (int i = 0; i < 4; i++)
                        {
                            pthread_join(p4_tids[i], NULL);
                        }

                        wait(NULL);
                        wait(NULL);
                        info(END, 4, 0);
                    }
                }
            }
            else
            {
                wait(NULL);
                info(END, 3, 0);
            }
        }
        else
        { //p1
            sem_close(t6_1_end);
            sem_close(t4_2_end);
            wait(NULL);
            wait(NULL);
            info(END, 1, 0);
        }
    }
    return 0;
}

void *p4_threadfunc(void *param)
{
    TH_STRUCT_4 *arg = (TH_STRUCT_4 *)param;

    if (arg->thread_id == 4)
    {
        info(BEGIN, 4, 4);
        sem_post(arg->t4_begin);
    }
    else if (arg->thread_id == 1)
    {
        sem_wait(arg->t4_begin);
        info(BEGIN, 4, 1);
        info(END, 4, 1);
        sem_post(arg->t1_done);
    }
    else
    {
        if (arg->thread_id == 2)
        {
            sem_wait(arg->t6_1_end);
        }
        info(BEGIN, 4, arg->thread_id);

        if (arg->thread_id == 2)
        {
            info(END, 4, 2);
            sem_post(arg->t4_2_end);
            //printf("t4.2 finished ... \n");
        }
        else
            info(END, 4, arg->thread_id);
    }

    if (arg->thread_id == 4)
    {
        //pthread_join(arg->tids[0], NULL); //sounds good doesn't work
        sem_wait(arg->t1_done);
        info(END, 4, 4);
    }
    return NULL;
}

void *p5_threadfunc(void *param) //semaphore with lock + cond_var
{
    TH_STRUCT_5 *arg = (TH_STRUCT_5 *)param;

    pthread_mutex_lock(arg->mutex);
    while (barrier_count == BARRIER_CAPACITY)
    {
        pthread_cond_wait(arg->barrier_full, arg->mutex);
    }
    barrier_count++;
    if (barrier_count == BARRIER_CAPACITY)
    {
        pthread_cond_signal(arg->t12_can_end);
    }
    printf("Barrier count = %d\n", barrier_count);
    fflush(stdin);

    if (arg->thread_id == 12)
    {
        tstar_running = 1;
    }
    pthread_mutex_unlock(arg->mutex);


    info(BEGIN, 5, arg->thread_id);

    //pthread_mutex_lock(arg->mutex);
    if (arg->thread_id == 12)
    {
        while (barrier_count < BARRIER_CAPACITY)
        {
            pthread_cond_wait(arg->t12_can_end, arg->mutex);
        }
        pthread_mutex_lock(arg->mutex);
        tstar_running = 0;
        pthread_mutex_unlock(arg->mutex);
        info(END, 5, 12);
        printf("Thread 12 ended, now signaling the others \n");
        fflush(stdin);
        pthread_cond_broadcast(arg->i_am_special);
        return NULL;
    }
    //pthread_mutex_unlock(arg->mutex);

    pthread_mutex_lock(arg->mutex);
    while (tstar_running == 1)
    {
        printf("Thread %d waiting to end \n", arg->thread_id);
        fflush(stdin);
        pthread_cond_wait(arg->i_am_special, arg->mutex);
    }
    info(END, 5, arg->thread_id);
    barrier_count--;
    pthread_cond_signal(arg->barrier_full);
    pthread_mutex_unlock(arg->mutex);

    return NULL;

}

void *p6_threadfunc(void *param)
{
    TH_STRUCT_4 *arg = (TH_STRUCT_4 *)param;

    if (arg->thread_id == 3)
    {
        sem_wait(arg->t4_2_end);
    }
    info(BEGIN, 6, arg->thread_id);

    if (arg->thread_id == 1)
    {
        info(END, 6, 1);
        sem_post(arg->t6_1_end);
    }
    else
        info(END, 6, arg->thread_id);

    return NULL;
}
