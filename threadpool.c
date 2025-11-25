#include"threadpool.h"

const int  CHANGENUM = 2;

ThreadPool* ThreadPoolCreate(int min, int max, int queueCapacity)
{
	printf("thread pool is creating\n");
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(struct ThreadPool));
	do {
		if (pool == NULL)//创建线程池失败
		{
			printf("malloc pool fail !\n");
			break;
		}

		pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
		if (pool->threadIDs == NULL)//创建工作线程失败
		{
			printf("malloc threadIDs fail !\n");
			break;
		}

		//初始化线程池
		memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
		pool->minNum = min;
		pool->maxNum = max;
		pool->busyNum = 0;
		pool->aliveNum = min;
		pool->exitNum = 0;
		pool->shutdown = 0;
		if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
			pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0)
		{
			printf("mutex or condition init fail !\n");
			break;
		}

		//初始化任务队列
		pool->taskQ = (Task*)malloc(sizeof(struct Task) * queueCapacity);
		pool->queueCapacity = queueCapacity;
		pool->queueSize = 0;
		pool->queueFront = 0;
		pool->queueRear = 0;

		//创建线程
		pthread_create(&pool->managerID, NULL, Manager, pool);
		for (int i = 0; i < min; i++)
		{
			pthread_create(&pool->threadIDs[i], NULL, Worker, pool);
		}
		printf("thread pool is created\n");
		return pool;
	} while (0);

	//因错误退出后释放资源
	if (pool && pool->threadIDs) free(pool->threadIDs);
	if (pool && pool->taskQ) free(pool->taskQ);
	if (pool) free(pool);
	return NULL;
}

int ThreadPoolDestory(ThreadPool* pool)
{
	printf("thread pool is destorying\n");
	if (pool == NULL)
	{
		return -1;
	}

	pool->shutdown = 1;//关闭线程池
	pthread_join(pool->managerID, NULL);//阻塞回收管理者线程
	pthread_cond_broadcast(&pool->notEmpty);//唤醒阻塞的工作线程

	//释放堆内存
	if (pool->taskQ) free(pool->taskQ);
	if (pool->threadIDs) free(pool->threadIDs);
	pthread_mutex_destroy(&pool->mutexBusy);
	pthread_mutex_destroy(&pool->mutexPool);
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);
	free(pool);
	pool = NULL;

	printf("thread pool is destoried\n");
	return 0;
}

void AddTask(ThreadPool* pool, void(*function), void* arg)
{
	pthread_mutex_lock(&pool->mutexPool);
	if (pool->shutdown)
	{
		pthread_mutex_unlock(&pool->mutexPool);
		return;
	}

	while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
	{
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);//阻塞生产者线程
	}

	//添加线程
	pool->taskQ[pool->queueRear].function = function;
	pool->taskQ[pool->queueRear].arg = arg;
	pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
	pool->queueSize++;

	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->mutexPool);
}

int GetBusyNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexBusy);
	int busyNum = pool->busyNum;
	pthread_mutex_unlock(&pool->mutexBusy);
	return busyNum;
}

int GetAliveNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexPool);
	int aliveNum = pool->aliveNum;
	pthread_mutex_unlock(&pool->mutexPool);
	return aliveNum;
}

void* Worker(ThreadPool* pool)
{
	while (1)
	{
		pthread_mutex_lock(&pool->mutexPool);//加锁

		while (pool->queueSize == 0 && !pool->shutdown)//队列若为空
		{
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);//则阻塞工作线程

			if (pool->exitNum > 0)//判断管理者线程是否要销毁工作线程
			{
				pool->exitNum--;
				if (pool->aliveNum > pool->minNum)
				{
					pool->aliveNum--;
					pthread_mutex_unlock(&pool->mutexPool);//pthread_cond_wait已将该线程锁住，需要再解锁
					ThreadExit(pool);
				}
			}
		}

		if (pool->shutdown)//若线程池关闭
		{
			//则解开互斥锁（防止死锁），并退出线程
			pthread_mutex_unlock(&pool->mutexPool);
			ThreadExit(pool);
		}

		//从任务队列中取出一个任务
		Task task;
		task.function = pool->taskQ[pool->queueFront].function;
		task.arg = pool->taskQ[pool->queueFront].arg;

		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;//移动头节点（循环队列）
		pool->queueSize--;

		pthread_cond_signal(&pool->notFull);//解锁
		pthread_mutex_unlock(&pool->mutexPool);

		printf("thread ID = %ld start working...\n", pthread_self());
		pthread_mutex_lock(&pool->mutexBusy);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->mutexBusy);

		task.function(task.arg);
		free(task.arg);
		task.arg = NULL;

		printf("thread ID = %ld end working...\n", pthread_self());
		pthread_mutex_lock(&pool->mutexBusy);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexBusy);
	}
	return NULL;
}

void* Manager(ThreadPool* pool)//管理者线程
{
	while (!pool->shutdown)
	{
		sleep(3);//每隔3秒检测一次

		//获取任务数量和线程数量
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->queueSize;
		int liveNum = pool->aliveNum;
		pthread_mutex_unlock(&pool->mutexPool);

		//获取忙的线程数量
		pthread_mutex_lock(&pool->mutexBusy);
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusy);

		//添加线程
		if (queueSize > liveNum && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			for (int i = 0, counter = 0; i < pool->maxNum && counter < CHANGENUM && pool->aliveNum < pool->maxNum; ++i)
			{
				if (pool->threadIDs[i] == 0)// 该位置无有效线程
				{
					pthread_create(&pool->threadIDs[i], NULL, Worker, pool);
					printf("new thread ID = %ld is created\n", pool->threadIDs[i]);
					counter++;
					pool->aliveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}

		//销毁线程
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = CHANGENUM;
			pthread_mutex_unlock(&pool->mutexPool);

			for (int i = 0; i < CHANGENUM; ++i)//让工作线程自杀
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}
	}
	return NULL;
}

void ThreadExit(ThreadPool* pool)
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < pool->maxNum; ++i)
	{
		if (tid == pool->threadIDs[i])
		{
			pool->threadIDs[i] = 0;
			printf("thread ID = %ld exit\n", tid);
			break;
		}
	}
	pthread_exit(NULL);
}
