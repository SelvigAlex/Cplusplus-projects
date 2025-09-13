# cmake_minimum_required(VERSION 3.12)
# project(MalwareScanner)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # Enable testing
# enable_testing()

# # DLL library
# add_library(scanner SHARED 
#     src/scanner/scanner.cpp
#     src/scanner/scanner.hpp
#     src/scanner/scanner_export.hpp
# )

# target_include_directories(scanner PUBLIC 
#     ${CMAKE_CURRENT_SOURCE_DIR}/src
# )

# target_compile_definitions(scanner PRIVATE SCANNER_EXPORTS)

# # Main executable
# add_executable(scanner_exe src/main.cpp)
# target_link_libraries(scanner_exe scanner)

# # Tests
# add_executable(scanner_tests tests/test_scanner.cpp)
# target_link_libraries(scanner_tests scanner gtest gtest_main)

# add_test(NAME ScannerTests COMMAND scanner_tests)

cmake_minimum_required(VERSION 3.12)
project(MalwareScanner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Устанавливаем пути вывода для библиотек и исполняемых файлов
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Для Debug и Release конфигураций также устанавливаем отдельные пути
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

# Enable testing
enable_testing()

# DLL library
add_library(scanner SHARED 
    src/scanner/scanner.cpp
    src/scanner/scanner.hpp
    src/scanner/scanner_export.hpp
)

target_include_directories(scanner PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(scanner PRIVATE SCANNER_EXPORTS)

# Main executable
add_executable(scanner_exe src/main.cpp)
target_link_libraries(scanner_exe scanner)

# Tests - тесты остаются в build директории
add_executable(scanner_tests tests/test_scanner.cpp)
target_link_libraries(scanner_tests scanner gtest gtest_main)

add_test(NAME ScannerTests COMMAND scanner_tests)