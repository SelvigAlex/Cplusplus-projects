#pragma once

#include <memory>
#include <thread>
#include <vector>
#include "messageQueue.hpp"
#include "entity.hpp"

class Simulation {
private:
    std::vector<std::unique_ptr<Entity>> entities; // список сущностей
    std::vector<std::thread> threads; // потоки, где они выполняются
    MessageQueue messageQueue; // общая очередь сообщений
    bool running; // флаг работы
public:
    Simulation() : running(false) {}
    ~Simulation() { stop(); }

    void addEntity(std::unique_ptr<Entity> entity); // добавить объект
    void start(); // запустить все потоки
    void stop(); // корректно завершить работу
    void processMessage(); // обработка сообщений
};