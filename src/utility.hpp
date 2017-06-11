#ifndef __PCOLL_UTILITY__
#define __PCOLL_UTILITY__

#include <stdexcept>
#include <string>
#include <mutex>

#include "filesystem.hpp"

#include <opencv2/core/core.hpp>

/** PColl Exception
 *  @author Thomas Ansill
 */
class Pexception : public std::runtime_error{
public:
    /** Construct this exception.
     *  @param msg the exception message
     */
    Pexception(const char *msg) : runtime_error(msg){}

	/** Construct this exception.
     *  @param msg the exception message
     */
    Pexception(const std::string& msg) : runtime_error(msg){}

    /** Get a string containing information about the exception.
     *  @return exception message
     */
    virtual const char* what() const throw(){
        return std::runtime_error::what();
    }
};

class Synchronized_Output{
public:
	Synchronized_Output();

	/** Prints a message after erasing the previous statement
	 *	This function will erase the previous statement and write on the same line
	 *	@param message message to be displayed
	 */
	void println(const std::string& message);

	/** Prints a message to error stream after erasing the previous statement
	 *	This function will erase the previous statement and write on the same line
	 *	@param message message to be displayed
	 */
	void printerrln(const std::string& message);

	/** Prints a message on the same line after erasing the previous statement
	 *	This function will erase the previous statement and write on the same line
	 *	@param message message to be displayed
	 */
	void print(const std::string& message);

private:
	/** output formatting */
	unsigned int _print_length; // length of string printed (used for carriage return erase)
	mutable std::mutex _cout_mutex; // mutex for printing and _print_length variable
	std::chrono::time_point<std::chrono::steady_clock> _last_print_time;
};

class Utility {
public:
	static Synchronized_Output sout;

	static cv::Mat open_image_path(const std::string& path);

	static unsigned int get_default_cores_count();

	/** Attempts to convert a path to absolute
	 *	@param path path that is possibly absolute or notify
	 *	@return absolute string if the string wasnt absolute, otherwise return original string
	 */
	static std::string try_to_convert_to_absolute_path(const std::string& path);

	/** Attempts to convert a path to absolute
	 *	@param path path that is possibly absolute or notify
	 *	@return absolute string if the string wasnt absolute, otherwise return original string
	 */
	static std::string try_to_normalize_path(const std::string& path);
};

#endif //__PCOLL_UTILITY__
