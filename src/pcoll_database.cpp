#include "pcoll_database.hpp"
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
	_path_storage(),
	_path_storage_mutex(),
	_dhash_storage(),
	_dhash_storage_mutex(),
	_chash_storage(),
	_chash_storage_mutex(),
	_path_to_chash_database(),
	_path_to_chash_database_mutex(),
	_chash_to_path_set_database(),
	_chash_to_path_set_database_mutex(),
	_dhash_database(),
	_dhash_database_mutex()
{}

Pcoll_Database::~Pcoll_Database(){
	reset();
}

void Pcoll_Database::insert(string& path){

	// Copy string and store it
	string* copy_path = new string(path);
	_path_storage.push_back(copy_path);

	// Get file hash and store it
	File_Checksum* hash = File_Checksum::compute_hash_by_file(*copy_path);
	_chash_storage.push_back(hash);

	// Update the path-chash database
	{
		std::unique_lock<std::shared_mutex> lock(_path_to_chash_database_mutex);
		auto id = std::hash<string>()(*copy_path);
		auto search = _path_to_chash_database.find(id);
		if(search == _path_to_chash_database.end()){
			_path_to_chash_database.insert(std::make_pair(id, hash));
		}
	}

	{ // Scope for chash_database unique_lock

		// Lock the database for updating
		std::unique_lock<std::shared_mutex> lock(_chash_to_path_set_database_mutex);

		// Look for matching hash
		auto id = std::hash<string>()(hash->get_string());
		auto search = _chash_to_path_set_database.find(id);
		if(search == _chash_to_path_set_database.end()){  // A matching hash not is found

			// Create new entry with brand new unordered set and put filepath in it
			std::unordered_set<string*> set;
			set.insert(copy_path);

			// Put the new set in the database with the hash
			_chash_to_path_set_database.insert(std::make_pair(id, set));

			// Check if file is an image
			if(Utility::is_image(*copy_path)){

				// Compute the hash
				Difference_Hash* dhash = new Difference_Hash(*copy_path);

				// Store it in the storage
				_dhash_storage.push_back(dhash);

				// Then put it in the database
				std::unique_lock<std::shared_mutex> lock(_dhash_database_mutex);
				_dhash_database.insert(std::make_pair(id, dhash));
			}
		}else{ // a matching hash is found, add to the set
			search->second.insert(copy_path);
		}
	}

	_total++;
}

unsigned int Pcoll_Database::size() {
	return _total;
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

	// Ensure that number of threads is not zero
	if(num_threads == 0) num_threads = 1;

	// Compute similarity in Difference Hashes
	unordered_map<std::size_t, std::unordered_map<std::size_t, float>> dhash_results = compile_dhash_similarity(percentage, num_threads); // <Checksum_id, <Checksum_id, similarity>>

	// Create results storage
	Results results;

	// Create queue
	Task_Queue<string*> results_queue;

	// Build the thread function
	auto results_compilation_function = [&](){

		// Finish all tasks
		while(results_queue.task_count() != 0){

			// Try to extract dhash queue
			try{
				// Extract
				string* path = results_queue.poll();

				// Construct list
				std::list<std::pair<string, float>> collisions;

				// Process Checksums - find chash to corresponding path
				File_Checksum* chash = _path_to_chash_database[std::hash<string>()(*path)];

				// Get id of that checksum
				size_t chash_id = std::hash<string>()(chash->get_string());

				// Get list of file paths with same Checksum
				std::unordered_set<std::string*> files = _chash_to_path_set_database[chash_id];

				// Put collisions in the list
				for(auto& other_files : files){
					if(*other_files != *path){ // Ignore if the comparing file is by itself
						std::string str(*other_files); // copy
						collisions.push_back(std::make_pair(str, 1.0f));
					}
				}

				// Process Difference Hash - find dhash set to corresponding chash
				std::unordered_map<std::size_t, float> dhash_collisions = dhash_results[chash_id];

				// Put dhash collisions in the list
				for(auto& entry : dhash_collisions){

					// Get file path names of corresponding File Checksum
					std::unordered_set<string*> paths = _chash_to_path_set_database[entry.first];

					// Go through file paths
					for(auto& other_files : paths){
						if(*other_files != *path){ // Ignore if the comparing file is by itself
							float percent = entry.second == 1.0f ? 0.99f : entry.second; // If both files don't match the checksum but rates 100% on Dhash, it's safe to assume it's very similar but not same
							std::string str(*other_files); // copy
							collisions.push_back(std::make_pair(str, percent));
						}
					}
				}

				if(collisions.size() != 0){

					// Sort the list to descending percentage of matches
					collisions.sort([](const std::pair<string,float>& one, const std::pair<string,float>& two) -> bool {
						return one.second > two.second;
					});

					// Put the list into the results struct
					std::string str(*path); // copy
					results.collisions.push_back(std::make_pair(str, collisions));

				}

				// Decrement task count
				results_queue.decrement_task_count();

			}catch(Pexception &e){
				sleep_for(milliseconds(10)); // Relax for a bit
			}
		}
	};

	// Create threads
	std::list<std::unique_ptr<std::thread>> threads;
	for(unsigned int i = 0; i < num_threads-1; i++){
		std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(results_compilation_function);
		threads.push_back(std::move(thread));
	}

	{
		std::shared_lock<std::shared_mutex> lock_path_storage(_path_storage_mutex);
		for(auto& entry : _path_storage){
			results_queue.insert(entry);
		}
	}

	// Run on main thread// Run on main thread
	results_compilation_function();

	// Join all threads if theres any
	for(auto& thread : threads)
		thread->join();
	threads.clear();

	// Sort the results starting with highest hits
	results.collisions.sort([](const std::pair<string, std::list<std::pair<string, float>>>& one, const std::pair<string, std::list<std::pair<string, float>>>& two) -> bool {
		return one.second.size() > two.second.size();
	});

	return results;
}

