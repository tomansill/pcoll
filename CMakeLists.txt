cmake_minimum_required(VERSION 3.7)
project(pcoll)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-lboost_filesystem -lboost_system -lssl -lcrypto -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lpthread -lopencv_features2d -lopencv_xfeatures2d -lopencv_flann")

add_compile_options(-g -gdwarf-2 -Wall -Wextra -Weffc++ -pedantic)

set(SOURCE_FILES
	src/utility.cpp
	src/task_queue.cpp
	src/diffhash.cpp
	src/pcoll_database.cpp
    src/pcoll.cpp
	src/pcoll_main.cpp
	)
add_executable(pcoll ${SOURCE_FILES})
