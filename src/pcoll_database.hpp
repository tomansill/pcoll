#ifndef __PCOLL_DATABASE__
#define __PCOLL_DATABASE__

#include <unordered_map>
#include <bitset>
#include <string>
#include <shared_mutex>
#include <list>
#include <utility>

using std::string;

struct Results {
	Results() : files(), space_used(), collisions() {}
	unsigned int files;
	unsigned long int space_used;
	std::list<std::pair<string, std::list<std::pair<string, float>>>> collisions;
};

class Pcoll_Database{
public:
	Pcoll_Database();
	void insert(std::string& path);
	unsigned int size();
	Results compile_similarity_results(bool quiet, float percentage);
	Results compile_similarity_results(bool quiet, float percentage, unsigned int num_threads);
	void reset();
private:
	void print_progress(const unsigned int task_count, const unsigned int collisions);

	unsigned int _total;

	/** difference hash database */
	std::unordered_map<string, std::bitset<64>> _dhash_database;
	std::shared_mutex _dhash_database_mutex;

	/** crypto hash database */
	std::unordered_map<string, std::string> _chash_database;
	std::shared_mutex _chash_database_mutex;
};

#endif //__PCOLL_DATABASE__
