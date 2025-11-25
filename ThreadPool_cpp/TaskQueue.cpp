#include "TaskQueue.h"

TaskQueue::TaskQueue()
{
	pthread_mutex_init(&mutexT, NULL);
}

TaskQueue::~TaskQueue()
{
	pthread_mutex_destroy(&mutexT);
}

void TaskQueue::AddTask(Task task)
{
	pthread_mutex_lock(&mutexT);
	taskQ.push(task);
	pthread_mutex_unlock(&mutexT);
}

void TaskQueue::AddTask(callback func, void* arg)
{
	pthread_mutex_lock(&mutexT);
	taskQ.push(Task(func, arg));
	pthread_mutex_unlock(&mutexT);
}

Task TaskQueue::TakeTask()
{
	Task task;
	if (!taskQ.empty())
	{
		pthread_mutex_lock(&mutexT);
		task = taskQ.front();
		taskQ.pop();
		pthread_mutex_unlock(&mutexT);
	}
	return task;
}
