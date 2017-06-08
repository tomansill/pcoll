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

#if __has_include("filesystem")
#define FS_EXPERIMENTAL 0
#include <filesystem>
namespace filesystem = std::filesystem;
#elif __has_include("boost/filesystem.hpp")
#define FS_EXPERIMENTAL 1
#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;
#warning "NOTICE: boost filesystem is used"
#elif __has_include("experimental/filesystem")
#define FS_EXPERIMENTAL 2
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#warning "NOTICE: g++ experimental filesystem is used"
#else
#warning "No filesystem library!"
#endif

using std::unordered_map;
using std::unordered_set;
using std::string;
using std::bitset;

struct Results {
	Results() : files(), space_used(), collisions() {}
	unsigned int files;
	unsigned long int space_used;
	std::list<std::pair<string, std::list<std::pair<string, float>>>> collisions;
};

class Pcoll {
public:

	/** Constructor for Pcoll object - initializes databases and all necessary objects for processing
	 *	@param similarity_percentage percentage of similarity in range of 0.0f to 1.0f. Inputs are assumed to be valid
	 */
	Pcoll(float similarity_percentage);

	/** Constructor for Pcoll object - initializes databases and all necessary objects for processing
	 *	@param similarity_percentage percentage of similarity in range of 0.0f to 1.0f. Inputs are assumed to be valid
	 *	@param directories list of directories to be searched
	 *	@param excludes set of paths to be excluded from the searched
	 */
	Pcoll(float similarity_percentage, const std::list<string> &directories, unordered_set<string> &excludes);

	/** Processes the directories with default number of threads
	 *	@param quiet turns off the progress if set to true
	 *	@return results of collisions
	 */
	Results process(bool quiet);

	/** Processes the directories with default number of threads
	 *	@param quiet turns off the progress if set to true
	 *	@param threads specifies the number of threads to be run for the processing
	 *	@return results of collisions
	 */
	Results process(bool quiet, unsigned int threads);

	/** Processes a path on the path queue
	 *	@param quiet turns off the progress if set to true
	 *	@return true if something was on the queue, otherwise false
	 */
	bool process_path(bool quiet);

	/** Processes an image on the image queue
	 *	@param quiet turns off the progress if set to true
	 *	@return true if something was on the queue, otherwise false
	 */
	bool process_image(bool quiet);

	/** Gets the length of both path and image queues
	 *	@return length of combined queues
	 */
	unsigned int get_length_of_queue() const;

	/** Gets the number of total images processed so far
	 *	@return total number of images
	 */
	unsigned int get_total_images_processed() const;

	/** Prints the progress update including total images processed, length of queue, and current path
	 *	This function will erase the previous statement and write on the same line
	 *	@param path path that is currently being processed
	 */
	void print_progress(const string& path);

	/** Prints a message after erasing the previous statement
	 *	This function will erase the previous statement and write on the same line
	 *	@param message message to be displayed
	 */
	void println(const string& message);

	/** Prints a message to error stream after erasing the previous statement
	 *	This function will erase the previous statement and write on the same line
	 *	@param message message to be displayed
	 */
	void printerrln(const string& message);

	/** Prints a message on the same line after erasing the previous statement
	 *	This function will erase the previous statement and write on the same line
	 *	@param message message to be displayed
	 */
	void print(const string& message);

	/** Resets the object NOTE: unimplemented */
	void reset();
private:

	/** Get the count of tasks
	 *	@return number of remaining tasks
	 */
	unsigned int get_task_count() const;

	/** Get the count of threads
	 *	@return number of remaining working threads
	 */
	unsigned int get_thread_count() const;

	/** Notifies the main thread that this thread has finished its job */
	void notify();

	/** Attempts to convert a path to absolute
	 *	@param path path that is possibly absolute or notify
	 *	@return absolute string if the string wasnt absolute, otherwise return original string
	 */
	string try_to_convert_to_absolute_path(const string& path) const;

	/** Value for similarity percentage */
	float _similarity_percentage;

	/** Set of excluded directories */
	unordered_set<string>& _excludes;

	/** hash database */
	std::unordered_map<string, string> _hash_database; // database
	mutable std::mutex _hash_database_mutex; // mutex for database

	/** dhash database */
	std::unordered_map<string, bitset<64>> _dhash_database; // database
	mutable std::mutex _dhash_database_mutex; // mutex for database

	/** collision database */
	unordered_map<string, unordered_map<string, float>> _collisions; // database
	mutable std::mutex _collisions_mutex; // mutex for database

	/** directory search path queue */
	std::queue<string> _path_queue; // queue
	mutable std::mutex _path_queue_mutex; // mutex for queue

	/** images processing queue */
	std::queue<string> _image_queue; // queue
	mutable std::mutex _image_queue_mutex; // mutex for queue

	/** tasks count */
	unsigned int _task_count;
	mutable std::mutex _task_count_mutex;

	/** thread count */
	unsigned int _thread_count;
	mutable std::mutex _thread_count_mutex;

	/** total number of images processed */
	unsigned int _total; // total
	mutable std::mutex _total_mutex; // mutex for total

	/** output formatting */
	unsigned int _print_length; // length of string printed (used for carriage return erase)
	mutable std::mutex _cout_mutex; // mutex for printing and _print_length variable


    /** condition variable for threads to finish their queues */
	mutable std::mutex _work_cv_mutex;
    std::condition_variable _work_cv;
};

#endif //__PCOLL_PCOLL__
