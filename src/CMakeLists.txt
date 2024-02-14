cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(simulator)

find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS} -g -Wall")

add_executable(simulator simulator.cpp manager.cpp config.cpp observer.cpp networkObject.cpp interface.cpp link.cpp packetHandler.cpp switch.cpp endpoint.cpp agent.cpp actorCritic.cpp sarsa.cpp tilecoder.cpp flow.cpp generator.cpp packet.cpp linUCB.cpp NDDAgents.cpp)

target_link_libraries(simulator "${TORCH_LIBRARIES}")
set_property(TARGET simulator PROPERTY CXX_STANDARD 20)
