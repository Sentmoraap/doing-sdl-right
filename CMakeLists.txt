## CMakeLists.txt
##
##

# Define the following vars in your CMakeCache.txt file(s)

cmake_minimum_required(VERSION 2.6)

PROJECT(doing-sdl-right)

IF(NOT WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic")
ENDIF(NOT WIN32)

FILE(
    GLOB_RECURSE
    project_files
    src/*
)


ADD_EXECUTABLE(sdl-test ${project_files})

SET(CURRENT_TARGETS sdl-test)

IF(WIN32)
	SET(CMAKE_INCLUDE_PATH "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include")
	SET(CMAKE_LIBRARY_PATH "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/lib")
ENDIF(WIN32)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

FIND_PACKAGE(SDL2 REQUIRED)

FOREACH(CURRENT_TARGET ${CURRENT_TARGETS})

    IF(SDL2_FOUND)
      TARGET_LINK_LIBRARIES(${CURRENT_TARGET} ${SDL2_LIBRARIES})
    ENDIF(SDL2_FOUND)

    SET_PROPERTY(TARGET ${CURRENT_TARGET} PROPERTY INCLUDE_DIRECTORIES
      ${CMAKE_SOURCE_DIR}/include/
      ${SDL_INCLUDE_DIR}
    )

ENDFOREACH(CURRENT_TARGET)
