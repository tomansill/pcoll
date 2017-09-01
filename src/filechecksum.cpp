#include "filechecksum.hpp"
#include "utility.hpp"

#include <openssl/sha.h>
#include <sstream>
#include <ios>
#include <iomanip>
#include <fstream>

using std::setfill;
using std::stringstream;
using std::setw;
using std::hex;

File_Checksum::File_Checksum() : _hash(nullptr) {}

File_Checksum::File_Checksum(const string& input) : _hash(compute_hash_by_input(input)) {}

File_Checksum::~File_Checksum(){
	delete _hash;
}

File_Checksum::File_Checksum(const File_Checksum& other) : _hash(nullptr) {
	if(this != &other)
		_hash = new string(*(other._hash));
}

File_Checksum::File_Checksum(File_Checksum&& other) : _hash(nullptr) {
	if(this != &other)
		_hash = std::move(other._hash);
}

File_Checksum& File_Checksum::operator=(const File_Checksum& other){
	if(this != &other)
		_hash = new string(*(other._hash));
	return *this;
}

File_Checksum& File_Checksum::operator=(File_Checksum&& other){
	if(this != &other){
		delete _hash;
		_hash = std::move(other._hash);
	}
	return *this;
}

bool File_Checksum::operator==(const File_Checksum& other) const{
	return *this == other;
}

std::string* File_Checksum::compute_hash_by_input(const string& input){
	unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }

	string* heap = new string(ss.str());

	return heap;
}

File_Checksum* File_Checksum::compute_hash_by_file(const string& path){

	// Create context
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

	// Open the file
	std::ifstream input_file(path);
	if(!input_file.is_open()) throw Pexception("Cannot open file '" + path + "'!");

	// Digest
	string line;
	while(std::getline(input_file, line))
		SHA256_Update(&sha256, line.c_str(), line.size());

	// Close the file
	input_file.close();

	// Get the final hash
	unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

	// Convert to string
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) ss << hex << setw(2) << setfill('0') << (int)hash[i];

	// Convert to pointer
	string* heap = new string(ss.str());

	File_Checksum* chk = new File_Checksum();
	chk->_hash = heap;

    return chk;
}

std::ostream& operator<<(std::ostream& os, const File_Checksum &fc){
    return os << "SHA256: " << *(fc._hash);
}

std::string& File_Checksum::get_string() const{
	return *_hash;
}
