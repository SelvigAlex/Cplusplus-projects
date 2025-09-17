#include "simulation.hpp"
#include <iostream>

void Simulation::addEntity(std::unique_ptr<Entity> entity) {
    entities.emplace_back(std::move(entity));
}

void Simulation::start() {
    running = true;
    for (auto& entity : entities) {
        Entity* raw = entity.get();
        threads.emplace_back([raw]{ raw->run(); });
    }
    threads.emplace_back([this] { processMessage(); });
}
//! ВАЖНО
// в данной функции мы проходим по всем сущностям из вектора, для того чтобы
// отдать их в поток, но есть несколько нюянсов
// во-первых, передавать в поток ссылку плохо, так как время ее жизни ограничено
// областью видимости цикла, и при окончании работы цикла у нас возникнет висячая
// ссылка (UB)
// во-вторых, копировать сущность в виде std::unique_ptr мы не можем из-за концепции
// данного умного указателя, удален конструктор копирования и оператор =
// в-третьих, использовать семантику перемещения тоже неправильно так как,
// сущность нам нужна и вне потока
// таким образом, выходит, что правильно и безопасно использовать сырой указатель
// на объект, так как адрес скопируется в поток и все будет работать корректно

void Simulation::stop() {
    if (!running) return;
    running = false;
    messageQueue.close();
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads.clear();
}

void Simulation::processMessage() {
    Message msg(MessageType::INFO, "", -1);
    while (messageQueue.pop(msg)) {
        std::cout << msg.toString() << '\n';
    }
}

