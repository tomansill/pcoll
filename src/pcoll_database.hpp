#ifndef __PCOLL_DATABASE__
#define __PCOLL_DATABASE__

#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <string>
#include <shared_mutex>
#include <list>
#include <utility>
#include <functional>

#include "diffhash.hpp"
#include "filechecksum.hpp"

using std::string;

struct Results {
	Results() : files(0), space_used(0), collisions() {}
	unsigned int files;
	unsigned long int space_used;
	std::list<std::pair<string, std::list<std::pair<string, float>>>> collisions;
};

class Pcoll_Database{
public:
	Pcoll_Database();
	~Pcoll_Database();
	void insert(std::string& path);
	unsigned int size();
	Results compile_similarity_results(bool quiet, float percentage);
	Results compile_similarity_results(bool quiet, float percentage, unsigned int num_threads);
	void reset();
private:
	void print_progress(const unsigned int task_count, const unsigned int collisions);
	unordered_map<std::size_t, std::unordered_map<std::size_t, float>> compile_dhash_similarity(float percentage, unsigned int num_threads);

	unsigned int _total;

	/** path string storage */
	std::list<std::string*> _path_storage;
	std::shared_mutex _path_storage_mutex;

	/** dhash storage */
	std::list<Difference_Hash*> _dhash_storage;
	std::shared_mutex _dhash_storage_mutex;

	/** chash storage */
	std::list<File_Checksum*> _chash_storage;
	std::shared_mutex _chash_storage_mutex;

	/** path to checksum hash database */
	std::unordered_map<std::size_t, File_Checksum*> _path_to_chash_database;
	std::shared_mutex _path_to_chash_database_mutex;

	/** checksum to set of paths database */
	std::unordered_map<std::size_t, std::unordered_set<std::string*>> _chash_to_path_set_database;
	std::shared_mutex _chash_to_path_set_database_mutex;

	/** difference hash database */
	std::unordered_map<std::size_t, Difference_Hash*> _dhash_database;
	std::shared_mutex _dhash_database_mutex;
};

#endif //__PCOLL_DATABASE__
