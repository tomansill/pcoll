#ifndef __FS_LOAD__
#define __FS_LOAD__

# if __has_include("filesystem")
# define FS_EXPERIMENTAL 0
# include <filesystem>
namespace filesystem = std::filesystem;
# elif __has_include("boost/filesystem.hpp")
# define FS_EXPERIMENTAL 1
# include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;
# pragma message "NOTICE: boost filesystem is used"
# elif __has_include("experimental/filesystem")
# define FS_EXPERIMENTAL 2
# include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
# pragma message "NOTICE: g++ experimental filesystem is used"
# else
# warning "No filesystem library!"
# endif

#endif //__FS_LOAD__
