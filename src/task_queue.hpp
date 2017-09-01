#ifndef __PCOLL_TASK_QUEUE__
#define __PCOLL_TASK_QUEUE__

#include "utility.hpp"

#include <queue>
#include <shared_mutex>
#include <string>
#include <mutex>

template <class T>
class Task_Queue {
private:
	std::queue<T> _queue;	// queue
	unsigned int _task_count;	// task
	mutable std::shared_mutex _queue_mutex; // mutex for queue
	mutable std::shared_mutex _task_count_mutex; // mutex for task

public:
	Task_Queue() :
		_queue(),
		_task_count(),
		_queue_mutex(),
		_task_count_mutex()
	{}

	void insert(T element){
		std::unique_lock<std::shared_mutex> lock_q(_queue_mutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> lock_tc(_task_count_mutex, std::defer_lock);
		std::lock(lock_q, lock_tc);
		_queue.push(element);
		_task_count++;
	}

	T poll(){
		std::unique_lock<std::shared_mutex> lock(_queue_mutex);
		if(_queue.empty()) throw Pexception("The queue is empty!");
		T value = _queue.front();
		_queue.pop();
		return value;
	}

	void decrement_task_count(){
		std::unique_lock<std::shared_mutex> lock(_task_count_mutex);
		_task_count--;
	}

	unsigned int queue_size(){
		std::shared_lock<std::shared_mutex> lock(_queue_mutex);
		return _queue.size();
	}

	unsigned int task_count(){
		std::shared_lock<std::shared_mutex> lock(_task_count_mutex);
		return _task_count;
	}

};

#endif
