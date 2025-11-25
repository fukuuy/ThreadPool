#include"ThreadPool.h"

int num;

void Func(void* arg)
{
    int num = *(int*)arg;
    cout << "thread ID = " << pthread_self() << " is working, number = " << num << endl;

    usleep(1000);
}

int main()
{
    ThreadPool pool(3, 10);
    for (int i = 0; i < 100; i++)
    {
        int* arg = new int(i);
        pool.AddTask(Task(Func, arg));
    }

    sleep(5);
    return 0;
}