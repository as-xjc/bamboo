add_executable(distributed-echo-server main.cpp)
add_dependencies(distributed-echo-server bamboo)
target_link_libraries(distributed-echo-server bamboo)