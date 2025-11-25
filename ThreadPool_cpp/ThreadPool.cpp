#include "ThreadPool.h"


 ThreadPool::ThreadPool(int min, int max)
{
	 cout << "thread pool is creating" << endl;
	do {
		threadIDs = new pthread_t[max];
		if (threadIDs == nullptr)//创建工作线程失败
		{
			cout << "new threadIDs fail !" << endl;
			break;
		}

		//初始化线程池
		memset(threadIDs, 0, sizeof(pthread_t) * max);
		minNum = min;
		maxNum = max;
		busyNum = 0;
		aliveNum = min;
		exitNum = 0;
		shutdown = 0;
		if (pthread_mutex_init(&mutexPool, NULL) != 0 || pthread_cond_init(&notEmpty, NULL) != 0 )
		{
			cout<<"mutex or condition init fail !" << endl;
			break;
		}

		//实例化任务队列
		taskQ = new TaskQueue;
		if (taskQ == nullptr)//创建任务队列失败
		{
			cout << "new taskQ fail !" << endl;
			break;
		}

		//创建线程
		pthread_create(&managerID, NULL, Manager, this);
		for (int i = 0; i < min; i++)
		{
			pthread_create(&threadIDs[i], NULL, Worker, this);
		}

		cout << "thread pool is created" << endl;
		return;
	} while (0);

	//因错误退出后释放资源
	if (threadIDs) delete []threadIDs;
	if (taskQ) delete taskQ;
}

ThreadPool::~ThreadPool()
{
	cout << "thread pool is destorying" << endl;


	shutdown = 1;//关闭线程池
	pthread_join(managerID, NULL);//阻塞回收管理者线程
	pthread_cond_broadcast(&notEmpty);//唤醒阻塞的工作线程

	//释放堆内存
	if (taskQ) delete taskQ;
	if (threadIDs) delete []threadIDs;
	pthread_mutex_destroy(&mutexPool);
	pthread_cond_destroy(&notEmpty);
	cout << "thread pool is destoried" << endl;
}

void ThreadPool::AddTask(Task task)
{
	if (shutdown) return;

	//添加线程
	taskQ->AddTask(task);
	pthread_cond_signal(&notEmpty);

}

int ThreadPool::GetBusyNum()
{
	pthread_mutex_lock(&mutexPool);
	int busynum = busyNum;
	pthread_mutex_unlock(&mutexPool);
	return busyNum;
}

int ThreadPool::GetAliveNum()
{
	pthread_mutex_lock(&mutexPool);
	int alivenum = aliveNum;
	pthread_mutex_unlock(&mutexPool);
	return alivenum;
}

void* ThreadPool::Worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (1)
	{
		pthread_mutex_lock(&pool->mutexPool);//加锁

		while (pool->taskQ->GetTaskNumber() == 0 && !pool->shutdown)//队列若为空
		{
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);//则阻塞工作线程

			if (pool->exitNum > 0)//判断管理者线程是否要销毁工作线程
			{
				pool->exitNum--;
				if (pool->aliveNum > pool->minNum)
				{
					pool->aliveNum--;
					pthread_mutex_unlock(&pool->mutexPool);//pthread_cond_wait已将该线程锁住，需要再解锁
					pool->ThreadExit();
				}
			}
		}

		if (pool->shutdown)//若线程池关闭
		{
			//则解开互斥锁（防止死锁），并退出线程
			pthread_mutex_unlock(&pool->mutexPool);
			pool->ThreadExit();
		}

		//从任务队列中取出一个任务
		Task task = pool->taskQ->TakeTask();
		pool->busyNum++;
		cout << "thread ID = " << pthread_self() << " start working..." << endl;
		pthread_mutex_unlock(&pool->mutexPool);

		task.function(task.arg); 
		delete task.arg;
		task.arg = nullptr;
		
		pthread_mutex_lock(&pool->mutexPool);
		cout << "thread ID = " << pthread_self() << " end working..." << endl;
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexPool);
	}
	return NULL;
}

void* ThreadPool::Manager(void* arg)//管理者线程
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (!pool->shutdown)
	{
		sleep(3);//每隔3秒检测一次

		//获取任务数量和线程数量
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->taskQ->GetTaskNumber();
		int liveNum = pool->aliveNum;
		int busyNum = pool->busyNum;//获取忙的线程数量
		pthread_mutex_unlock(&pool->mutexPool);
		
		//添加线程
		if (queueSize > liveNum && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			for (int i = 0, counter = 0; i < pool->maxNum && counter < CHANGENUM && pool->aliveNum < pool->maxNum; ++i)
			{
				if (pool->threadIDs[i] == 0)// 该位置无有效线程
				{
					pthread_create(&pool->threadIDs[i], NULL, Worker, pool);
					cout << "new thread ID = " << pool->threadIDs[i] << " is created" << endl;
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

void ThreadPool::ThreadExit()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < maxNum; ++i)
	{
		if (tid == threadIDs[i])
		{
			threadIDs[i] = 0;
			cout << "thread ID = " << tid << " exit" << endl;
			break;
		}
	}
	pthread_exit(NULL);
}
