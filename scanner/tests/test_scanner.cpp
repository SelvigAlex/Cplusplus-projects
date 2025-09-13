#include <gtest/gtest.h>
#include "scanner/scanner.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

class MalwareScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory structure
        fs::create_directories("test_dir/subdir");
        
        // Create test files with EXACT content (no extra newlines)
        std::ofstream file1("test_dir/file1.txt", std::ios::binary);
        file1.write("Hello World", 11);  // Exact 11 bytes: "Hello World"
        file1.close();
        
        std::ofstream file2("test_dir/file2.txt", std::ios::binary);
        file2.write("Malicious content", 17);  // Different content
        file2.close();
        
        std::ofstream file3("test_dir/subdir/file3.txt", std::ios::binary);
        file3.write("Another file", 12);  // Different content
        file3.close();
        
        // Create malware base with correct MD5
        std::ofstream base("test_base.csv", std::ios::binary);
        base.write("b10a8db164e0754105b7a99be72e3fe5;TestMalware\n", 44);
        base.close();

        // Debug: check file sizes
        std::cout << "File sizes:" << std::endl;
        std::cout << "file1.txt: " << fs::file_size("test_dir/file1.txt") << " bytes" << std::endl;
        std::cout << "file2.txt: " << fs::file_size("test_dir/file2.txt") << " bytes" << std::endl;
        std::cout << "file3.txt: " << fs::file_size("test_dir/subdir/file3.txt") << " bytes" << std::endl;
    }

    void TearDown() override {
        // Clean up test files
        try {
            fs::remove_all("test_dir");
            fs::remove("test_base.csv");
            fs::remove("test_log.log");
        } catch (...) {
            // Ignore cleanup errors
        }
    }
};

TEST_F(MalwareScannerTest, LoadMalwareBase) {
    MalwareScanner scanner;
    EXPECT_TRUE(scanner.LoadMalwareBase("test_base.csv"));
}

TEST_F(MalwareScannerTest, ScanDirectoryFindsMaliciousFiles) {
    MalwareScanner scanner;
    ASSERT_TRUE(scanner.LoadMalwareBase("test_base.csv"));
    
    // Debug: check what files were created
    std::cout << "Test files created:" << std::endl;
    for (const auto& entry : fs::recursive_directory_iterator("test_dir")) {
        if (entry.is_regular_file()) {
            std::cout << "  " << entry.path() << " (" << entry.file_size() << " bytes)" << std::endl;
        }
    }
    
    ScanResult result = scanner.ScanDirectory("test_dir", "test_log.log", 1);
    
    std::cout << "Scan results:" << std::endl;
    std::cout << "  Total files: " << result.totalFiles << std::endl;
    std::cout << "  Malicious files: " << result.maliciousFiles << std::endl;
    std::cout << "  Errors: " << result.errorCount << std::endl;
    
    EXPECT_EQ(result.totalFiles, 3);
    EXPECT_EQ(result.maliciousFiles, 1); // Only file1.txt should match
    EXPECT_GE(result.executionTime, 0.0);
    
    // Check if log file was created and contains expected content
    std::ifstream log("test_log.log");
    ASSERT_TRUE(log.is_open());
    
    std::string content((std::istreambuf_iterator<char>(log)), 
                       std::istreambuf_iterator<char>());
    
    std::cout << "Log file content:" << std::endl;
    std::cout << content << std::endl;
    
    EXPECT_TRUE(content.find("file1.txt") != std::string::npos);
    EXPECT_TRUE(content.find("TestMalware") != std::string::npos);
}

TEST_F(MalwareScannerTest, ScanInvalidDirectory) {
    MalwareScanner scanner;
    ASSERT_TRUE(scanner.LoadMalwareBase("test_base.csv"));
    
    ScanResult result = scanner.ScanDirectory("nonexistent_dir_12345", "test_log2.log");
    
    EXPECT_GT(result.errorCount, 0);
}