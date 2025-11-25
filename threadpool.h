#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>

#ifndef _TREADPOOL_H
#define _TREADPOOL_H
typedef struct ThreadPool ThreadPool;

//任务结构体
typedef struct Task
{
	void (*function)(void* arg);
	void* arg;
}Task;

//线程池结构体
struct ThreadPool
{
	pthread_t managerID;//管理者线程ID
	pthread_t* threadIDs;//工作的线程ID
	int minNum;//最小线程数
	int maxNum;//最大线程数
	int busyNum;//忙的线程数
	int aliveNum;//存活的线程数
	int exitNum;//要销毁的线程数
	pthread_mutex_t mutexPool;//整个线程池的锁
	pthread_mutex_t mutexBusy;//busyNum变量的锁

	int shutdown;//是否销毁线程池，是为1，否为0

	//任务队列参数
	Task* taskQ;
	int queueCapacity;//容量
	int queueSize;//当前任务数
	int queueFront;//队头
	int queueRear;//队尾
	pthread_cond_t notFull;//任务队列是否为满
	pthread_cond_t notEmpty;//任务队列是否为空
};

//创建线程池并初始化
ThreadPool* ThreadPoolCreate(int min, int max, int queueCapacity);

//销毁线程池
int ThreadPoolDestory(ThreadPool* pool);

//向线程池添加任务
void AddTask(ThreadPool* pool, void (*function), void* arg);

//获取线程池中工作的线程个数
int GetBusyNum(ThreadPool* pool);

//获取线程池中存活的线程个数
int GetAliveNum(ThreadPool* pool);

//工作线程
void* Worker(ThreadPool* pool);
//管理者线程
void* Manager(ThreadPool* pool);
//线程退出
void ThreadExit(ThreadPool* pool);
#endif

