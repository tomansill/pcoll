#include "pcoll.hpp"
#include <sstream>
#include <regex>
#include <chrono>
#include <thread>

using std::unique_lock;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;

Pcoll::Pcoll(float similarity_percentage) : Pcoll(similarity_percentage, std::list<string> directories, unordered_set<string> excludes)
{
	// Build the initial path queue
	for(auto& directory : directories){
		// Poll in the queue
		_path_queue.push(try_to_convert_to_absolute_path(directory));
		_task_count++;
	}
}

Pcoll::Pcoll(float similarity_percentage, const std::list<string> &directories, unordered_set<string> &excludes) :
	_similarity_percentage(similarity_percentage),
	_excludes(excludes),
	_hash_database(),
	_hash_database_mutex(),
	_dhash_database(),
	_dhash_database_mutex(),
	_collisions(),
	_collisions_mutex(),
	_path_queue(),
	_path_queue_mutex(),
	_image_queue(),
	_image_queue_mutex(),
	_task_count(0),
	_task_count_mutex(),
	_thread_count(0),
	_thread_count_mutex(),
	_total(0),
	_total_mutex(),
	_print_length(0),
	_cout_mutex(),
	_work_cv_mutex(),
    _work_cv()
{
	// Build the initial path queue
	for(auto& directory : directories){
		// Poll in the queue
		_path_queue.push(try_to_convert_to_absolute_path(directory));
		_task_count++;
	}
}

Results Pcoll::process(bool quiet){
	return process(quiet, 4); // no internet :(
}

Results Pcoll::process(bool quiet, unsigned int threads){
	// If one thread, run in main thread
	if(threads <= 1){
		while(true){
			bool path = process_path(quiet);
			bool image = process_image(quiet);
			if(!path && !image) break;
		}
	}else{

		// Build the thread function
		auto thread_function = [&](){

			// Finish all tasks
			while(get_task_count() != 0){
				bool path = process_path(quiet);
				bool image = process_image(quiet);

				// If nothing is in the queue, wait a bit for other threads to populate it
				if(!path && !image)
					sleep_for(milliseconds(10)); // Relax for a bit
			}

			// Good bye
			notify();
		};

		// Create threads and detach them
		_thread_count = threads;
		for(unsigned int i = 0; i < threads; i++){
			std::thread thread(thread_function);
			thread.detach();
		}

		// Wait until both queues are empty
		std::unique_lock<std::mutex> lock(_work_cv_mutex);
		while(get_length_of_queue() != 0 || get_task_count() != 0 || get_thread_count() != 0)
			_work_cv.wait(lock);
	}

	if(!quiet){
		std::stringstream ss;
		ss << "Done! Total: " << _total;
		println(ss.str());
	}

	// Return the result
	Results results;
	return results;
}

bool Pcoll::process_path(bool quiet){

	// Attempt to get the image path from queue
	string path_string;
	{
		unique_lock<std::mutex> lock(_path_queue_mutex);
		if(_path_queue.empty()) return false;
		path_string = _path_queue.front();
		_path_queue.pop();
	}

	// Convert to path object to use filesystem API
	filesystem::path path(path_string);

	// If element is a directory, add items inside the directory to the queue
	if(filesystem::is_directory(path)){

		// Print statistics
		if(!quiet) print_progress(path_string);

		// Iterate through the directory
		for(filesystem::path file : filesystem::directory_iterator(path)){
			// Check if path is part of excluded directory set
			if(_excludes.find(file.string()) == _excludes.end()){
				{
					unique_lock<std::mutex> lock(_path_queue_mutex);
					_path_queue.push(file.string());
				}
				{
					unique_lock<std::mutex> lock(_task_count_mutex);
					_task_count++;
				}
			}
		}
	}else if(filesystem::is_regular_file(path) && !filesystem::is_symlink(path)){
		// Need to verify if this file is actually an image and can be read
		// for now, just put in queue
		{
			unique_lock<std::mutex> lock(_image_queue_mutex);
			_image_queue.push(path_string);
		}
		{
			unique_lock<std::mutex> lock(_task_count_mutex);
			_task_count++;
		}
	}else if(!quiet) printerrln(path_string);

	// One less task
	unique_lock<std::mutex> lock(_task_count_mutex);
	_task_count--;

	return true;
}

