cmake_minimum_required(VERSION 3.20)
project(shelfdb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(shelf_core STATIC
		core/Book.cpp
		core/DataFile.cpp
		core/HashIndex.cpp
		core/Database.cpp
		core/BookField.cpp
		core/Backup.cpp
		core/XlsxWriter.cpp
		core/Exporter.cpp
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

add_executable(database_test tests/database_test.cpp)
target_link_libraries(database_test PRIVATE shelf_core)
add_test(NAME database_test COMMAND database_test)

add_executable(author_test tests/author_test.cpp)
target_link_libraries(author_test PRIVATE shelf_core)
add_test(NAME author_test COMMAND author_test)

add_executable(lifecycle_test tests/lifecycle_test.cpp)
target_link_libraries(lifecycle_test PRIVATE shelf_core)
add_test(NAME lifecycle_test COMMAND lifecycle_test)

add_executable(services_test tests/services_test.cpp)
target_link_libraries(services_test PRIVATE shelf_core)
add_test(NAME services_test COMMAND services_test)
# Замеры времени для отчёта (запускать в Release)
add_executable(benchmark bench/benchmark.cpp)
target_link_libraries(benchmark PRIVATE shelf_core)

# Графический интерфейс (Qt 6)
if(APPLE)
	# Где Homebrew хранит Qt (Apple Silicon и Intel)
	list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" "/usr/local/opt/qt")
endif()

find_package(Qt6 COMPONENTS Widgets)
if(Qt6_FOUND)
	add_executable(shelfdb
			gui/main.cpp
			gui/MainWindow.cpp
			gui/BookDialog.cpp
			gui/FieldValueDialog.cpp
	)
	target_link_libraries(shelfdb PRIVATE shelf_core Qt6::Widgets)
else()
	message(WARNING "Qt6 не найден, GUI не будет собран")
endif()