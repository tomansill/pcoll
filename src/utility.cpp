#include "utility.hpp"

#include <iostream>
#include <thread>
#include <regex>

using std::string;
using std::unique_lock;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

Synchronized_Output Utility::sout;

unsigned int Utility::get_default_cores_count(){
	// Get the number of cores available on the computer
	unsigned int num_threads = std::thread::hardware_concurrency();

	// The previous function may return zero if it cannot detect the number of cores on the computer
	// Make sure we correct that before passing it in
	return num_threads == 0 ? 1 : num_threads;
}

bool Utility::is_image(const std::string& path){

	OIIO::string_view path_view(path);

	// Construct the image data
	ImageInput* image;

	// Create image
	image = ImageInput::create(path_view);

	// If failed to open, probably not a valid image
	if(!image) return false;

	// Get result
	bool result = image->valid_file(path_view);

	// Destroy the obj
	ImageInput::destroy(image);

	return result;
}

ImageInput* Utility::open_image_path(const std::string& path){
	OIIO::string_view path_view(path);

	// Construct the image data
	ImageInput* image;

	// Try to open image
	image = ImageInput::open(path_view);
	if(!image) throw Pexception("Failed to open image: " + path);

	return image;
}

ImageBuf* Utility::get_image_buffer(const std::string& path){
	OIIO::string_view path_view(path);

	// Construct the image data
	ImageBuf* image = new ImageBuf(path_view);

	return image;
}

string Utility::try_to_convert_to_absolute_path(const string& path){
	// Convert path to absolute path if it's not an absolute path
	filesystem::path fs_path = path;
	if(!fs_path.is_absolute())
		fs_path = filesystem::current_path() / fs_path;
	return fs_path.string();
}

std::string Utility::try_to_normalize_path(const std::string& path){
	string search = filesystem::current_path().string();
    search += "/";
	return std::regex_replace(path, std::regex(search), "");
}

Synchronized_Output::Synchronized_Output() :  _print_length(0), _cout_mutex(), _last_print_time(steady_clock::now()) {}

void Synchronized_Output::println(const string& message){
	// Lock the output
	unique_lock<std::mutex> lock(_cout_mutex);

	// Erase previous string
	if(_print_length != 0){
		std::cout << "\r";
		for(unsigned int i = 0; i < _print_length; i++) std::cout << " ";
		std::cout << "\r";
	}

	// Set the print length to 0
	_print_length = 0;

	// Print the string
	std::cout << message << std::endl << std::flush;

	_last_print_time = steady_clock::now();
}

void Synchronized_Output::printerrln(const string& message){
	// Lock the output
	unique_lock<std::mutex> lock(_cout_mutex);

	// Erase previous string
	if(_print_length != 0){
		std::cout << "\r";
		for(unsigned int i = 0; i < _print_length; i++) std::cout << " ";
		std::cout << "\r";
	}

	// Set the print length to 0
	_print_length = 0;

	// Print the string
	std::cerr << "Error: " << message << std::endl << std::flush;
}

void Synchronized_Output::print(const string& message){
	// Control printing speed
	unsigned long duration = duration_cast<microseconds>(steady_clock::now() - _last_print_time).count();
	if(duration < 1000)
		return;

	// Lock the output
	unique_lock<std::mutex> lock(_cout_mutex);

	// Erase previous string
	if(_print_length != 0){
		std::cout << "\r";
		for(unsigned int i = 0; i < _print_length; i++) std::cout << " ";
		std::cout << "\r";
	}

	// Get length of current string
	_print_length = message.length();

	// Print the string
	std::cout << message << "\r" << std::flush;

	_last_print_time = steady_clock::now();
}
