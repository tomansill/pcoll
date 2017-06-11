#include "diffhash.hpp"
#include "utility.hpp"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

bitset<64> Difference_Hash::compute_hash(const string& path){
	return compute_hash(Utility::open_image_path(path));
}

bitset<64> Difference_Hash::compute_hash(const cv::Mat& image){
	using namespace cv;

	// step 1: convert to grayscale
	Mat work;
	cvtColor(image, work, CV_BGR2GRAY);

	// step 2: shrink image to 8x8 so there are 64 pixels
	Size size(8, 8);
	resize(work, work, size);

	// step 3: compute difference
	bitset<64> hash;

	// Select the last pixel in the image as previous pixel because we will start with first pixel in the image
	uchar previous_pixel = work.at<uchar>(7,7);

	// Go over every pixel, starting at columns in each row
	for(unsigned int rows = 0; rows < (unsigned int)size.height; rows++){
		for(unsigned int columns = 0; columns < (unsigned int)size.width; columns++){

			// If even, go left to right, if odd, go right to left
			unsigned int position = (rows % 2) == 0 ? columns : (size.width - columns - 1);

			// Push back the hash string
			hash <<= 1;

			// Retrieve the pixel at the position
			uchar pixel = work.at<uchar>(rows,position);

			// Set the value in the rightmost postion in the hash string
			hash.set(0, (previous_pixel < pixel ? false : true));

			// Update the previous pixel string
			previous_pixel = pixel;
		}
	}

	return hash;
}

float Difference_Hash::compare_hashes(const bitset<64>& hash_one, const bitset<64>& hash_two){

	// Get the hamming distance
	unsigned int hamming_distance = bitset<64>(hash_one^hash_two).count();

	// Determine the percentage based on hamming distance, longer distance will reduce the score
	return 1.0 - (hamming_distance / 64.0f);
}
