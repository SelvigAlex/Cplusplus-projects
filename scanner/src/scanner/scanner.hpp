#pragma once
#include "scanner_export.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct ScanResult {
    size_t totalFiles = 0;
    size_t maliciousFiles = 0;
    size_t errorCount = 0;
    double executionTime = 0.0;
};

class SCANNER_API MalwareScanner {
public:
    MalwareScanner();
    ~MalwareScanner();

    bool LoadMalwareBase(const std::string& csvFilePath);
    ScanResult ScanDirectory(const std::string& directoryPath, 
                           const std::string& logFilePath,
                           size_t threadCount = 0);

private:
    std::unordered_map<std::string, std::string> malwareHashes_;
};

// Экспортируем функцию CalculateMD5 для тестирования
SCANNER_API std::string CalculateMD5(const std::string& filePath);