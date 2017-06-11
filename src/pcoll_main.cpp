#include "pcoll.hpp"

#include <iomanip>
#include <regex>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>

#include "filesystem.hpp"

using filesystem::path;
using filesystem::exists;
using filesystem::is_directory;

using std::string;
using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::flush;
using std::unordered_set;
using std::list;
using std::istream;

static const float DEFAULT_SIMILARITY_PERCENTAGE = 0.9f;

string cleanFileBackSlash(const path &fpath){
	string dir = fpath.string();
	if(dir[dir.length()-1] == '/' || dir[dir.length()-1] == '\\'){
		unsigned int found = dir.find_last_of("/\\");
		dir = dir.substr(0,found);
	}
	return dir;
}

int usage(const char* program_name, const string& message){
    if(message.length() != 0) cerr << "ERROR: " << message << endl;
    cout << "Usage: " << program_name << " -q -t <integer> -p <float/integer> <directory> ...<additional_directories> -n <excluded_directory> ...<excluded_directories>" << endl;
    cout << "  [options]" << endl;
    cout << "\t-q :\tquiet mode - default is disabled" << endl;
	cout << "\t-t :\tthread count - default is your CPU's core count" << endl;
	cout << "\t-p :\tsimilarity percentage - set a minimum similarity percentage. " << endl;;
	cout << "\t\tMust be either a float number between 0.0-1.0 or an integer between 0 and 100. Default value is " << DEFAULT_SIMILARITY_PERCENTAGE << endl;
    cout << "\t-n :\texclude flag - list directories you want to be excluded from the search" << endl;
    return -1;
}

int usage(const char* program_name){
    return usage(program_name, "");
}

int main(int argc, char* argv[]){
    // Write a welcome message
    cout << "pcoll v0.1 - finds similar pictures in directories" << endl;

    // Check input for any errors
    if(argc < 2) return usage(argv[0]);

	unsigned int arg_pos = 1;

    // Check if user has flagged for quiet mode
    bool quiet = false;
	bool thread = false;
	bool percent = false;
	unsigned int thread_count = 1;
	float percentage = DEFAULT_SIMILARITY_PERCENTAGE;
	while(strcmp(argv[arg_pos], "-q") == 0 || strcmp(argv[arg_pos], "-t") == 0 || strcmp(argv[arg_pos], "-p") == 0){

		// Quiet flag
		if(strcmp(argv[arg_pos], "-q") == 0){
			if(quiet == true) return usage(argv[0]);
			quiet = true;
			arg_pos++;
		}

		// Threads count option
		if(strcmp(argv[arg_pos], "-t") == 0){
			if(thread == true) return usage(argv[0]);
			arg_pos++;
			if(!std::regex_match(argv[arg_pos], std::regex("[-]?([0-9]*.)?[0-9]+")))
				return usage(argv[0], "the thread count must be a positive integer above zero!");

			int input = std::atoi(argv[arg_pos]);

			if(input < 0)
				return usage(argv[0], "the thread count must be a positive integer above zero!");

			thread_count = input;
			arg_pos++;
			thread = true;
		}

		// Percentage option
		if(strcmp(argv[arg_pos], "-p") == 0){
			if(percent == true) return usage(argv[0]);
			arg_pos++;
			if(std::regex_match(argv[arg_pos], std::regex("[-]?[0-9]+"))){
				int percent_int = std::atoi(argv[arg_pos]);
				if(percent_int > 100 || percent_int < 0)
					return usage(argv[0], "the similarity percentage value must be an integer ranging from 0 to 100 (inclusive) or a float number ranging from 0.0 to 1.0 exclusively!");
				percentage = percent_int / 100.0f;
			}else if(std::regex_match(argv[arg_pos], std::regex("[-]?([0-9]*.)?[0-9]+"))){
				percentage = std::atof(argv[arg_pos]);
				if(percentage > 100.0f || percentage < 0.0f)
					return usage(argv[0], "the similarity percentage value must be an integer ranging from 0 to 100 (inclusive) or a float number ranging from 0.0 to 1.0 exclusively!");
			}else{
				return usage(argv[0], "the similarity percentage value must be an integer ranging from 0 to 100 (inclusive) or a float number ranging from 0.0 to 1.0 exclusively!");
			}
			arg_pos++;
			percent = true;
		}
	}

    // Process the arguments and check them for errors
    list<string> directories;
    int position = -1;
    unsigned int directories_size = 0;
    for(unsigned int i = arg_pos; i < (unsigned int)argc; i++){
        if(strcmp(argv[i], "-n") == 0){
            position = i + 1;
            break;
        }

        path directory(argv[i]);

        // Check if directory exists
        if(!exists(directory)) return usage(argv[0], "Directory '" + directory.string() + "' does not exist");

        // Check if directory is actually a directory
        if(!is_directory(directory)) return usage(argv[0], "Specified path '" + directory.string() + "' is not a directory");

        // Add to vector
        directories.push_back(cleanFileBackSlash(directory));
        directories_size++;
    }

    // Check if input directories are not empty
    if(directories_size == 0) return usage(argv[0]);

    // Iterate the excluded directories list if -n is detected
    unordered_set<string> exclude;
    if(position != -1){
        for(int i = position; i < argc; i++){
            path directory(argv[i]);

            // Check if directory exists
            if(!exists(directory)) return usage(argv[0], "Directory '" + directory.string() + "' does not exist");

            // Check if directory is actually a directory
            if(!is_directory(directory)) return usage(argv[0], "Specified path '" + directory.string() + "' is not a directory");

            // Add it in exclude list
            exclude.insert(cleanFileBackSlash(directory));
        }
    }

    // Start the hasher
	auto results = thread ?
		Pcoll::find_similar_images(directories, exclude, quiet, percentage, thread_count) :
		Pcoll::find_similar_images(directories, exclude, quiet, percentage, Utility::get_default_cores_count());

	// Show results
	unsigned int count = 1;
	for(auto& entry : results.collisions){
		cout << count++ << "/" << results.collisions.size() << " images: " << entry.second.size() << " - " << Utility::try_to_normalize_path(entry.first) << endl;
		unsigned int inner_count = 1;
		for(auto& inner : entry.second){
			cout << "\t" << inner_count++ << "/" << entry.second.size() << " " << ((int)(inner.second * 100)) << "% - " << Utility::try_to_normalize_path(inner.first) <<  endl;
		}
		cout << endl;
	}
	cout << "Total similar files found: " << results.files << endl;
}
