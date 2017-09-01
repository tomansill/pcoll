cmake_minimum_required(VERSION 3.7)
project(pcoll)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
	src/utility.cpp
	src/task_queue.cpp
	src/diffhash.cpp
	src/filechecksum.cpp
	src/pcoll_database.cpp
    src/pcoll.cpp
	src/pcoll_main.cpp
	)

set(CMAKE_EXE_LINKER_FLAGS "-lboost_filesystem -lboost_system -lssl -lcrypto -lpthread -lOpenImageIO")

add_compile_options(-pg -g -gdwarf-2 -Wall -Wextra -Weffc++ -pedantic)

add_executable(pcoll ${SOURCE_FILES})
