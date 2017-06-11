#ifndef __PCOLL_CRYPTO_HASH__
#define __PCOLL_CRYPTO_HASH__

#include <string>
#include <bitset>

using std::string;
using std::bitset;

class Crypto_Hash {
public:
	static std::string compute_hash(const string& path);
	static bool compare_hashes(const std::string& hash_one, const std::string& hash_two);
};

#endif //__PCOLL_CRYPTO_HASH__
