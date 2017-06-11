#include "cryptohash.hpp"

#include <openssl/sha.h>
#include <sstream>
#include <ios>
#include <iomanip>

using std::setfill;
using std::stringstream;
using std::setw;
using std::hex;

std::string Crypto_Hash::compute_hash(const string& path){
	std::string input = path;

	unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool Crypto_Hash::compare_hashes(const std::string& hash_one, const std::string& hash_two){
	return hash_one == hash_two;
}
