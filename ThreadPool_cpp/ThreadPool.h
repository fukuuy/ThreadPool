#pragma once
#include"TaskQueue.h"
#include<string.h>
#include<string>
#include<unistd.h>

class ThreadPool
{
public:
	ThreadPool(int min, int max);
	~ThreadPool();

	void AddTask(Task task);

	int GetBusyNum();
	int GetAliveNum();

private:
	TaskQueue* taskQ;

	pthread_t managerID;
	pthread_t* threadIDs;
	int minNum;
	int maxNum;
	int busyNum;
	int aliveNum;
	int exitNum;
	static const int CHANGENUM = 2;
	pthread_mutex_t mutexPool;
	pthread_cond_t notEmpty;

	bool shutdown;

	static void* Worker(void* arg);
	static void* Manager(void* arg);
	void ThreadExit();

};

