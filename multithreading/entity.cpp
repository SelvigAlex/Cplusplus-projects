#include "entity.hpp"
#include <thread>
#include <sstream>

Entity::Entity(std::size_t id, const std::string& name, MessageQueue& messageQueue, std::atomic<bool>& runFlag)
    : id(id), name(name), messageQueue(messageQueue), running(runFlag) {}

void Entity::send(const Message& msg) {
    messageQueue.push(msg);
}

size_t Entity::getId() const  { 
    return id;
}

Bus::Bus(size_t id, const std::string& name, MessageQueue& mq, int route, std::atomic<bool>& runFlag)
    : Entity(id, name, mq, runFlag), routeNumber(route) {}

void Bus::run() {
    while (running) {
        auto threadId = std::this_thread::get_id();
        std::ostringstream threadIdStream;
        threadIdStream << threadId;
        std::string strId = "[Thread " + threadIdStream.str() + "] ";
        send(Message(MessageType::INFO, strId + "Bus on route " + std::to_string(routeNumber) + " is running", getId()));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

PowerPlant::PowerPlant(std::size_t id, const std::string& name, MessageQueue& mq, int capacity, std::atomic<bool>& runFlag)
    : Entity(id, name, mq, runFlag), capacity(capacity) {}

void PowerPlant::run() {
    while (running) {
        auto threadId = std::this_thread::get_id();
        std::ostringstream threadIdStream;
        threadIdStream << threadId;
        std::string strId = "[Thread " + threadIdStream.str() + "] ";
        std::string msgText = strId + "PowerPlant with capacity " + std::to_string(capacity) +
                              " MW is generating electricity";
        send(Message(MessageType::INFO, msgText, getId()));
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

DataServer::DataServer(std::size_t id, const std::string& name, MessageQueue& mq, const std::string& ipAddress, std::atomic<bool>& runFlag)
    : Entity(id, name, mq, runFlag), ipAddress(ipAddress) {}

void DataServer::run() {
    int i = 0;
    while (running) {
        auto threadId = std::this_thread::get_id();
        std::ostringstream threadIdStream;
        threadIdStream << threadId;
        std::string strId = "[Thread " + threadIdStream.str() + "] ";
        std::string msgText = strId + "DataServer at " + ipAddress +
                              " processed request #" + std::to_string(++i);
        send(Message(MessageType::INFO, msgText, getId()));
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

Market::Market(std::size_t id, const std::string& name, MessageQueue& mq, double price, std::atomic<bool>& runFlag)
    : Entity(id, name, mq, runFlag), price(price) {}

void Market::run() {
    while (running) {
        auto threadId = std::this_thread::get_id();
        std::ostringstream threadIdStream;
        threadIdStream << threadId;
        std::string strId = "[Thread " + threadIdStream.str() + "] ";
        std::string msgText = strId + "The store set the price: " + std::to_string(price);
        send(Message(MessageType::INFO, msgText, getId()));
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}