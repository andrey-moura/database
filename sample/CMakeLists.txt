#Require a minimum version
cmake_minimum_required(VERSION 3.10)
project(database-sample)

add_executable(database-sample ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
target_link_libraries(database-sample uva-database)