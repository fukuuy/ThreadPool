#include<stdio.h>
#include"threadpool.h"

int num;

void Func(void* arg)
{
	int num = *(int*)arg;  
	printf("thread ID = %ld is working, number = %d\n", pthread_self(), num);

	usleep(1000);
}

int main()
{
    ThreadPool* pool = ThreadPoolCreate(3, 10, 100);
    for (int i = 0; i < 100; i++)
    {
        int* arg = (int*)malloc(sizeof(int));
        *arg = i;
        AddTask(pool, Func, arg);
    }

    while (1) {
        pthread_mutex_lock(&pool->mutexPool);
        if (pool->queueSize == 0 && GetBusyNum(pool) == 0) {
            pthread_mutex_unlock(&pool->mutexPool);
            break;
        }
        pthread_mutex_unlock(&pool->mutexPool);
        sleep(1); 
    }
    ThreadPoolDestory(pool);
    return 0;
}