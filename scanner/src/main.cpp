#include "scanner/scanner.hpp"
#include <iostream>
#include <string>

void PrintUsage() {
    std::cout << "Usage: scanner.exe --base <base.csv> --log <report.log> --path <directory>\n";
    std::cout << "Options:\n";
    std::cout << "  --base    Path to malware base CSV file\n";
    std::cout << "  --log     Path to output log file\n";
    std::cout << "  --path    Path to directory to scan\n";
    std::cout << "  --threads Number of threads (optional, default: auto)\n";
    std::cout << "  --help    Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string basePath, logPath, scanPath;
    size_t threadCount = 1; // Default to single-threaded for stability

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--base" && i + 1 < argc) {
            basePath = argv[++i];
        } else if (arg == "--log" && i + 1 < argc) {
            logPath = argv[++i];
        } else if (arg == "--path" && i + 1 < argc) {
            scanPath = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            threadCount = std::stoul(argv[++i]);
        } else if (arg == "--help") {
            PrintUsage();
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            PrintUsage();
            return 1;
        }
    }

    if (basePath.empty() || logPath.empty() || scanPath.empty()) {
        std::cerr << "Error: Missing required arguments" << std::endl;
        PrintUsage();
        return 1;
    }

    MalwareScanner scanner;
    
    if (!scanner.LoadMalwareBase(basePath)) {
        std::cerr << "Error: Could not load malware base from " << basePath << std::endl;
        return 1;
    }

    std::cout << "Starting scan of directory: " << scanPath << std::endl;
    std::cout << "Using malware base: " << basePath << std::endl;
    std::cout << "Log file: " << logPath << std::endl;
    std::cout << "Threads: " << threadCount << std::endl;
    std::cout << "Scanning..." << std::endl;

    try {
        ScanResult result = scanner.ScanDirectory(scanPath, logPath, threadCount);

        std::cout << "\n=== Scan Results ===" << std::endl;
        std::cout << "Total files processed: " << result.totalFiles << std::endl;
        std::cout << "Malicious files found: " << result.maliciousFiles << std::endl;
        std::cout << "Errors encountered: " << result.errorCount << std::endl;
        std::cout << "Execution time: " << result.executionTime << " seconds" << std::endl;

        if (result.maliciousFiles > 0) {
            std::cout << "WARNING: Malicious files detected! Check log file for details." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during scan: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}