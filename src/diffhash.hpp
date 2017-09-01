#ifndef __PCOLL_DIFFHASH__
#define __PCOLL_DIFFHASH__

#include <string>
#include <bitset>
#include <OpenImageIO/imagebuf.h>

using std::string;
using std::bitset;

using namespace OIIO;

class Difference_Hash {
public:
	Difference_Hash(const string& path);
	Difference_Hash(const ImageBuf& image);
	Difference_Hash(const bitset<64>& difference_hash);
	~Difference_Hash();
	Difference_Hash(const Difference_Hash& other);
	Difference_Hash(Difference_Hash&& other);
	Difference_Hash& operator=(const Difference_Hash& other);
	Difference_Hash& operator=(Difference_Hash&& other);
	bool operator==(const Difference_Hash& other) const;
	float compare(const Difference_Hash& other) const;
	friend std::ostream& operator<<(std::ostream& os, const Difference_Hash &dh);
	std::size_t hash() const;

	// Static data members
	static float compare(const Difference_Hash& hash_one, const Difference_Hash& hash_two);

private:
	bitset<64>* _hash;
	static bitset<64>* compute_hash(const ImageBuf& image);
};

#endif //__PCOLL_DIFFHASH__
