#pragma once

#include <cstddef>
#include <string>
#include <atomic>
#include "messageQueue.hpp"

// базовый класс для любого объекта системы
class Entity {
private:
    std::size_t id;
    std::string name;
    MessageQueue& messageQueue; // ссылка на общую очередь собщений
protected:
    std::atomic<bool>& running;
public:
    Entity(std::size_t id, const std::string& name, MessageQueue &messageQueue, std::atomic<bool>& runFlag);
    virtual void run() = 0; //  метод, выполняющийся в потоке
    void send(const Message& msg); // отправка сообещний в очередь
    std::size_t getId() const; // геттер для id
    virtual ~Entity() = default;
};

// конкретные сущности
class Bus : public Entity {
private:
    int routeNumber;
public:
    Bus(size_t id, const std::string& name, MessageQueue& mq, int route, std::atomic<bool>& runFlag);
    void run() override;
};

class PowerPlant : public Entity {
private:
    int capacity;
public:
    PowerPlant(std::size_t id, const std::string& name, MessageQueue& mq, int capacity, std::atomic<bool>& runFlag);
    void run() override;
};

class DataServer : public Entity {
private:
    std::string ipAddress;
public:
    DataServer(std::size_t id, const std::string& name, MessageQueue& mq, const std::string& ipAddress, std::atomic<bool>& runFlag);
    void run() override;
};
