#ifndef __PCOLL_PCOLL__
#define __PCOLL_PCOLL__

#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <list>
#include <unordered_set>
#include <queue>
#include <bitset>
#include <utility>

#include "filesystem.hpp"
#include "utility.hpp"
#include "task_queue.hpp"
#include "pcoll_database.hpp"

using std::unordered_map;
using std::unordered_set;
using std::string;
using std::bitset;

class Pcoll {
public:
	static Results find_similar_images(std::list<string>& directories, std::unordered_set<string>& exclude, bool quiet, float percentage, unsigned int num_threads);
private:
	static bool process_path(bool quiet, Task_Queue<std::string>& path_queue, std::unordered_set<string>& exclude, Task_Queue<std::string>& file_queue);
	static bool process_file(bool quiet, Task_Queue<std::string>& file_queue, Pcoll_Database& db);
	static void print_progress(const string& path, Pcoll_Database& db);
};

#endif //__PCOLL_PCOLL__
