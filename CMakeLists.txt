cmake_minimum_required(VERSION 3.20)
project(shelfdb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(shelf_core STATIC
        core/Book.cpp
		core/DataFile.cpp
		core/HashIndex.cpp
)
target_include_directories(shelf_core PUBLIC core)

# Тесты
enable_testing()

add_executable(book_test tests/book_test.cpp)
target_link_libraries(book_test PRIVATE shelf_core)
add_test(NAME book_test COMMAND book_test)

add_executable(datafile_test tests/datafile_test.cpp)
target_link_libraries(datafile_test PRIVATE shelf_core)
add_test(NAME datafile_test COMMAND datafile_test)

add_executable(hashindex_test tests/hashindex_test.cpp)
target_link_libraries(hashindex_test PRIVATE shelf_core)
add_test(NAME hashindex_test COMMAND hashindex_test)