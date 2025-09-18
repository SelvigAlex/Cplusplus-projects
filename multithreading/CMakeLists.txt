cmake_minimum_required(VERSION 3.5)	 # Проверка версии CMake.
										# Если версия установленной программы
										# старее указаной, произойдёт аварийный выход.

add_executable(infrastructure_simulation
    entity.cpp
    messageQueue.cpp
    simulation.cpp
    main.cpp
)

