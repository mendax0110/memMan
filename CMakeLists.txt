cmake_minimum_required(VERSION 3.5)

project(memMan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCE_FILES
    src/memUtility.cpp
    src/memMan.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})