void Pcoll_Database::reset(){
	// Create locks and lock them simulateously
	std::unique_lock<std::shared_mutex> lock_path_to_chash(_path_to_chash_database_mutex, std::defer_lock);
	std::unique_lock<std::shared_mutex> lock_chash_to_path_set(_chash_to_path_set_database_mutex, std::defer_lock);
	std::unique_lock<std::shared_mutex> lock_dhash(_dhash_storage_mutex, std::defer_lock);
	lock(lock_path_to_chash, lock_chash_to_path_set, lock_dhash);

	// Clear databases
	_path_to_chash_database.clear();
	_chash_to_path_set_database.clear();
	_dhash_database.clear();

	// Delete paths
	for(auto& path : _path_storage) delete path;

	// Delete dhashes
	for(auto& hash : _dhash_storage) delete hash;

	// Delete chashes
	for(auto& hash : _chash_storage) delete hash;

	_total = 0;
}

unordered_map<std::size_t, std::unordered_map<std::size_t, float>> Pcoll_Database::compile_dhash_similarity(float percentage, unsigned int num_threads){

	// Create results storage
	unordered_map<std::size_t, std::unordered_map<std::size_t, float>> results; // <File_Checksum id, map<File_Checksum id, percent>>
	std::mutex results_mutex;

	// Compare dhash to every other dhash in dhash database
	Task_Queue<std::unordered_map<std::size_t, Difference_Hash*>::const_iterator> dhash_queue; // <File_Checksum id, Difference_Hash*>
	Task_Queue<std::pair<std::size_t, std::size_t>> compare_queue; // <File_Checksum id, File_Checksum id>

	// Build the thread function
	auto dhash_comparison_function = [&](){

		// Finish all tasks
		while(dhash_queue.task_count() != 0 || compare_queue.task_count() != 0){

			//std::cout << "tc1:\t" << dhash_queue.task_count() << "\ttc2:\t" << compare_queue.task_count() << std::endl << std::flush;

			// Flags for threads to either wait
			bool dhash_queue_results = true;
			bool compare_queue_results = true;

			// Try to extract dhash queue
			try{
				// Extract
				auto element = dhash_queue.poll();

				// Store the current value
				std::size_t file_id = element->first;

				// Advance the element;
				element++;

				// Iterate elements
				for(; element != _dhash_database.cend(); element++){
					compare_queue.insert(std::make_pair(file_id, element->first));
				}

				// Decrement task count
				dhash_queue.decrement_task_count();

			}catch(Pexception &e){
				dhash_queue_results = false;
			}

			// Try to extract compare queue
			try{
				// Extract
				auto element = compare_queue.poll();

				// Get Difference Hashes
				Difference_Hash* first = _dhash_database[element.first];
				Difference_Hash* second = _dhash_database[element.second];

				// Get comparison results
				float result_percent = Difference_Hash::compare(*first, *second);

				// If equal or exceed the expected percentage, insert
				if(result_percent >= percentage){

					// Lock the results storage and insert results
					std::unique_lock<std::mutex> lock_mutex(results_mutex);

					// Find first part of pair in the results map
					auto search_dhash_results = results.find(element.first);
					if(search_dhash_results == results.end()){

						// If it doesnt exist, create new map and put results in
						unordered_map<std::size_t, float> map;
						map.insert(std::make_pair(element.second, result_percent));
						results.insert(std::make_pair(element.first, map));

					}else{
						// It exists, update the map
						search_dhash_results->second.insert(std::make_pair(element.second, result_percent));
					}

					// Vice versa - find second part of pair in the results map
					search_dhash_results = results.find(element.second);
					if(search_dhash_results == results.end()){

						// If it doesnt exist, create new map and put results in
						unordered_map<std::size_t, float> map;
						map.insert(std::make_pair(element.first, result_percent));
						results.insert(std::make_pair(element.second, map));

					}else{
						// It exists, update the map
						search_dhash_results->second.insert(std::make_pair(element.first, result_percent));
					}
				}

				// Decrement task count
				compare_queue.decrement_task_count();

			}catch(Pexception &e){
				compare_queue_results = false;
			}

			// If nothing is in the queues, wait a bit for other threads to populate it
			if(!dhash_queue_results && !compare_queue_results){
				sleep_for(milliseconds(10)); // Relax for a bit
			}
		}
	};

	// Create threads
	std::list<std::unique_ptr<std::thread>> threads;
	for(unsigned int i = 0; i < num_threads-1; i++){
		std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(dhash_comparison_function);
		threads.push_back(std::move(thread));
	}

	// Populate dhash_queue
	for(auto entry = _dhash_database.cbegin(); entry != _dhash_database.cend(); entry++){
		dhash_queue.insert(entry);
	}

	// Run on main thread// Run on main thread
	dhash_comparison_function();

	// Join all threads if theres any
	for(auto& thread : threads)
		thread->join();

	return results;
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
