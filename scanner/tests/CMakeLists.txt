add_executable(scanner_tests test_scanner.cpp)
target_link_libraries(scanner_tests scanner)

# Add include directories
target_include_directories(scanner_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME ScannerTests COMMAND scanner_tests)