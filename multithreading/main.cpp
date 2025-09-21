#include "entity.hpp"
#include "simulation.hpp"
#include <memory>


int main() {
    Simulation simulation;

    // создаем сущность - автобус и добавляем в Simulation
    simulation.addEntity(std::make_unique<Bus>(1, "Bus-42", simulation.getMessageQueue(), 42, simulation.getRunning()));
    simulation.addEntity(std::make_unique<Bus>(2, "Bus-15", simulation.getMessageQueue(), 15, simulation.getRunning()));
    simulation.addEntity(std::make_unique<Bus>(3, "Bus-7", simulation.getMessageQueue(), 7, simulation.getRunning()));

    // создаем сущности - сервер и электростанция
    simulation.addEntity(std::make_unique<PowerPlant>(4, "Nuclear Plant", simulation.getMessageQueue(), 1000, simulation.getRunning()));
    simulation.addEntity(std::make_unique<DataServer>(5, "Main Server", simulation.getMessageQueue(), "192.168.1.1", simulation.getRunning()));
    simulation.addEntity(std::make_unique<Market>(6, "Store", simulation.getMessageQueue(), 5000., simulation.getRunning()));

    simulation.start();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    simulation.stop();

    return 0;
}