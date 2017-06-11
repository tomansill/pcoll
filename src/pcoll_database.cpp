#include "pcoll_database.hpp"
#include "diffhash.hpp"
#include "cryptohash.hpp"
#include "task_queue.hpp"
#include "utility.hpp"

#include <mutex>
#include <thread>
#include <unordered_set>
#include <functional>
#include <chrono>
#include <thread>

using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::string;

Pcoll_Database::Pcoll_Database():
	_total(0),
	_dhash_database(),
	_dhash_database_mutex(),
	_chash_database(),
	_chash_database_mutex()
{}

void Pcoll_Database::insert(string& path){
	{
		string hash = Crypto_Hash::compute_hash(path);
		std::unique_lock<std::shared_mutex> lock(_chash_database_mutex);
		_chash_database.insert(std::make_pair(path, hash));
	}
	{
		std::bitset<64> hash = Difference_Hash::compute_hash(path);
		std::unique_lock<std::shared_mutex> lock(_dhash_database_mutex);
		_dhash_database.insert(std::make_pair(path, hash));
	}
	_total++;
}

unsigned int Pcoll_Database::size() {
	return _total++;
}

Results Pcoll_Database::compile_similarity_results(bool quiet, float percentage){
	// Get the number of cores available on the computer
	unsigned int num_threads = std::thread::hardware_concurrency();

	// The previous function may return zero if it cannot detect the number of cores on the computer
	// Make sure we correct that before passing it in
	num_threads = num_threads == 0 ? 1 : num_threads;

	// Start the process
	return compile_similarity_results(quiet, percentage, num_threads);
}

