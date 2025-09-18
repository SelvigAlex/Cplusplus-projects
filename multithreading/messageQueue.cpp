#include "messageQueue.hpp"
#include <stdexcept>
#include <string>
#include <iomanip>
// #include <thread>

Message::Message(const MessageType& type, const std::string& message, int senderId)
    : type(type), message(message), senderId(senderId) {}

std::string Message::toString() {
    std::string str;

    // добавляем текущее время
    auto time = std::chrono::system_clock::now();
    std::time_t time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    str += "[" + oss.str() + "]";

    switch (type) {
    case MessageType::INFO:
        str += "[INFO]";
        break;
    case MessageType::WARNING:
        str += "[WARNING]";
        break;
    case MessageType::ERROR:
        str += "[ERROR]";
        break;
    case MessageType::EVENT:
        str += "[EVENT]";
        break;
    default:
        str += "[UNKNOWN]";
        break;
    }

    // // добавляем id потока
    // auto threadId = std::this_thread::get_id();
    // std::ostringstream threadIdStream;
    // threadIdStream << threadId;
    // str += "[Thread " + threadIdStream.str() + "]";


    str += "[Entity " + std::to_string(senderId) + "] " + message;
    return str;
}

void MessageQueue::push(Message msg) {
    std::lock_guard<std::mutex> lock(mtx); // блокируем мьютекс для потокобезопасности
    if (closed) {
        throw std::runtime_error("MessageQueue is closed. Cannot push new messages.");
    }
    queue.push(std::move(msg)); // кладем сообщение в очередь
    cv.notify_one(); // уведомляем ждущий поток что сообщение добавилось в очередь
}

bool MessageQueue::pop(Message& msg) {
    std::unique_lock<std::mutex> lock(mtx); // лочим мьютекс
    cv.wait(lock, [&]{ return !queue.empty() || closed; });
    if (queue.empty() && closed) return false;
    msg = std::move(queue.front());
    queue.pop();
    return true;
}

bool MessageQueue::tryPop(Message& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    if (queue.empty()) return false;
    msg = std::move(queue.front());
    queue.pop();
    return true;
}

void MessageQueue::close() {
    std::lock_guard<std::mutex> lock(mtx);
    closed = true;
    cv.notify_all();
}