#include "diffhash.hpp"
#include "utility.hpp"
#include <OpenImageIO/imagebufalgo.h>

Difference_Hash::Difference_Hash(const string& path) : _hash(nullptr) {
	ImageBuf* img = Utility::get_image_buffer(path);
	_hash = compute_hash(*img);
	delete img;
}

Difference_Hash::Difference_Hash(const ImageBuf& image) : _hash(compute_hash(image)) {}

Difference_Hash::Difference_Hash(const bitset<64>& difference_hash) : _hash(new bitset<64>(difference_hash)) {}

Difference_Hash::~Difference_Hash(){
	delete _hash;
}

Difference_Hash::Difference_Hash(const Difference_Hash& other) : _hash(nullptr) {
	if(this != &other)
		_hash = new bitset<64>(*(other._hash));
}

Difference_Hash::Difference_Hash(Difference_Hash&& other) : _hash(nullptr) {
	if(this != &other)
		_hash = std::move(other._hash);
}

Difference_Hash& Difference_Hash::operator=(const Difference_Hash& other){
	if(this != &other)
		_hash = new bitset<64>(*(other._hash));
	return *this;
}

Difference_Hash& Difference_Hash::operator=(Difference_Hash&& other){
	if(this != &other){
		delete _hash;
		_hash = std::move(other._hash);
	}
	return *this;
}

bool Difference_Hash::operator==(const Difference_Hash& other) const{
	return *this == other;
}

float Difference_Hash::compare(const Difference_Hash& other) const{
	return compare(*this, other);
}

bitset<64>* Difference_Hash::compute_hash(const ImageBuf& image){

	// step 1: shrink image to 8x8 so there are 64 pixels
	ImageBuf resized;
	ROI roi(0, 8, 0, 8, 0, 1, 0, image.nchannels()); // define 8x8 image
	ImageBufAlgo::resample(resized, image, NULL, roi); // resize

	// step 3: compute difference
	bitset<64>* hash = new bitset<64>();

	// Select the last pixel in the image as previous pixel because we will start with first pixel in the image
	float previous_pixel;
	resized.getpixel(7, 7, 0, &previous_pixel);

	// Go over every pixel
	for(ImageBuf::ConstIterator<float> it (resized); !it.done(); ++it){

		// Get luminance value of that pixel
		float pixel = (0.2126 * it[0]) + (0.7152 * it[1]) + (0.0722 * it[2]); // https://en.wikipedia.org/wiki/Grayscale#Converting_color_to_grayscale

		// set the value in the bit hash
		hash->set(0, (previous_pixel < pixel ? false : true));

		// Shift hash
		*hash <<= 1;
	}

	return hash;
}

float Difference_Hash::compare(const Difference_Hash& hash_one, const Difference_Hash& hash_two){

	// Get the hamming distance
	unsigned int hamming_distance = bitset<64>(*(hash_one._hash) ^ *(hash_two._hash)).count();

	// Determine the percentage based on hamming distance, longer distance will reduce the score
	return 1.0 - (hamming_distance / 64.0f);
}

std::ostream& operator<<(std::ostream& os, const Difference_Hash &dh){
    return os << "Difference Hash: " << *(dh._hash);
}

std::size_t Difference_Hash::hash() const{
	return std::hash<bitset<64>>()(*_hash);
}
