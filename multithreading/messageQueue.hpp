#pragma once

#include <condition_variable>
#include <queue>
#include <string>
#include <mutex>

// тип сообщения
enum class MessageType {
    INFO,
    WARNING,
    ERROR,
    EVENT
};

// сообщение, которое пересылается между потоками
class Message {
private:
    MessageType type; // тип сообщения
    std::string message; // текст сообщения
    int senderId; // ID источника
public:
    Message(const MessageType& type, const std::string& message, int senderId);
    std::string toString();
};

// общая очередь для пересылки сообщений между потоками
class MessageQueue {
private:
    std::queue<Message> queue; // сами сообщения
    std::mutex mtx; // защита очереди
    std::condition_variable cv; // "будить" потоки
    bool closed = false; // флаг, помечающий, что очередь закрывается
public:
    void push(const Message& msg); // положить сообщение
    bool pop(Message& msg); // забрать сообщение (с блокировкой, ждать если пусто)
    bool tryPop(Message& msg); // неблокировать
    void close(); // пометить очередь закрытой и разбудить все ожидающие потоки
};