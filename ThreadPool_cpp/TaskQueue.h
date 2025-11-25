#pragma once
#include<queue>
#include<pthread.h>
#include<iostream>

using namespace std;
using callback = void(*)(void* arg);

class Task
{
public:
	callback function;
	void* arg;

	Task()
	{
		function = nullptr;
		arg = nullptr;
	}
	Task(callback func, void* arg)
	{
		this->function = func;
		this->arg = arg;
	}
};

class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	//添加任务
	void AddTask(Task task);
	void AddTask(callback func, void* arg);
	//取出任务
	Task TakeTask();

	//获取当前任务个数
	inline size_t GetTaskNumber()
	{
		return taskQ.size();
	}

private :
	queue<Task> taskQ;
	pthread_mutex_t mutexT;
};

