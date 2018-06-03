# CMakeLists.txt
# CMakeLists specific to unit test target

###########################

set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set (CMAKE_CXX_FLAGS "-std=c++14")

# compiler includes
include_directories(
	${SOURCE_ROOT}
)

# source files
FILE( GLOB_RECURSE  SRC_FILES
    *.cpp
    *.h
)

add_executable( audiofile_helper_tests ${SOURCE_ROOT} ${SRC_FILES} )