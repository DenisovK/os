cmake_minimum_required(VERSION 3.17)
project(kp)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ZMQ REQUIRED libzmq)

add_executable(server
    server.cpp
    game.h
    player.h
    myMQ.h
)

add_executable(client
    client.cpp
    game.h
    player.h
    myMQ.h
)

target_include_directories(server PRIVATE ${ZMQ_INCLUDE_DIRS})
target_include_directories(client PRIVATE ${ZMQ_INCLUDE_DIRS})

target_link_libraries(server ${ZMQ_LIBRARIES} Threads::Threads)
target_link_libraries(client ${ZMQ_LIBRARIES} Threads::Threads)
