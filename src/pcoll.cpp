#include "pcoll.hpp"

#include <sstream>
#include <chrono>
#include <thread>
#include <memory>
#include <regex>

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

Results Pcoll::find_similar_images(std::list<string>& directories, std::unordered_set<string>& exclude, bool quiet, float percentage, unsigned int num_threads){

	// Fix if zero
	if(num_threads == 0) num_threads = 1;

	// Queues
	Task_Queue<string> path_queue;
	Task_Queue<string> file_queue;

	// Database
	Pcoll_Database db;

	// Build the initial path queue
	for(auto& directory : directories){
		// Poll in the queue
		path_queue.insert(Utility::try_to_convert_to_absolute_path(directory));
	}

	// Build the thread function
	auto thread_function = [&](){

		// Finish all tasks
		while(path_queue.task_count() != 0 || file_queue.task_count() != 0){
			bool path = process_path(quiet, path_queue, exclude, file_queue);
			bool image = process_file(quiet, file_queue, db);

			// If nothing is in the queue, wait a bit for other threads to populate it
			if(!path && !image)
				sleep_for(milliseconds(10)); // Relax for a bit
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

	return db.compile_similarity_results(quiet, percentage, num_threads);;
}

bool Pcoll::process_path(bool quiet, Task_Queue<std::string>& path_queue, std::unordered_set<string>& exclude, Task_Queue<std::string>& file_queue){

	// Get path from the queue
	string path_string;
	try{ path_string = path_queue.poll();
	}catch(Pexception& perr){ return false; }

	// Convert to path object to use filesystem API
	filesystem::path path(path_string);

	// If element is a directory, add items inside the directory to the queue
	if(filesystem::is_directory(path)){

		// Iterate through the directory
		for(filesystem::path file : filesystem::directory_iterator(path)){
			// Check if path is part of excluded directory set
			if(exclude.find(file.string()) == exclude.end())
				path_queue.insert(file.string());
		}
	}else if(filesystem::is_regular_file(path) && !filesystem::is_symlink(path)){
		// Need to verify if this file is actually an image and can be read
		// for now, just put in queue
		file_queue.insert(path_string);

	}else if(!quiet) Utility::sout.printerrln(path_string);

	// One less task
	path_queue.decrement_task_count();

	return true;
}

bool Pcoll::process_file(bool quiet, Task_Queue<std::string>& file_queue, Pcoll_Database& db){

	// Get path from the queue
	string path_string;
	try{ path_string = file_queue.poll();
	}catch(Pexception& perr){ return false; }

	// Print statistics
	if(!quiet) print_progress(path_string, db);

	// Insert into database
	try{
		db.insert(path_string);
	}catch(Pexception& pe){
		Utility::sout.printerrln(pe.what());
	}

	// Update the task count
	file_queue.decrement_task_count();

	return true;
}


void Pcoll::print_progress(const string& path, Pcoll_Database& db){
	// Try to normalize the input path string
	/*
	string search = filesystem::current_path().string();
    search += "/";
    search = std::regex_replace(path, std::regex(search), "");
	*/
	string search = Utility::try_to_normalize_path(path);

	// Build the string
	std::stringstream ss;
	ss << " total: " << db.size();
	ss << "  file path being searched: " << search;

	// Convert to string
	search = ss.str();

	// Put the string to output
	Utility::sout.print(search);
}
