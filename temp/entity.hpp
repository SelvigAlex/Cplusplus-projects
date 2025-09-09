#pragma once

#include <cstddef>
#include <string>
#include "messageQueue.hpp"

// базовый класс для любого объекта системы
class Entity {
private:
    std::size_t id;
    std::string name;
    MessageQueue& messageQueue; // ссылка на оьщеую очередь собщений
public:
  Entity(std::size_t id, const std::string& name, MessageQueue &messageQueue);
  virtual void run() = 0; //  метод, выполняющийся в потоке
  void send(Message msg); // отправка сообещний в очередь
  virtual ~Entity() = default;
};


// конкретные сущности

class Bus : public Entity {
private:
    int routeNumber;
public:
    Bus(std::size_t id, const std::string& name, MessageQueue& mq, int route)
        : Entity(id, name, mq), routeNumber(route) {}
    void run() override;
};

class PowerPlant : public Entity {
private:
    int capacity;
public:
    PowerPlant(std::size_t id, const std::string& name, MessageQueue& mq, int capacity)
        : Entity(id, name, mq), capacity(capacity) {}
    void run() override;
};

class DataServer : public Entity {
private:
    std::string ipAddress;
public:
    DataServer(std::size_t id, const std::string& name, MessageQueue& mq, const std::string& ipAddress)
        : Entity(id, name, mq), ipAddress(ipAddress) {}
    void run() override;
};
