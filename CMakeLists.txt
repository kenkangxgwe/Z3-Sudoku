cmake_minimum_required(VERSION 3.12)
project(Z3-Sudoku)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        src/SymName.cpp
        src/NumPred.cpp
        src/Board.cpp
        src/main.cpp
        )

find_package(Z3 REQUIRED)
include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${Z3_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${Z3_LIBRARIES}
        )