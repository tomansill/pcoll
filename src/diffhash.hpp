#ifndef __PCOLL_DIFFHASH__
#define __PCOLL_DIFFHASH__

#include <string>
#include <bitset>
#include <opencv2/core/core.hpp>

using std::string;
using std::bitset;

class Difference_Hash {
public:
	static bitset<64> compute_hash(const string& path);
	static bitset<64> compute_hash(const cv::Mat image);
	static float compare_hashes(const bitset<64>& hash_one, const bitset<64>& hash_two);
};

#endif //__PCOLL_DIFFHASH__
