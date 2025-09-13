#include <iostream>
#include <fstream>
#include <string>
#include "scanner.hpp"

// Объявим CalculateMD5 как extern, если она не экспортируется
extern std::string CalculateMD5(const std::string& filePath);

int main() {
    // Create test file with known content
    std::ofstream test_file("test_md5_file.txt");
    test_file << "Hello World";
    test_file.close();

    std::cout << "Testing MD5 calculation..." << std::endl;
    
    // Test MD5 calculation
    std::string hash = CalculateMD5("test_md5_file.txt");
    
    std::cout << "Calculated MD5: '" << hash << "'" << std::endl;
    std::cout << "Expected MD5:   '5eb63bbbe01eeed093cb22bb8f5acdc3'" << std::endl;
    std::cout << "Hash length: " << hash.length() << std::endl;
    
    if (hash == "5eb63bbbe01eeed093cb22bb8f5acdc3") {
        std::cout << "MD5 calculation is CORRECT!" << std::endl;
        return 0;
    } else if (hash.empty()) {
        std::cout << "MD5 calculation returned EMPTY string!" << std::endl;
        return 1;
    } else {
        std::cout << "MD5 calculation is WRONG!" << std::endl;
        return 1;
    }
}