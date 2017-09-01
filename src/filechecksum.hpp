#ifndef __PCOLL_FILECHECKSUM__
#define __PCOLL_FILECHECKSUM__

#include <string>
#include <iostream>

using std::string;

class File_Checksum {
public:
	File_Checksum(const string& input);
	~File_Checksum();
	File_Checksum(const File_Checksum& other);
	File_Checksum(File_Checksum&& other);
	File_Checksum& operator=(const File_Checksum& other);
	File_Checksum& operator=(File_Checksum&& other);
	bool operator==(const File_Checksum& other) const;
	static File_Checksum* compute_hash_by_file(const string& path);
	friend std::ostream& operator<<(std::ostream& os, const File_Checksum &fc);
	std::string& get_string() const;
private:
	File_Checksum();
	std::string* _hash;
	static std::string* compute_hash_by_input(const string& input);
};

#endif //__PCOLL_FILECHECKSUM__
