#include "task_queue.hpp"
#include "utility.hpp"
#include <mutex>
#include <utility>
#include <tuple>

template <typename T>
Task_Queue<T>::Task_Queue() :
	_queue(),
	_task_count(),
	_queue_mutex(),
	_task_count_mutex()
{}

template <typename T>
void Task_Queue<T>::insert(T element){
	std::unique_lock<std::shared_mutex> lock_q(_queue_mutex, std::defer_lock);
	std::unique_lock<std::shared_mutex> lock_tc(_task_count_mutex, std::defer_lock);
	std::lock(lock_q, lock_tc);
	_queue.push(element);
	_task_count++;
}

template <typename T>
T Task_Queue<T>::poll(){
	std::unique_lock<std::shared_mutex> lock(_queue_mutex);
	if(_queue.empty()) throw Pexception("The queue is empty!");
	T value = _queue.front();
	_queue.pop();
	return value;
}

template <typename T>
void Task_Queue<T>::decrement_task_count(){
	std::unique_lock<std::shared_mutex> lock(_task_count_mutex);
	_task_count--;
}

template <typename T>
unsigned int Task_Queue<T>::queue_size(){
	std::shared_lock<std::shared_mutex> lock(_queue_mutex);
	return _queue.size();
}

template <typename T>
unsigned int Task_Queue<T>::task_count(){
	std::shared_lock<std::shared_mutex> lock(_task_count_mutex);
	return _task_count;
}

template class Task_Queue<std::string>;
template class Task_Queue<std::pair<std::string, std::string>*>;
