#ifndef __PCOLL_TASK_QUEUE__
#define __PCOLL_TASK_QUEUE__

#include <queue>
#include <shared_mutex>
#include <string>

template <typename T>
class Task_Queue {
private:
	std::queue<T> _queue;	// queue
	unsigned int _task_count;	// task
	mutable std::shared_mutex _queue_mutex; // mutex for queue
	mutable std::shared_mutex _task_count_mutex; // mutex for task

public:
	Task_Queue();
	void insert(T element);
	T poll();
	void decrement_task_count();
	unsigned int queue_size();
	unsigned int task_count();

};

#endif