Results Pcoll_Database::compile_similarity_results(bool quiet, float percentage, unsigned int num_threads){

	if(num_threads == 0) num_threads = 1;

	// Construct the visited lookup table
	std::unordered_set<std::pair<string, string>*, std::function<size_t(const std::pair<string, string>* obj)>> visited(0, [](const std::pair<string, string>* obj) {
		if(obj->first < obj->second) return std::hash<string>()(obj->first) + std::hash<string>()(obj->second);
		else return std::hash<string>()(obj->second) + std::hash<string>()(obj->first);
	});
	std::shared_mutex visited_mutex;

	// Construct the results collection object
	std::unordered_map<string, std::unordered_map<string, float>> collisions;
	std::mutex collisions_mutex;

	// Function for easy collision insert
	auto insert_collision = [&](string& member, string collided, float similarity) {
		std::unique_lock<std::mutex> lock(collisions_mutex);
		auto search = collisions.find(member);
		if(search != collisions.end()){
			search->second.emplace(collided, similarity);
		}else{
			std::unordered_map<string, float> colls;
			colls.emplace(collided, similarity);
			collisions.emplace(member, colls);
		}
	};

	// Build task queue
	Task_Queue<string> file_queue;
	Task_Queue<std::pair<string, string>*> task_queue;

	// Populate the file task queue
	{
		std::shared_lock<std::shared_mutex> lock(_dhash_database_mutex);
		for(auto& entry : _dhash_database)
			file_queue.insert(entry.first);
	}

	// Build the thread function
	auto thread_function = [&](){

		// Finish all tasks
		while(file_queue.task_count() != 0 || task_queue.task_count() != 0){

			// Pair up
			bool file = true;
			try{
				// Get from file queue
				string file_path_string = file_queue.poll();

				// Pair em up
				for(auto& entry : _dhash_database){
					std::shared_lock<std::shared_mutex> lock(_dhash_database_mutex);
					if(entry.first != file_path_string){
						std::pair<string, string>* pair = new std::pair<string, string>(file_path_string, entry.first);
						{
							std::unique_lock<std::shared_mutex> lock_visit(visited_mutex);
							visited.insert(pair);
						}
						task_queue.insert(pair);
					}
				}

				file_queue.decrement_task_count();
			}catch(Pexception& pe){
				file = false;
			}

			// Compute pairs
			bool pair = true;
			try{
				// Get from task queue
				std::pair<string, string>* work_pair = task_queue.poll();

				// Get hash one
				string c_hash_one;
				{
					std::shared_lock<std::shared_mutex> lock(_chash_database_mutex);
					auto search = _chash_database.find(work_pair->first);
					if(search == _chash_database.end()) throw Pexception("FATAL ERROR: '" + work_pair->first + "' not found in the hash database!");
					c_hash_one = search->second;
				}

				// Get hash two
				string c_hash_two;
				{
					std::shared_lock<std::shared_mutex> lock(_chash_database_mutex);
					auto search = _chash_database.find(work_pair->second);
					if(search == _chash_database.end()) throw Pexception("FATAL ERROR: '" + work_pair->second + "' not found in the hash database!");
					c_hash_two = search->second;
				}

				// Check if identical
				if(Crypto_Hash::compare_hashes(c_hash_one, c_hash_two)){
					//collisions
					insert_collision(work_pair->first, work_pair->second, 1.0f);
					insert_collision(work_pair->second, work_pair->first, 1.0f);
				}else{
					// Get hash one
					bitset<64> hash_one;
					{
						std::shared_lock<std::shared_mutex> lock(_dhash_database_mutex);
						auto search = _dhash_database.find(work_pair->first);
						if(search == _dhash_database.end()) throw Pexception("FATAL ERROR: '" + work_pair->first + "' not found in the hash database!"); // replace Pexception to something else
						hash_one = search->second;
					}

					// Get hash two
					bitset<64> hash_two;
					{
						std::shared_lock<std::shared_mutex> lock(_dhash_database_mutex);
						auto search = _dhash_database.find(work_pair->second);
						if(search == _dhash_database.end()) throw Pexception("FATAL ERROR: '" + work_pair->second + "' not found in the hash database!");
						hash_two = search->second;
					}

					// Compare hashes
					float result = Difference_Hash::compare_hashes(hash_one, hash_two);

					// Check if within acceptable level of similarity
					if(result > percentage){
						//collisions
						insert_collision(work_pair->first, work_pair->second, result);
						insert_collision(work_pair->second, work_pair->first, result);
					}
				}

				// Decrement the task count
				task_queue.decrement_task_count();

			}catch(Pexception& pe){
				pair = false;
			}

			// If nothing is in the queue, wait a bit for other threads to populate it
			if(!file && !pair){
				sleep_for(milliseconds(10)); // Relax for a bit
			}else{
				unsigned int size = 0;
				{
					std::unique_lock<std::mutex> lock(collisions_mutex);
					size = collisions.size()/2;
				}
				if(!quiet) print_progress(task_queue.task_count(), size);
			}
		}
	};

	// Create threads
	std::list<std::unique_ptr<std::thread>> threads;
	for(unsigned int i = 0; i < num_threads-1; i++){
		std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(thread_function);
		threads.push_back(std::move(thread));
	}

	// Run on main thread
	thread_function();

	// Join all threads if theres any
	for(auto& thread : threads)
		thread->join();

	// Clean up
	for(auto entry : visited){
		delete entry;
	}

	// Convert collisions to results
	Results result;
	result.files = collisions.size()/2;
	for(auto& entry : collisions){

		// Build the list
		std::list<std::pair<string, float>> list;
		for(auto& inside : entry.second){
			list.push_back(std::make_pair(inside.first, inside.second));
		}

		// Sort the list starting with highest similarity percentage
		list.sort([](const std::pair<string,float>& one, const std::pair<string,float>& two) -> bool {
			return one.second > two.second;
		});

		// Put it in the results
		result.collisions.push_back(std::make_pair(entry.first, list));
	}

	// Sort the results starting with highest hits
	result.collisions.sort([](const std::pair<string, std::list<std::pair<string, float>>>& one, const std::pair<string, std::list<std::pair<string, float>>>& two) -> bool {
		return one.second.size() > two.second.size();
	});

	return result;
}

void Pcoll_Database::reset(){
	std::unique_lock<std::shared_mutex> lock(_dhash_database_mutex);
	_dhash_database.clear();
}

void Pcoll_Database::print_progress(const unsigned int task_count, const unsigned int collisions){

	// Build the string
	std::stringstream ss;
	ss << " tasks: " << task_count;
	ss << "  collisions: " << collisions;

	// Convert to string
	string search = ss.str();

	// Put the string to output
	Utility::sout.print(search);
}
