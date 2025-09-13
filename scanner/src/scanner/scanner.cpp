#include "scanner.hpp"
#include <windows.h>
#include <wincrypt.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>
#include <functional>

namespace fs = std::filesystem;

class ThreadPool {
public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F>
    void Enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    void WaitAll() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        condition_empty.wait(lock, [this]() {
            return tasks.empty();
        });
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable condition_empty;
    bool stop;
};

MalwareScanner::MalwareScanner() = default;
MalwareScanner::~MalwareScanner() = default;

bool MalwareScanner::LoadMalwareBase(const std::string& csvFilePath) {
    malwareHashes_.clear();
    
    std::ifstream file(csvFilePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove carriage return if present (for Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        size_t pos = line.find(';');
        if (pos != std::string::npos) {
            std::string hash = line.substr(0, pos);
            std::string verdict = line.substr(pos + 1);
            malwareHashes_[hash] = verdict;
        }
    }

    return true;
}

std::string CalculateMD5(const std::string& filePath) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE rgbHash[16];
    DWORD cbHash = 16;
    CHAR rgbDigits[] = "0123456789abcdef";
    std::string result;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return "";
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }

    const size_t BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE];

    while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
        if (!CryptHashData(hHash, (BYTE*)buffer, (DWORD)file.gcount(), 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }
    }

    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        for (DWORD i = 0; i < cbHash; i++) {
            result += rgbDigits[rgbHash[i] >> 4];
            result += rgbDigits[rgbHash[i] & 0xf];
        }
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return result;
}

ScanResult MalwareScanner::ScanDirectory(const std::string& directoryPath, 
                                       const std::string& logFilePath,
                                       size_t threadCount) {
    auto startTime = std::chrono::high_resolution_clock::now();
    ScanResult result;

    if (malwareHashes_.empty()) {
        std::cerr << "Warning: Malware base is empty" << std::endl;
        return result;
    }

    // Validate directory exists
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        std::cerr << "Error: Directory does not exist: " << directoryPath << std::endl;
        result.errorCount++;
        return result;
    }

    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) threadCount = 4;
    }

    std::ofstream logFile(logFilePath);
    if (!logFile.is_open()) {
        std::cerr << "Error: Could not open log file: " << logFilePath << std::endl;
        result.errorCount++;
        return result;
    }

    // Collect file paths
    std::vector<std::string> filePaths;
    std::atomic<size_t> filesProcessed{0};
    std::atomic<size_t> maliciousFound{0};
    std::atomic<size_t> errors{0};

    try {
        for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                try {
                    filePaths.push_back(entry.path().string());
                } catch (const std::exception& e) {
                    errors++;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
        errors++;
    }

    result.totalFiles = filePaths.size();

    if (filePaths.empty()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTime = std::chrono::duration<double>(endTime - startTime).count();
        return result;
    }

    std::mutex logMutex;

    // Use simpler approach without ThreadPool class
    std::vector<std::thread> threads;
    size_t filesPerThread = (filePaths.size() + threadCount - 1) / threadCount;

    for (size_t i = 0; i < threadCount; ++i) {
        threads.emplace_back([&, i]() {
            size_t start = i * filesPerThread;
            size_t end = std::min(start + filesPerThread, filePaths.size());

            for (size_t j = start; j < end; ++j) {
                const auto& filePath = filePaths[j];
                
                try {
                    std::string hash = CalculateMD5(filePath);
                    if (hash.empty()) {
                        errors++;
                        continue;
                    }

                    auto it = malwareHashes_.find(hash);
                    if (it != malwareHashes_.end()) {
                        maliciousFound++;
                        
                        std::lock_guard<std::mutex> lock(logMutex);
                        logFile << "File: " << filePath << "\n";
                        logFile << "Hash: " << hash << "\n";
                        logFile << "Verdict: " << it->second << "\n";
                        logFile << "----------------------------------------\n";
                    }
                } catch (const std::exception& e) {
                    errors++;
                }

                filesProcessed++;
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    result.maliciousFiles = maliciousFound;
    result.errorCount = errors;

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTime = std::chrono::duration<double>(endTime - startTime).count();

    return result;
}