bool Pcoll::process_image(bool quiet){

	// Attempt to get the image path from queue
	string path_string;
	{
		unique_lock<std::mutex> lock(_image_queue_mutex);
		if(_image_queue.empty()) return false;
		path_string = _image_queue.front();
		_image_queue.pop();
	}

	// Print statistics
	if(!quiet) print_progress(path_string);

	// Get DHash
	sleep_for(milliseconds(100));

	// Update the total
	{
		unique_lock<std::mutex> lock(_total_mutex);
		_total++;
	}

	// Update the task count
	{
		unique_lock<std::mutex> lock(_task_count_mutex);
		_task_count--;
	}

	return true;
}

unsigned int Pcoll::get_length_of_queue() const{
	return _path_queue.size() + _image_queue.size();
}

unsigned int Pcoll::get_total_images_processed() const{
	return _total;
}

void Pcoll::print_progress(const string& path){
	// Try to normalize the input path string
	string search = filesystem::current_path().string();
    search += "/";
    search = std::regex_replace(path, std::regex(search), "");

	// Build the string
	std::stringstream ss;
	ss << " total: " << get_total_images_processed() << "  queued: " << get_length_of_queue();
	ss << "  path being searched: " << search;

	// Convert to string
	search = ss.str();

	// Put the string to output
	print(search);
}

void Pcoll::println(const string& message){
	// Lock the output
	unique_lock<std::mutex> lock(_cout_mutex);

	// Erase previous string
	if(_print_length != 0){
		std::cout << "\r";
		for(unsigned int i = 0; i < _print_length; i++) std::cout << " ";
		std::cout << "\r";
	}

	// Set the print length to 0
	_print_length = 0;

	// Print the string
	std::cout << message << std::endl << std::flush;
}

void Pcoll::printerrln(const string& message){
	// Lock the output
	unique_lock<std::mutex> lock(_cout_mutex);

	// Erase previous string
	if(_print_length != 0){
		std::cout << "\r";
		for(unsigned int i = 0; i < _print_length; i++) std::cout << " ";
		std::cout << "\r";
	}

	// Set the print length to 0
	_print_length = 0;

	// Print the string
	std::cerr << "Error: " << message << std::endl << std::flush;
}

void Pcoll::print(const string& message){
	// Lock the output
	unique_lock<std::mutex> lock(_cout_mutex);

	// Erase previous string
	if(_print_length != 0){
		std::cout << "\r";
		for(unsigned int i = 0; i < _print_length; i++) std::cout << " ";
		std::cout << "\r";
	}

	// Get length of current string
	_print_length = message.length();

	// Print the string
	std::cout << message << "\r" << std::flush;
}

void Pcoll::reset(){

}

unsigned int Pcoll::get_task_count() const{
	unique_lock<std::mutex> lock(_task_count_mutex);
	return _task_count;
}

unsigned int Pcoll::get_thread_count() const{
	unique_lock<std::mutex> lock(_thread_count_mutex);
	return _thread_count;
}

void Pcoll::notify(){
	// Decrement the working thread number
	{
		unique_lock<std::mutex> lock(_thread_count_mutex);
		_thread_count--;
	}

	// Notify the condition variable
	unique_lock<std::mutex> lock(_work_cv_mutex);
    _work_cv.notify_all();
}

string Pcoll::try_to_convert_to_absolute_path(const string& path) const{
	// Convert path to absolute path if it's not an absolute path
	filesystem::path fs_path = path;
	if(!fs_path.is_absolute())
		fs_path = filesystem::current_path() / fs_path;
	return fs_path.string();
}
